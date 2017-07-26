#include "ofApp.h"

using namespace cv;

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetVerticalSync(false);
	ofSetFrameRate(CAMERA_FRAMERATE);

	initCamera();

	// init cv variables
	src_img = Mat(CAMERA_W, CAMERA_H, CV_8UC1);
	bg_img = Mat(CAMERA_W, CAMERA_H, CV_8UC1);
	diff_img = Mat(CAMERA_W, CAMERA_H, CV_8UC1);
	blur_img = Mat(CAMERA_W, CAMERA_H, CV_8UC1);
	binarized_img = Mat(CAMERA_W, CAMERA_H, CV_8UC1);
	dest_img1 = Mat(CAMERA_W, CAMERA_H, CV_8UC3);
	dest_img2 = Mat(CAMERA_W, CAMERA_H, CV_8UC3);

	// init bg
	grabBG = true;
	bg_start = ofGetElapsedTimeMillis();

	// init fbo
	mapper_fbo.allocate(CAMERA_W, CAMERA_H);
	bg_fbo.allocate(CAMERA_W, CAMERA_H);
	shadow_fbo.allocate(CAMERA_W, CAMERA_H);
	heatmap_fbo.allocate(CAMERA_W, CAMERA_H);
	src_fbo.allocate(CAMERA_W, CAMERA_H);
	blur_fbo.allocate(CAMERA_W, CAMERA_H);
	prev_fbo.allocate(CAMERA_W, CAMERA_H);
	spout_fbo1.allocate(CAMERA_W, CAMERA_H);
	spout_fbo2.allocate(CAMERA_W, CAMERA_H);
	prev_fbo.begin();
	ofBackground(0);
	prev_fbo.end();

	// spout settings
	//sender.init("shadow");
	spout1.init("shadow", CAMERA_W, CAMERA_H);
	spout2.init("heatmap", CAMERA_W, CAMERA_H);

	// init mapper
	bezManager.setup(10); //WarpResolution
	bezManager.setCanvasRatio(2.0); // CanvasSize
	bezManager.addFbo(&mapper_fbo);
	bezManager.loadSettings();
	//bezManager.toggleGuideVisible();

	// gui
	gui_params.setName("parameters");
	gui_params.add(showMapper.set("showMapper", true));
	gui_params.add(binarize_threshold.set("bin thresh", 0, -1.0, 1.0));
	gui_params.add(subtract_threshold.set("sub thresh", 0, -1.0, 1.0));
	gui_params.add(increaseParam.set("increase", 0.02, 0.0, 2.2));
	gui_params.add(decreaseParam.set("decrease", 0.1, 0.0, 1.0));
	gui_params.add(blur_size.set("blur_size", 3, 1, 21));
	gui_params.add(boald_size.set("boald_size", 3, 1, 21));
	gui_params.add(offset_pos.set("offset", ofVec2f(0, 0), ofVec2f(-100, -100), ofVec2f(100, 100)));
	gui_params.add(offset_scale.set("scale", ofVec2f(1.0, 1.0), ofVec2f(0.8, 0.8), ofVec2f(1.2, 1.2)));
	gui.setup(gui_params);
	gui.loadFromFile("settings.xml");
	showGui = true;

	// load shaders
	reloadShader();

	// set display mode
	display_mode = DISPLAY_MODE_CALIBRATION;
}

