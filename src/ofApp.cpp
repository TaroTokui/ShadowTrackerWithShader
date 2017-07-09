#include "ofApp.h"

using namespace cv;

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetVerticalSync(false);
	ofSetFrameRate(CAMERA_FRAMERATE);

	pgr_camera = new ofxFlyCap2();
	pgr_camera->setup(0, 0, CAMERA_W, CAMERA_H, FlyCapture2::MODE_0, FlyCapture2::PIXEL_FORMAT_MONO8); //PIXEL_FORMAT_RAW8, PIXEL_FORMAT_MONO8

	float fps = CAMERA_FRAMERATE;
	pgr_camera->setShutterSpeed(1000 / fps);
	pgr_camera->setFrameRate(fps);

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
	gui_params.add(increaseParam.set("increase", 0.02, 0.0, 0.2));
	gui_params.add(decreaseParam.set("decrease", 0.1, 0.0, 1.0));
	gui.setup(gui_params);
	gui.loadFromFile("settings.xml");
	showGui = true;

	// load shaders
	reloadShader();

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
	gaussianBlur.setUniform1i("size", 3);
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

	// draw to spout fbo
	//shadow_fbo.begin();
	//ofBackground(0);
	////bezManager.draw();
	//shadow_fbo.end();

	// send image
	spout1.send(shadow_fbo.getTexture());	// 3ms
	spout2.send(heatmap_fbo.getTexture());	// 3ms

}

//--------------------------------------------------------------
void ofApp::draw() {

	ofBackground(0);

	int w = CAMERA_W / 2;
	int h = CAMERA_H / 2;

	// src image
	pgr_camera->getOfImage().draw(0, 0, w, h);
	
	// bg image
	bg_fbo.draw(0, h, w, h);
	
	// shadow image(spout 1)
	shadow_fbo.draw(w, 0, w, h);

	// heatmap image(spout 2)
	heatmap_fbo.draw(w, h, w, h);


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

	ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()), 10, 15);
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
	cout << "shader loaded." << endl;
}