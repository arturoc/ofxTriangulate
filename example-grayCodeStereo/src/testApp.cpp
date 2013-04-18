#include "testApp.h"

/**
\brief An example which uses 2 cameras and a projector to scan a scene
*/

//--------------------------------------------------------------
void testApp::setup(){
	ofSetVerticalSync(true);
	string path = "";
	
	// Setup the payload (i.e. projector resolution)
	payload.init(1024, 768);

	// Initialise the decoder with projector's encoding
	decoder1.init(payload);

	// Load the graycode images from camera 1 into decoder1
	ofLogNotice() << "Loading images for camera 1";
	for(int i=0;i<payload.getFrameCount();i++){
		if (i > 0) {
			cout << ", ";
		}
		cout << i;
		imgColor.loadImage(path+"capture1/"+ofToString(i)+".png");
		decoder1 << imgColor;
	}
	cout << endl;

	// Load the mean brightness in camera 1 during
	//  the scan into texture mean1 (i.e. this should
	//  be a photo from camera 1)
	mean1.allocate(decoder1.getDataSet().getMean());
	mean1.loadData(decoder1.getDataSet().getMean());

	// Load the mean brightness per projector pixel as seen in camera 1 into inverseMean1
	//  i.e. this should be a photo as seen from the projector
	inverseMean1.allocate(decoder1.getDataSet().getMeanInverse());
	inverseMean1.loadData(decoder1.getDataSet().getMeanInverse());

	// Load the graycode images from camera 2 into decoder2
	decoder2.init(payload);
	for(int i=0;i<payload.getFrameCount();i++){
		if (i > 0) {
			cout << ", ";
		}
		cout << i;
		imgColor.loadImage(path+"capture2/"+ofToString(i)+".png");
		decoder2 << imgColor;
	}
	cout << endl;

	// Load the mean brightness in camera 2 during
	//  the scan into texture mean2 (i.e. this should
	//  be a photo from camera 2)
	mean2.allocate(decoder2.getDataSet().getMean());
	mean2.loadData(decoder2.getDataSet().getMean());

	// Load the mean brightness per projector pixel as seen in camera 2 into inverseMean2
	//  i.e. this should be a photo as seen from the projector
	inverseMean2.allocate(decoder2.getDataSet().getMeanInverse());
	inverseMean2.loadData(decoder2.getDataSet().getMeanInverse());

	ofEnableAlphaBlending();
	ofBackground(255);

	// Allocate the depth map with the projector's width and height.
	depthMap.allocate(payload.getWidth(),payload.getHeight(),OF_IMAGE_GRAYSCALE);

	// Blank all the pixels in the depth map.
	depthMap.getPixelsRef().set(0);

	// Use the first image from the scan as the colour image.
	// Ideally you'd take a seperate image when the lights are on and use that
	//  (e.g. when the projector is showing all white).
	imgColor.loadImage(path+"capture2/0.png");

    mesh.setMode(OF_PRIMITIVE_POINTS);

	// Initialise the properties of the camera.
	// In this test, we use 2 identical cameras
	//  and we estimate that they both have
	//  identical intrinsics.
	float zNear = -1;
	float zFar = 1000;
	float cx = 6.7843858886797614e+02;
	float cy = 3.4517466503352483e+02;
	float fx = 1.0719532879358244e+03;
	float fy = 1.0703376726883023e+03;
	float w = 1280;
	float h = 720;

	// Setup the cameras
	ofxRay::Camera camera1(ofVec2f(fx,fy),ofVec2f(cx,cy),zNear,zFar,w,h);
	ofxRay::Camera camera2(ofVec2f(fx,fy),ofVec2f(cx,cy),zNear,zFar,w,h);

	// Set the camera locations in the scene.
	// Whichever units we use here will be units
	//  which the final scan data will also have.
	// (mm, cm, meters, inches, etc).
	camera1.move(-80,0,0);
    camera2.move(80,0,0);

	// Loop through projector pixels (by looping through getDataInverse()
	ofLogNotice() << "Triangulating 3D positions..";
	for(int i=0;i<decoder1.getDataSet().getDataInverse().size();i++){
		int d1 = decoder1.getDataSet().getDataInverse()[i];
		int d2 = decoder2.getDataSet().getDataInverse()[i];
		int x = i%decoder1.getDataSet().getDataInverse().getWidth();
		int y = i/decoder1.getDataSet().getDataInverse().getWidth();

		if(decoder1.getDataSet().getActive()[d1] && decoder2.getDataSet().getActive()[d2]){
			int d2_x = d2%decoder1.getDataSet().getData().getWidth();
			int d2_y = d2/decoder2.getDataSet().getData().getWidth();
			ofVec3f world = ofxTriangulate::Triangulate(d1,d2,camera1,camera2);
			mesh.addVertex(world);
			mesh.addTexCoord(ofVec2f(d2_x,d2_y));//ofFloatColor(imgColor2.getColor(d2_x,d2_y)));
			depthMap.getPixelsRef()[x+y*depthMap.getWidth()] = (world.z*255)/zFar;
		}
	}
	depthMap.update();

	// Save all results
	mesh.save("mesh.ply");
	depthMap.saveImage("depthMap.png");
	imgColor.saveImage("imgColor.png");

	// Point camera at center of mesh
	ofVec3f centroid=mesh.getCentroid();
	camera.setTarget(centroid);

	//glEnable(GL_POINT_SMOOTH);
	//glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
	glPointSize(2);
	glEnable(GL_DEPTH_TEST);

	mode = ThreeD;
}

//--------------------------------------------------------------
void testApp::update(){
}

//--------------------------------------------------------------
void testApp::draw(){
	int height = mean1.getHeight()*640/mean1.getWidth();
	switch(mode){
	case Mean:
		mean1.draw(0,0,640,height);
		mean2.draw(640,0,640,height);
		ofSetColor(255,180);
		inverseMean1.draw(0,height,640,480);
		ofSetColor(255,180);
		inverseMean2.draw(0,height,640,480);
		break;
	case DepthMap:
		depthMap.draw(0,0);
		break;
	case ThreeD:
		camera.begin();
		imgColor.bind();
		mesh.draw();
		imgColor.unbind();
		camera.end();
		break;
	}
}



//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if(key==' '){
		mode= (Mode)(int(mode)+ 1);
		mode= (Mode)(int(mode)%3);
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
