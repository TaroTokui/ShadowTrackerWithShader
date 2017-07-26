#pragma once

#include "ofMain.h"
#include "ofxFlyCap2.h"
#include "ofxGui.h"
#include "ofxSpout.h"
#include "ofxBezierWarpManager.h"
#include "ofxXmlSettings.h"

static const int CAMERA_W = 1920;
static const int CAMERA_H = 1080;
static const int CAMERA_FRAMERATE = 60;
static const int BG_CALC_DURATION_MS = 1000;
static const string APPLICATION_SETTINGS_PASS = "app_settings.xml";

enum DISPLAY_MODE {
	DISPLAY_MODE_CALIBRATION, 
	DISPLAY_MODE_SHADOW,
	DISPLAY_MODE_HEATMAP,
};

class ofApp : public ofBaseApp {

public:

	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:

	void reloadShader();
	void saveSettings();
	void loadSettings();
	void initCamera();


	// point grey camera
	ofxFlyCap2 *pgr_camera;

	// cv variables
	cv::Mat src_img;
	cv::Mat bg_img;
	cv::Mat diff_img;
	cv::Mat blur_img;
	cv::Mat binarized_img;
	cv::Mat dest_img1;
	cv::Mat dest_img2;
	bool grabBG;

	// GUI variables
	ofxPanel gui;
	ofParameterGroup gui_params;
	ofParameter<float> binarize_threshold;
	ofParameter<float> subtract_threshold;
	ofParameter<int> blur_size;
	ofParameter<int> boald_size;
	ofParameter<bool> showMapper;
	ofParameter<float> increaseParam;
	ofParameter<float> decreaseParam;
	ofParameter<ofVec2f> offset_pos;
	ofParameter<ofVec2f> offset_scale;
	bool showGui;

	// ofImage
	ofImage diffImage;
	ofImage bgImage;
	ofImage destImage1;
	ofImage destImage2;

	// spout
	ofxSpout::Sender spout1;
	ofxSpout::Sender spout2;
	ofFbo shadow_fbo, heatmap_fbo, src_fbo, blur_fbo, prev_fbo;
	ofFbo spout_fbo1, spout_fbo2;

	// mapper
	ofxBezierWarpManager bezManager;
	ofFbo mapper_fbo;
	ofFbo bg_fbo;
	//ofFbo spout_fbo;

	// shader
	ofShader subtract;
	ofShader bilateral;
	ofShader binarize;
	ofShader heatmap;
	ofShader smoothBG;
	ofShader gaussianBlur;
	ofShader boald;

	// timer
	unsigned long long bg_start;

	// display mode
	DISPLAY_MODE display_mode;
};