//--------------------------------------------------------------
void ofApp::update() {

	// grab new frame, 15~20ms
	pgr_camera->grab_image();

	// draw to mapper fbo, 1ms
	mapper_fbo.begin();
	ofBackground(0);
	pgr_camera->draw(0, 0);
	mapper_fbo.end();

	// reshape fbo, 1ms
	src_fbo.begin();
	bezManager.draw();
	src_fbo.end();

	// make bg image
	if (grabBG)
	{
		bezManager.setGuideVisible(false);

		bg_fbo.begin();
		smoothBG.begin();
		smoothBG.setUniformTexture("prevTex", bg_fbo, 1);
		src_fbo.draw(0,0);
		//src_fbo.draw(0, CAMERA_H, CAMERA_W, -CAMERA_H);
		smoothBG.end();
		bg_fbo.end();

		if (ofGetElapsedTimeMillis() - bg_start > BG_CALC_DURATION_MS)
		{
			grabBG = false;
		}
	}

	// subtract, 1ms
	shadow_fbo.begin();
	subtract.begin();
	subtract.setUniformTexture("bgTex", bg_fbo, 1);
	subtract.setUniform1f("threshold", subtract_threshold);
	src_fbo.draw(0, 0);
	subtract.end();
	shadow_fbo.end();

	// blur
	blur_fbo.begin();
	gaussianBlur.begin();
	gaussianBlur.setUniform1i("size", blur_size);
	gaussianBlur.setUniform1f("sigma", 1.0);
	shadow_fbo.draw(0, 0);
	gaussianBlur.end();
	blur_fbo.end();

	// binarize
	shadow_fbo.begin();
	binarize.begin();
	binarize.setUniform1f("threshold", binarize_threshold);
	blur_fbo.draw(0, 0);
	binarize.end();
	shadow_fbo.end();

	// blur
	blur_fbo.begin();
	gaussianBlur.begin();
	gaussianBlur.setUniform1i("size", blur_size);
	gaussianBlur.setUniform1f("sigma", 1.0);
	shadow_fbo.draw(0, 0);
	gaussianBlur.end();
	blur_fbo.end();

	// boald image
	shadow_fbo.begin();
	boald.begin();
	boald.setUniform1i("size", boald_size);
	blur_fbo.draw(0, 0);
	boald.end();
	shadow_fbo.end();

	// update heatmap
	heatmap_fbo.begin();
	heatmap.begin();
	heatmap.setUniformTexture("preTex", prev_fbo.getTexture(), 1);
	heatmap.setUniform1f("increaseParam", increaseParam);
	heatmap.setUniform1f("decreaseParam", decreaseParam);
	shadow_fbo.draw(0, 0);
	heatmap.end();
	heatmap_fbo.end();

	// copy buffer
	prev_fbo.begin();
	heatmap_fbo.draw(0, 0);
	prev_fbo.end();

	// adjust position and scaling
	spout_fbo1.begin();
	ofPushMatrix();
	ofTranslate(offset_pos.get());
	ofScale(offset_scale.get());
	shadow_fbo.draw(0, CAMERA_H, CAMERA_W, -CAMERA_H);
	ofPopMatrix();
	spout_fbo1.end();

	spout_fbo2.begin();
	ofPushMatrix();
	ofTranslate(offset_pos.get());
	ofScale(offset_scale.get());
	//heatmap_fbo.draw(0, 0);
	heatmap_fbo.draw(0, CAMERA_H, CAMERA_W, -CAMERA_H);
	ofPopMatrix();
	spout_fbo2.end();

	// send image
	spout1.send(spout_fbo1.getTexture());	// 3ms
	spout2.send(spout_fbo2.getTexture());	// 3ms

	ofSetWindowTitle(ofToString(ofGetFrameRate()));
}

//--------------------------------------------------------------
void ofApp::draw() {

	ofBackground(0);

	int w = CAMERA_W / 2;
	int h = CAMERA_H / 2;

	switch (display_mode)
	{
	case DISPLAY_MODE_CALIBRATION:
		// src image
		pgr_camera->getOfImage().draw(0, h, w, -h);

		// bg image
		bg_fbo.draw(0, h, w, h);

		// shadow image(spout 1)
		shadow_fbo.draw(w, 0, w, h);

		// heatmap image(spout 2)
		heatmap_fbo.draw(w, h, w, h);
		break;

	case DISPLAY_MODE_SHADOW:
		shadow_fbo.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
		break;

	case DISPLAY_MODE_HEATMAP:
		heatmap_fbo.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
		break;

	default:
		break;
	}


	//destImage2.draw(0, CAMERA_H / 2, CAMERA_W / 2, CAMERA_H / 2);
	//bgImage.draw(CAMERA_W / 2, 0, CAMERA_W / 2, CAMERA_H / 2);
	//spout_fbo.draw(CAMERA_W / 2, CAMERA_H / 2, CAMERA_W / 2, CAMERA_H / 2);

	// mapper image for setting
	if (showMapper)
	{
		bezManager.draw();
	}

	if (showGui)
	{
		gui.draw();
	}

	//ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()), 10, 15);
}

//--------------------------------------------------------------
void ofApp::exit() {

	delete pgr_camera;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	bezManager.keyPressed(key);

	switch (key)
	{
	case 'b':
		grabBG = true;
		bg_start = ofGetElapsedTimeMillis();
		bg_fbo.begin();
		ofBackground(0);
		bg_fbo.end();
		break;

	case 'f':
		ofToggleFullscreen();
		break;

	case 'g':
		showGui = !showGui;
		break;

	case 's':
		bezManager.saveSettings();
		break;

	case 'r':
		reloadShader();
		break;

	case '1':
		display_mode = DISPLAY_MODE_CALIBRATION;
		break;

	case '2':
		display_mode = DISPLAY_MODE_SHADOW;
		break;

	case '3':
		display_mode = DISPLAY_MODE_HEATMAP;
		break;

	case OF_KEY_RETURN:
		bezManager.toggleGuideVisible();
		break;

	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

	bezManager.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	bezManager.mousePressed(x, y, button);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

//--------------------------------------------------------------
void ofApp::reloadShader()
{
	subtract.load("", "shaders/subtract.frag");
	bilateral.load("", "shaders/bilateral.frag");
	binarize.load("", "shaders/binarize.frag");
	smoothBG.load("", "shaders/smooth.frag");
	heatmap.load("", "shaders/heatmap.frag");
	gaussianBlur.load("", "shaders/gaussianBlur.frag");
	boald.load("", "shaders/boald.frag");
	cout << "shader loaded." << endl;
}

//--------------------------------------------------------------
void ofApp::saveSettings()
{
	////screen settings
	//ofxXmlSettings _xml = ofxXmlSettings();
	////int lastTagNumber;
	//// screen
	////for (int m = 0; m < bezierList.size(); m++) {
	//	_xml.addTag("APP");
	//	_xml.setValue("SCREEN:IS_BEZIER", (int)bezierList[m].anchorControl, lastTagNumber);
	//	_xml.setValue("SCREEN:RESOLUTION", (int)bezierList[m].prev_gridRes, lastTagNumber);
	//	if (_xml.pushTag("SCREEN", lastTagNumber)) {
	//		for (int c = 0; c < 4; c++) {
	//			int tagNum = _xml.addTag("CORNER");
	//			_xml.setValue("CORNER:X", (int)bezierList[m].corners[c].x, tagNum);
	//			_xml.setValue("CORNER:Y", (int)bezierList[m].corners[c].y, tagNum);
	//		}
	//		for (int a = 0; a < 8; a++) {
	//			int tagNum = _xml.addTag("ANCHOR");
	//			_xml.setValue("ANCHOR:X", (int)bezierList[m].anchors[a].x, tagNum);
	//			_xml.setValue("ANCHOR:Y", (int)bezierList[m].anchors[a].y, tagNum);
	//		}
	//		_xml.popTag();
	//	}
	////}

	//_xml.saveFile(APPLICATION_SETTINGS_PASS);
}

//--------------------------------------------------------------
void ofApp::loadSettings()
{

	//_xml.pushTag("SCREEN", m);
	//ofLog(OF_LOG_NOTICE, " SCREEN:" + ofToString(m));

	//pgr_camera->setup(4, 52, CAMERA_W, CAMERA_H, FlyCapture2::MODE_0, FlyCapture2::PIXEL_FORMAT_MONO8); //PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8



	//bezierList[m].anchorControl = _xml.getValue("IS_BEZIER", 0);
	//bezierList[m].gridRes = _xml.getValue("RESOLUTION", 10);
	//bezierList[m].prev_gridRes = bezierList[m].gridRes;
	//ofLog(OF_LOG_NOTICE, " IS_BEZIER:" + ofToString(bezierList[m].anchorControl));
	//ofLog(OF_LOG_NOTICE, " RESOLUTION:" + ofToString(bezierList[m].prev_gridRes));
	//int numCornerTags = _xml.getNumTags("CORNER");
	//for (int i = 0; i < numCornerTags; i++) {
	//	int x = _xml.getValue("CORNER:X", 0, i);
	//	int y = _xml.getValue("CORNER:Y", 0, i);
	//	bezierList[m].corners[i].x = x;
	//	bezierList[m].corners[i].y = y;
	//	ofLog(OF_LOG_NOTICE, " CORNER:" + ofToString(i) + " x:" + ofToString(x) + " y:" + ofToString(y));
	//}
	//int numAnchorTags = _xml.getNumTags("ANCHOR");
	//for (int i = 0; i < numAnchorTags; i++) {
	//	int x = _xml.getValue("ANCHOR:X", 0, i);
	//	int y = _xml.getValue("ANCHOR:Y", 0, i);
	//	bezierList[m].anchors[i].x = x;
	//	bezierList[m].anchors[i].y = y;
	//	ofLog(OF_LOG_NOTICE, " ANCHOR:" + ofToString(i) + " x:" + ofToString(x) + " y:" + ofToString(y));
	//}
	//_xml.popTag();
}

//--------------------------------------------------------------
void ofApp::initCamera()
{
	ofxXmlSettings _xml;

	if (_xml.loadFile("BezierWarpManager_settings.xml")) {
		cout << "[" + APPLICATION_SETTINGS_PASS + "] loaded!" << endl;
	}
	else {
		cout << "unable to load [" + APPLICATION_SETTINGS_PASS + "] check data/ folder" << endl;
		//return;
	}

	int numMovieTags = _xml.getNumTags("APP");

	string mode = _xml.getValue("MODE", "MODE_0");
	string format = _xml.getValue("FORMAT", "PIXEL_FORMAT_MONO8");
	_xml.getNumTags("CORNER");
	int offset_x = _xml.getValue("X", 0);
	int offset_y = _xml.getValue("Y", 0);

	pgr_camera = new ofxFlyCap2();
	pgr_camera->setup(80, 236, CAMERA_W, CAMERA_H, FlyCapture2::MODE_0, FlyCapture2::PIXEL_FORMAT_MONO8); //PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8

	float fps = CAMERA_FRAMERATE;
	pgr_camera->setShutterSpeed(1000 / fps);
	pgr_camera->setFrameRate(fps);
	pgr_camera->setBrightness(7.422);

}