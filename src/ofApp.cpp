#include "ofApp.h"
#include "ofxGstRTPUtils.h"
#include "ofxNice.h"
#include "ofConstants.h"

#define STRINGIFY(A) #A

//--------------------------------------------------------------
void ofApp::setup(){
    
    shaderLoad();
    
    ofSetLogLevel(ofxGstRTPClient::LOG_NAME,OF_LOG_VERBOSE);
#ifdef BUNDLED
    ofxGStreamerSetBundleEnvironment();
#endif
    
    //  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    contours=true;
    updateVisualMode(NORMAL);
    
    newLocalVideo=false;
    newLocalContour=false;
    newRemoteContour=false;
    newRemoteVideo=false;
    newMessage=false;
    
    alphaFade=10;
    
    // NOTE: 720p FaceTime HD (retina display mac book pro) is 1280x720
    //camWidth 		= 640;//640;	// try to grab at this si ze.
	//camHeight 	= 480;//360;
    
	if(settings_XMPP.load("settings_XMPP.xml")){
		string server = settings_XMPP.getValue("server");
		string user = settings_XMPP.getValue("user");
		string pwd = settings_XMPP.getValue("pwd");
        camWidth = settings_XMPP.getIntValue("camwidth");
        camHeight= settings_XMPP.getIntValue("camheight");
        fps= settings_XMPP.getIntValue("fps");
        
        //  int latency = settings_XMPP.getIntValue("latency");
        
		//ofxNiceEnableDebug();
        rtp.setup();// client latency default 200, but we will load the rtp.parameters from the XML file (client latency & drop / server bitrate)
		rtp.setStunServer("132.177.123.6");
		rtp.getXMPP().setCapabilities("eTango");
		rtp.connectXMPP(server,user,pwd);
		rtp.addSendVideoChannel(camWidth,camHeight,fps); // RGB CHANNEL ONLY!!!!
		rtp.addSendOscChannel();
		//rtp.addSendAudioChannel(); // no audio channel, as we are going to synch iTunes instead
        
		callingState = Disconnected;
	} else {
        cout << "COULD NOT LOAD settings_XMPP.xml" << endl; ofExit();
    }
	
    // =============================== Gui for control of RTP protocol ===============================
    // The following is only done in order to create the settings_XMPP.xml file the first time:
    //
    //    settings_XMPP.serialize(rtp.parameters);
    //    settings_XMPP.save("settings_XMPP.xml");
    
    settings_XMPP.deserialize(rtp.parameters);
    
    gui_RTP.setup("MENU RTP","settings_XMPP.xml",ofGetWidth()-250,10);
	gui_RTP.add(fpsParam.set("FPS", 29, 1, 40));
    gui_RTP.add(rtp.parameters);
    
    // =============================== Gui for control of visualization ===============================
    
    // we add this listener before setting up so the initial modes will be automatically set after loading the XML file:
	fullScreen.addListener(this, &ofApp::screenMode);
    contours.addListener(this, &ofApp::sendContourPlease);
    noVideo.addListener(this, &ofApp::changeVideoMode_noVideo);
    normalVideo.addListener(this, &ofApp::changeVideoMode_normalVideo);
    bw.addListener(this, &ofApp::changeVideoMode_bw);
    sepia.addListener(this, &ofApp::changeVideoMode_sepia);
    shadow.addListener(this, &ofApp::changeVideoMode_shadow);
    shaderView.addListener(this, &ofApp::changeVideoMode_shaderView);
    
    parameters_VIEW.setName("VIEW SETTINGS");
    parameters_VIEW.add(noVideo.set("No Video", true));
    parameters_VIEW.add(normalVideo.set("Normal Video", false));
    parameters_VIEW.add(bw.set("Black & White",false));
    parameters_VIEW.add(sepia.set("Sepia",false));
    parameters_VIEW.add(shadow.set("Shadows",false));
    parameters_VIEW.add(shaderView.set("Shader",false));
    
    //parameters_VIEW.add(saturation.set( "Saturation", 128, 0, 255));
    parameters_VIEW.add(contours.set("Contours",false));
    parameters_VIEW.add(thresholdGray.set("Threshold Contours", 140, 0, 255));
    parameters_VIEW.add(invertThreshold.set("Invert",true));
	
    parameters_VIEW.add(fullScreen.set("Windowed Screen",false));
    //parameters_VIEW.add(showOwnVideo.set("Show own video",false));
    parameters_VIEW.add(showMessages.set("Show messages",false));
    parameters_VIEW.add(showTweets.set("Show tweets",false));
    parameters_VIEW.add(showSong.set("Show song playing",true));
	
    // The following is only done in order to create the XML file the first time:
    //settings_VIEW.serialize(parameters_VIEW);
    //settings_VIEW.save("settings_VIEW.xml");
    
    settings_VIEW.load("settings_VIEW.xml");
    settings_VIEW.deserialize(parameters_VIEW);
    
    //ofxPanel * setup(string collectionName="", string filename="settings.xml", float x = 10, float y = 10);
	//ofxPanel * setup(const ofParameterGroup & parameters, string filename="settings.xml", float x = 10, float y = 10);
    //gui_VIEW.setup("MENU VIEW", "setting_VIEW.xml", ofGetWidth()-250,10);
    gui_VIEW.setup(parameters_VIEW, "setting_VIEW.xml", ofGetWidth()-250,10);
    
    // Allocation of space for local and remote images:
	textureVideoRemote.allocate(camWidth,camHeight,GL_RGB8);
	textureVideoLocal.allocate(camWidth,camHeight,GL_RGB8);
    bwTexture.allocate(camWidth,camHeight, GL_LUMINANCE);
    bwTextureRemote.allocate(camWidth,camHeight, GL_LUMINANCE);
    
    // Allocation of space for image processing:
    // myImageGray.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
    // myImageColor.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    //myImage.changeTypeOfPixels(ofPixels_<PixelType> &pix, ofImageType type);
    for (int i=0; i<2; i++) {
        colorImage[i].allocate(camWidth, camHeight);
        bwImage[i].allocate(camWidth,camHeight);
        sepiaImage[i].allocate(camWidth, camHeight);
        shadowImage[i].allocate(camWidth, camHeight);
    }
    
    // Video camera initialization (can be a kinect in the future):
	vector<ofVideoDevice> devices = vidGrabber.listDevices();
    cout << "CAMERA DEVICES: *********"<< endl;
    for(int i = 0; i < devices.size(); i++){
		cout << devices[i].id << ": " << devices[i].deviceName;
        if( devices[i].bAvailable ){
            cout << endl;
        }else{
            cout << " - unavailable " << endl;
        }
	}
    cout << "*************************"<< endl;
    
	vidGrabber.setDeviceID(0);
	vidGrabber.setDesiredFrameRate(fps);
	vidGrabber.setVerbose(true);
	vidGrabber.initGrabber(camWidth,camHeight);
    
    //ofRectangle viewPortFull, viewPortWindow;
    float factor;
    if (ofGetWidth()/ofGetHeight()>camWidth/camHeight) {
        factor=ofGetHeight()/camHeight;
        viewPortFull=ofRectangle(0.5*(ofGetWidth()-camWidth*factor),0,camWidth*factor, ofGetHeight());
    } else {
        factor=ofGetWidth()/camWidth;
        viewPortFull=ofRectangle(0,0.5*(ofGetHeight()-camHeight*factor),ofGetWidth(), camHeight*factor);
    }
    
	//camera.setVFlip(true);
	//camera.setTarget(ofVec3f(0.0,0.0,-1000.0));
    
	calling = -1;
    
    ofBackground(0,0,0);
    ofSetBackgroundAuto(true);
    
	guiState = HIDDEN;
    callingState = Disconnected;
    
    // LISTENER CALLBACKS:
    ofAddListener(rtp.getXMPP().newMessage,this,&ofApp::onNewMessage); // IM message (not OSC)
	ofAddListener(rtp.callReceived,this,&ofApp::onCallReceived);
	ofAddListener(rtp.callFinished,this,&ofApp::onCallFinished);
	ofAddListener(rtp.callAccepted,this,&ofApp::onCallAccepted);
    
    
	ring.loadSound("ring.wav",false);
	lastRing = 0;
    
    // Threaded function to check iTunes state, using system commands:
    iTunesChecker.setCheckPeriod(1000); // in ms
    iTunesChecker.startThreadChecker();
    
    myFont.loadFont("Batang.ttf", 15, true, true, true);
    //myFont.setGlobalDpi(200);
    myFontTweet.loadFont("Batang.ttf", 55, true, true, true);
    
    bFirst  = true;
	typeStr.clear();// = "";
    cursorMode=true;
    cursorTimer=ofGetElapsedTimeMillis();
    
    
    // ------ TWITTER:
    
    twitterClient.setDiskCache(true);
    twitterClient.setAutoLoadImages(true, false); // Loads images into memory as ofImage;
    
    //( THE twitter APPLICATION IS here: https://apps.twitter.com/app/7013938/show )
    
    string const CONSUMER_KEY = "7HS8aCGTwBIJTpf8S81l1Q0H4";
    string const CONSUMER_SECRET = "pg4jXKVbmnqGqI5axOnZoxFS32VtSZc5WY3YZYjkWrCGoWXZ0Z";
    
    twitterClient.authorize(CONSUMER_KEY, CONSUMER_SECRET);
    
    actualTweet = 0; // note: the latest tweet has the largest index
    newTweetText=""; oldTweetText="";
    newTweet=false;
    alphaTimerTweet=ofGetElapsedTimeMillis();
    checkTimerTweet=ofGetElapsedTimeMillis();
    numberOfTweetChecks=0;
    
}

void ofApp::updateVisualMode(VisualModes mode) {
    visualMode=mode;
    noVideo=false; normalVideo=false; bw=false; sepia=false; shadow=false; shaderView=false;
    switch(mode) {
        case NONE:
            noVideo=true; normalVideo=false; bw=false; sepia=false; shadow=false; shaderView=false;
            break;
        case NORMAL:
            noVideo=false; normalVideo=true; bw=false; sepia=false; shadow=false; shaderView=false;
            break;
        case BW:
            noVideo=false; normalVideo=false; bw=true; sepia=false; shadow=false; shaderView=false;
            break;
        case SEPIA:
            noVideo=false; normalVideo=false; bw=false; sepia=true; shadow=false; shaderView=false;
            break;
        case SHADOW:
            noVideo=false; normalVideo=false; bw=false; sepia=false; shadow=true; shaderView=false;
            break;
        case SHADER:
            noVideo=false; normalVideo=false; bw=false; sepia=false; shadow=false; shaderView=true;
            break;
        default:
            break;
            
    }
}


void ofApp::onCallReceived(string & from){
	callFrom = ofSplitString(from,"/")[0];
	callingState = ReceivingCall;
    
#ifdef ACCEPT_CALL
    rtp.acceptCall();
    callingState = InCall;
#endif
}

void ofApp::onCallAccepted(string & from){
	if(callingState == Calling){
		callingState = InCall;
	}
    // clear screen:
    //ofSetColor(0);
    //    ofRect(0,0,ofGetWidth(), ofGetHeight());
}

void ofApp::onCallFinished(ofxXMPPTerminateReason & reason){
	if(callingState==Calling){
		ofSystemAlertDialog("Call declined");
	}
	cout << "received end call" << endl;
	callingState = Disconnected;
	calling = -1;
	//rtp.setStunServer("132.177.123.6");
	//rtp.addSendVideoChannel(640,480,30);
	//rtp.addSendOscChannel();
    //rtp.addSendAudioChannel();
}

void ofApp::onNewMessage(ofxXMPPMessage & msg){
	xmppMessages.push_back(msg);
	if(xmppMessages.size()>8) xmppMessages.pop_front();
}

void ofApp::exit(){
}

void ofApp::screenMode(bool& screenmode) {
    ofToggleFullscreen();
}

void ofApp::sendContourPlease(bool& send) {
    if (send) contours=true; // is this necessary?
    ofxOscMessage msg;
    string sendWhat;
    if (send) {sendWhat="true"; receiveContour=true;}
    else {sendWhat="false"; receiveContour=false;}//remoteContour.clear();}
    msg.setAddress("//eTango/sendContour/"+sendWhat);
    GstClockTime now = rtp.getServer().getTimeStamp();
    rtp.getServer().newOscMsg(msg,now);
}

void ofApp::changeVideoMode_noVideo(bool& mode) {
    if (mode) {
        updateVisualMode(NONE);
        sendVideoPlease(NONE);
    }
}

void ofApp::changeVideoMode_normalVideo(bool& mode) {
    if (mode) {
        updateVisualMode(NORMAL);
        sendVideoPlease(NORMAL);
    }
}

void ofApp::changeVideoMode_bw(bool& mode) {
    if (mode) {
        updateVisualMode(BW);
        sendVideoPlease(BW);
    }
}

void ofApp::changeVideoMode_sepia(bool& mode) {
    if (mode) {
        updateVisualMode(SEPIA);
        sendVideoPlease(SEPIA);
    }
}

void ofApp::changeVideoMode_shadow(bool& mode) {
    if (mode) {
        updateVisualMode(SHADOW);
        sendVideoPlease(SHADOW);
    }
}

void ofApp::changeVideoMode_shaderView(bool& mode) {
    if (mode) {
        updateVisualMode(SHADER);
        sendVideoPlease(SHADER);
    }
}

void ofApp::sendVideoPlease(VisualModes mode) {
    ofxOscMessage msg;
    msg.setAddress("//eTango/visualMode");
    msg.addIntArg((int)mode); // this is possible (if we did NONE=0), but the reverse NO
    GstClockTime now = rtp.getServer().getTimeStamp();
    rtp.getServer().newOscMsg(msg,now);
}

ofxCvColorImage ofApp::convertToSepia(ofxCvColorImage& inputImage) {
    ofxCvColorImage outputImage;
    outputImage.allocate(camWidth, camHeight);
    ofPixelsRef pixels = inputImage.getPixelsRef();
    ofPixelsRef pixelsOutput = outputImage.getPixelsRef();
    
    for(int y = 0; y < inputImage.getHeight(); y++){
		for(int x = 0; x < inputImage.getWidth(); x++){
			
            ofColor color = pixels.getColor(x, y);
            // pixelsOutput.setColor(x, y, color);
            int outputRed = (int)MIN((1.0*color.r * .393) + (1.0*color.g *.769) + (1.0*color.b * .189), 255);
            int outputGreen =(int) MIN((1.0*color.r * .349) + (1.0*color.g *.686) + (1.0*color.b * .168), 255);
            int outputBlue = (int)MIN((1.0*color.r * .272) + (1.0*color.g *.534) + (1.0*color.b * .131), 255);
            pixelsOutput.setColor(x, y, ofColor(outputRed,outputGreen, outputBlue));
		}
	}
    
    //after we're done we need to update the image:
    outputImage.setFromPixels(pixelsOutput);
    // outputImage.update();
    return(outputImage);
}

//--------------------------------------------------------------
void ofApp::update(){
    
    if (showTweets) {
        if (ofGetElapsedTimeMillis() -checkTimerTweet > CHECK_TIMER_TWEET) {
            checkNewTweet();
            checkTimerTweet=ofGetElapsedTimeMillis();
            
            numberOfTweetChecks++;
            cout << " twitter check n." << numberOfTweetChecks << endl;
        }
    }
    
    GstClockTime now = rtp.getServer().getTimeStamp();
    
    //  ================== (1) UPDATE THE SERVER  ==================
    // NOTE: I can only send RGB images and contours (as OSC), but I compute all the other treated images because we may want to see what the others
    // will actually see (for the time being, this is seen in the windowed mode only).
    
    vidGrabber.update();
    newLocalVideo=vidGrabber.isFrameNew();
    
    if (newLocalVideo) {
        //  vidGrabber.update();
        fpsRGB.newFrame();
        
        // We do this always (for tests) even if the visualMode does not demand it:
        // myImageGray.setFromPixels(vidGrabber.getPixels(),camWidth, camHeight,OF_IMAGE_GRAYSCALE);
        // myImageColor.setFromPixels(vidGrabber.getPixels(),camWidth, camHeight,OF_IMAGE_COLOR);
        //myImage.changeTypeOfPixels(ofPixels_<PixelType> &pix, ofImageType type);
        
        colorImage[0].setFromPixels(vidGrabber.getPixels(), camWidth, camHeight);
        
        bwImage[0].setFromColorImage(colorImage[0]);
        bwImage[0].brightnessContrast(0.5, 0.5);
        
        shadowImage[0]=bwImage[0];
        shadowImage[0].blurHeavily();
        shadowImage[0].erode_3x3();
        shadowImage[0].dilate_3x3();
        shadowImage[0].threshold(thresholdGray, invertThreshold);
        
        sepiaImage[0]=convertToSepia(colorImage[0]);
        
        
        // Textures (why?)
        textureVideoLocal.loadData(colorImage[0].getPixelsRef());
        bwTexture.loadData(vidGrabber.getPixelsRef());
        
        //Contour:
        if(contourFinder.findContours(shadowImage[0],10*10,640*480/3,1,false,true)>0) newLocalContour=true;
        else newLocalContour=false;
        
        // (a) Send requested image to all clients (if sending mode enabled) using video channel:
        // NOTE: there seem to be a problem when I send grayscale images!! INDEED!!! the XMPP uses only COLOR images!!
        // Therefore, I will send always the RGB image (IF requested):
        
        
        switch(visualMode) {
            case NONE: // we don't send anything...
                break;
                // IN ALL THE OTHER CASES, send RGB IMAGE:
            case NORMAL:
                rtp.getServer().newFrame(colorImage[0].getPixelsRef(),now); // with time stamp!
                break;
            case BW:
                rtp.getServer().newFrame(colorImage[0].getPixelsRef(),now); // with time stamp!
                //rtp.getServer().newFrame(bwImage.getPixelsRef(),now); // with time stamp!
                break;
            case SEPIA:
                rtp.getServer().newFrame(colorImage[0].getPixelsRef(),now); // with time stamp!
                //rtp.getServer().newFrame(sepiaImage.getPixelsRef(),now); // with time stamp!
                break;
            case SHADOW:
                rtp.getServer().newFrame(colorImage[0].getPixelsRef(),now); // with time stamp!
                //rtp.getServer().newFrame(shadowImage.getPixelsRef(),now); // with time stamp!
                break;
            case SHADER:
                rtp.getServer().newFrame(colorImage[0].getPixelsRef(),now); // with time stamp!
                //rtp.getServer().newFrame(shadowImage.getPixelsRef(),now); // with time stamp!
                break;
            default:
                break;
        }
        
        // (b) Send Countour using OSC:
        if (contours&&newLocalContour) {
            ofxOscMessage msg;
            msg.setAddress("//eTango/contour");
            
            if(contourFinder.blobs.size()){
                localContour.clear();
                localContour.addVertices(contourFinder.blobs[0].pts);
                //localContour.simplify(2);
                localContour=localContour.getSmoothed(8);//localContour.size()/5);
                
                for(int i=0;i<(int)localContour.size();i++){
                    msg.addFloatArg(localContour[i].x);
                    msg.addFloatArg(localContour[i].y);
                }
            }
            rtp.getServer().newOscMsg(msg,now);
        }
    }
    
    // (c) Send itunes commands over OSC:
    if (iTunesChecker.songChange()) {
        ofxOscMessage msg;
        msg.setAddress("//eTango/itunes/playsong");
        msg.addStringArg(iTunesChecker.getCurrentTrack());
        rtp.getServer().newOscMsg(msg,now);
        
        iTunesChecker.resetSongChange();
        iTunesChecker.resetStateChange(); // we need to do this too to avoid sending a new play (resume) command
    }
    
    if (iTunesChecker.stateChange()) {
        // send command to change song to other computer:
        ofxOscMessage msg;
        msg.setAddress("//eTango/itunes/"+iTunesChecker.getCurrentState());
        // ATTENTION: the osc message is just the output of iTunes state
        rtp.getServer().newOscMsg(msg,now);
        
        iTunesChecker.resetStateChange(); // we need to do this too to avoid sending a new play (resume) command
    }
	
    // (d) Send other commands over OSC (ex: sending modes)
    // THIS IS DONE from the keyboard or GUI controls.
    
    
    
    // ================== (2) UDPATE THE CLIENT  ==================
    // NOTE: if there is a new feed (contour, video or itunes data) we will show it (even though we set the visual mode to NONE). This means
    // there was a message transmission error. This way we can send the proper command again.
    
    
    // (a) Video image:
    rtp.getClient().update();
    newRemoteVideo=rtp.getClient().isFrameNewVideo();
    if(newRemoteVideo){
        fpsClientVideo.newFrame();
        // We load on the texture... REGARDLESS of the type (color or grayscale)?
        // It MUST be color...
        // video format that we are pushing to the pipeline: (in ARTURO CODE):
		// string vcaps="video/x-raw,format=RGB,width="+ofToString(w)+ ",height="+ofToString(h)+",framerate="+ofToString(fps)+"/1";
        // This is a pity, because we will loose bandwidth sometimes (when we send black and white for instance), plus it means we need to
        // recompute the image processing on the client side!!
        
        textureVideoRemote.loadData(rtp.getClient().getPixelsVideo());
        bwTexture=textureVideoRemote;//.loadData(vidGrabber.getPixelsRef());
        
        
        colorImage[1].setFromPixels(rtp.getClient().getPixelsVideo().getPixels(), camWidth, camHeight);
        
        bwImage[1].setFromColorImage(colorImage[1]);
        bwImage[1].brightnessContrast(0.5, 0.5);
        
        shadowImage[1]=bwImage[1];
        shadowImage[1].blurHeavily();
        shadowImage[1].erode_3x3();
        shadowImage[1].dilate_3x3();
        shadowImage[1].threshold(thresholdGray, invertThreshold);
        
        sepiaImage[1]=convertToSepia(colorImage[1]);
        
        // Textures (why?)
        textureVideoRemote.loadData(colorImage[1].getPixelsRef());
        bwTextureRemote.loadData(bwImage[1].getPixelsRef());
        
        
    }
    
    // (b) OSC:
    newRemoteContour=false;
    if(rtp.getClient().isFrameNewOsc()){
        ofxOscMessage msg = rtp.getClient().getOscMessage();
        //cout << "OSC MESSAGE ADDRESS: " << msg.getAddress() << endl;
        // (b.1) check if it is a contour:
        if (msg.getAddress() == "//eTango/contour") {
            newRemoteContour=true;
			remoteContour.clear();
			for(int i=0;i<msg.getNumArgs();i+=2){
				remoteContour.addVertex(msg.getArgAsFloat(i),msg.getArgAsFloat(i+1));
            }
        }
        // (b.1) check if it is an iTunes command:
        else if (msg.getAddress() == "//eTango/itunes/playsong") {
            string nameSong=msg.getArgAsString(0); // the single argument is a string
            iTunesChecker.playSong(nameSong);
            // string cmd = "../../../data/itunes2.sh playsong "+nameSong;
            // system(cmd.c_str());
        }
        else if (msg.getAddress() == "//eTango/itunes/paused") {
            // string nameSong=msg.getArgAsString(0); // the single argument is a string
            iTunesChecker.pause();
            //string cmd = "../../../data/itunes2.sh pause";
            //system(cmd.c_str());
        }
        else if (msg.getAddress() == "//eTango/itunes/playing") {
            // string nameSong=msg.getArgAsString(0); // the single argument is a string
            iTunesChecker.resume();
            //string cmd = "../../../data/itunes2.sh play";
            //system(cmd.c_str());
        }
        else if (msg.getAddress() == "//eTango/itunes/stopped") {
            // string nameSong=msg.getArgAsString(0); // the single argument is a string
            iTunesChecker.stop();
            //string cmd = "../../../data/itunes2.sh stop";
            //system(cmd.c_str());
        }
        //(b.2) Check if it's a change of sending modes:
        else if (msg.getAddress() == "//eTango/visualMode") {
            cout << "received visual mode change command" << endl;
            if (msg.getNumArgs()==1) {
                // visualMode=msg.getArgAsInt32(0); // ARG!! "An enumerator can be promoted to an integer value. However, converting an integer to an
                //enumerator requires an explicit cast, and the results are not defined."
                switch(msg.getArgAsInt32(0)) {
                    case 0:
                        updateVisualMode(NONE);
                        break;
                    case 1:
                        updateVisualMode(NORMAL);
                        break;
                    case 2:
                        updateVisualMode(BW);
                        break;
                    case 3:
                        updateVisualMode(SEPIA);
                        break;
                    case 4:
                        updateVisualMode(SHADOW);
                        break;
                    case 5:
                        updateVisualMode(SHADER);
                        break;
                    default:
                        cout << "Unknown visual mode" << endl;
                        break;
                        
                }
            }// otherwise there is an error
        }
        
        // (b.3) Contours?
        else if (msg.getAddress() == "//eTango/sendContour/true") {
            contours=true;
        }
        else if (msg.getAddress() == "//eTango/sendContour/false") {
            contours=false;
        }
        
        //(b.4) Check if it is a text message (NOTE: this is a hack because I wanted to use ofxXMPPMessage but it does not work?
        else if (msg.getAddress() == "//eTango/message") {
            string remoteMessage=msg.getArgAsString(0); // the single argument is a string
            oscMessage.push_back("OTHER: "+remoteMessage);
        }
        
    }
    
	/*if(calling!=-1){
     if(rtp.getXMPP().getJingleState()==ofxXMPP::Disconnected){
     calling = -1;
     }
     }*/
    
	if(callingState==ReceivingCall || callingState==Calling){
		unsigned long long now = ofGetElapsedTimeMillis();
		if(now - lastRing>2500){
			lastRing = now;
			ring.play();
		}
	}
    
    // Get the list of friends currently connected:
    friends.clear();
    friends = rtp.getXMPP().getFriendsWithCapability("eTango");
    
}


void ofApp::drawVideo(ofTexture image, ofRectangle viewPort) {
    image.draw(viewPort);
}

void ofApp::drawContour(ofPolyline contour, ofRectangle viewPort)
{
    // We have to adjust the polyline to fit the viewport:
    ofPolyline auxContour;
    ofVec2f recenter(viewPort.x, viewPort.y);
    for(int i=0;i<(int)contour.size();i++){
        auxContour.addVertex(recenter.x+contour[i].x*viewPort.width/camWidth,recenter.y+contour[i].y*viewPort.height/camHeight);
    }
    auxContour.draw();
}

void ofApp::drawCallNotification() {
    ofSetColor(30,30,30,170);
    ofRect(0,0,ofGetWidth(),ofGetHeight());
    ofSetColor(255,255,255);
    ofRectangle popup(ofGetWidth()*.5-200,ofGetHeight()*.5-100,400,200);
    ofRect(popup);
    ofSetColor(0);
    myDrawText("Receiving call from " + callFrom,popup.x+30,popup.y+30);
    
    ok.set(popup.x+popup.getWidth()*.25-50,popup.getCenter().y+20,100,30);
    cancel.set(popup.x+popup.getWidth()*.75-50,popup.getCenter().y+20,100,30);
    ofRect(ok);
    ofRect(cancel);
    
    ofSetColor(255);
    myDrawText("Ok",ok.x+30,ok.y+20);
    myDrawText("Decline",cancel.x+30,cancel.y+20);
}

// This "wrapping" will facilitate changing drawing methods (bitmap, vector, etc):
void ofApp::myDrawText(string textString, float x, float y) {
    
    myFont.drawString(textString, x, y);
    
    // ofDrawBitmapString(textString, x, y);
    
    //ofFill();
    //myFont.drawStringAsShapes(textString, x, y);
    
    //   ofNoFill();
    //  myFont.drawStringAsShapes(textString, x, y);
    
    
}


void ofApp::drawFriendsConnected(ofRectangle viewPort) {
    // List of all friends currently connected:
	ofSetColor(255);
	//ofRect(ofGetWidth()-300,0,300,ofGetHeight());
    ofRect(viewPort);
    
    for(size_t i=0;i<friends.size();i++){
        ofSetColor(0);
        if(calling==i){
            if(callingState == Calling){
                ofSetColor(ofMap(sin(ofGetElapsedTimef()*2),-1,1,50,127));
            }else if(callingState==InCall){
                ofSetColor(127);
            }
            ofRect(viewPort.x, viewPort.y+calling*20+5, viewPort.width, 20);// ofGetWidth()-300,calling*20+5,300,20);
            ofSetColor(255);
        }else{
            if(callingState==InCall && friends[i].userName==callFrom){
                ofSetColor(127);
                //ofRect(ofGetWidth()-300,i*20+5,300,20);
                ofRect(viewPort.x, viewPort.y+i*20+5, viewPort.width, 20);
                ofSetColor(255);
            }
        }
        //myDrawText(friends[i].userName,ofGetWidth()-250,20+20*i);
        myDrawText(friends[i].userName,viewPort.x, viewPort.y+20+20*i);
        if(friends[i].show==ofxXMPPShowAvailable){
            ofSetColor(ofColor::green);
        }else{
            ofSetColor(ofColor::orange);
        }
        //ofCircle(ofGetWidth()-270,20+20*i-5,3);
        ofCircle(viewPort.x+30, viewPort.y+20+20*i-5,3);
    }
}

void ofApp::drawXmppMessages(ofRectangle viewPort, int maxMessages) {
    size_t i=friends.size();
    size_t j=0;
    
    for (;j<xmppMessages.size();j++){
        myDrawText(ofSplitString(xmppMessages[j].from,"/")[0] +":\n" + xmppMessages[j].body,ofGetWidth()-280,20+i*20+j*30);
    }
    
    if(calling>=0 && calling<(int)friends.size()){
        if(friends[calling].chatState==ofxXMPPChatStateComposing){
            myDrawText(friends[calling].userName + ": ...", ofGetWidth()-280, 20 + i*20 + j*30);
        }
    }
}

void ofApp::drawOscMessages(ofRectangle viewPort, int maxMessages) {
    
    ofPushStyle();
    //ofFill();
    //ofSetColor(255,255,255,50);
    //ofRect(viewPort);
    
    if (oscMessage.size()>0) {
        int maxMessagesHere=maxMessages;
        if (maxMessages>oscMessage.size()) maxMessagesHere=oscMessage.size();
        size_t j=0;
        for (int i=oscMessage.size()-1; i>=((int)oscMessage.size()-maxMessagesHere); i=i-1) {
            int alpha=ofMap(j, 0, maxMessages, 255, 0);
            ofSetColor(255, alpha);
            myDrawText(oscMessage[i], viewPort.x, viewPort.y+viewPort.height-1.0*j*viewPort.height/maxMessages);
            j++;
        }
    }
    ofPopStyle();
    
}

void ofApp::drawTypingString(ofRectangle viewPort) {
    if (ofGetElapsedTimeMillis()-cursorTimer>CURSOR_PERIOD) {
        cursorMode=!cursorMode;
        cursorTimer=ofGetElapsedTimeMillis();
    }
    
    ofPushStyle();
    //   ofFill();
    //   ofSetColor(255,255,255,100);
    //   ofRect(viewPort);
    
    ofSetLineWidth(2);
    ofSetColor(0, 255 ,0, 10);
    ofNoFill();
    ofRect(viewPort);
    
    ofSetColor(0,255,0, 200);
    string text;
    if (cursorMode) text=typeStr+"_"; else text=typeStr;
    myDrawText(text, viewPort.x, viewPort.y+viewPort.height);
    
    ofPopStyle();
}

void ofApp::drawCurrentSong(ofRectangle viewPort) {
    // ofSetColor(100,100,100,50);
    // ofRect(viewPort);
    ofSetColor(255, 150);
    myDrawText("iTunes state: " + iTunesChecker.getCurrentState(),viewPort.x, viewPort.y);//ofGetWidth()-290,ofGetHeight()-20);
    if (iTunesChecker.getCurrentState()=="playing") {
        ofSetColor(0,0,255, 150);
        myDrawText("Song: " + iTunesChecker.getCurrentTrack(),viewPort.x, viewPort.y+30);//ofGetWidth()-290,ofGetHeight()-10);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    if (  ofGetWindowMode()== OF_FULLSCREEN ) {
        // how to show it?
        //if (newRemoteVide) {
        // (1) ONLY the remote video (background) and remote contour:
        // if (remoteVideo) { // TEST NOT DONE to avoid flickering
        if (newRemoteVideo) {
            switch(visualMode) {
                case NONE:
                    //if (newRemoteVideo) drawVideo(textureVideoRemote, viewPortFull); // this is just to check...
                    // Or we draw a black square... or BETTER IDEA: FADE the video away while we don't get remote keyframes!!!
                    break;
                case NORMAL:
                    ofSetColor(255);
                    drawVideo(textureVideoRemote, viewPortFull);
                    break;
                case BW:
                    ofSetColor(255);
                    //drawVideo(bwTextureRemote, viewPortFull);
                    bwImage[1].draw(viewPortFull);
                    break;
                case SEPIA:
                    ofSetColor(255);
                    sepiaImage[1].draw(viewPortFull);
                    break;
                case SHADOW:
                    ofSetColor(255);
                    shadowImage[1].draw(viewPortFull);
                    break;
                case SHADER:
                    shader.begin();
                    // Pass the video texture with ofTexture * getTexture();
                    // if decodeMode == OF_QTKIT_DECODE_PIXELS_ONLY,
                    // the returned pointer will be NULL.
                    // pass other things:
                    shader.setUniformTexture("tex0", colorImage[1].getTextureReference(), 1 );
                    shader.setUniform2f("imageSize",  colorImage[1].getWidth(), colorImage[1].getHeight() );
                    shader.setUniform1f("thresholdGrey", 0.1);
                    shader.setUniform4f("cropBorder", 0.8, 0.8, 1.0, 1.0); // normalized with respect to the image size
                    shader.setUniform1f("fakeFog", ofClamp(.1, 0, 0.5));
                    
                    ofPushMatrix();
                    //ofRotateX(25);
                    //ofRotateY(25);
                    
                    colorImage[1].draw(viewPortFull);
                    
                    ofPopMatrix();
                    
                    shader.end();
                    
                    break;
                default:
                    break;
            }
            
        } else { // i.e., no remote video
            //   ofSetColor(0, 0, 0, alphaFade);//alphaFade);
            //   ofRect(viewPortFull);
        }
        
        // if (newRemoteContour) {// by commenting this, the old contour is shown, which avoids flicker
        if (contours & receiveContour) {
            ofSetColor(0,255,255);
            //drawContour(localContour,viewPortFull); //remoteContour,viewPortFull);
            drawContour(remoteContour,viewPortFull); //remoteContour,viewPortFull);
        }
        //}
        
        // FPS:
        //ofSetColor(0,255,255);
        //myDrawText(ofToString(ofGetFrameRate()),20,20);
        //myDrawText(ofToString(fpsClientVideo.getFPS(),2),20,40);
        
        // (2) The messages:
        if (showMessages) {
            ofSetColor(0);
            drawOscMessages(ofRectangle(ofGetWidth()/2-300,ofGetHeight()-10*30-80,600,10*30), 10); // last value is max messages...
            
            //(5) The current typed string:
            drawTypingString(ofRectangle(ofGetWidth()/2-300,ofGetHeight()-50,600,20));
        }
        
        // (5) iTunes state and current song:
        if (showSong) {
            drawCurrentSong(ofRectangle(100, 50, 300, 60));
        }
        
        // (7) Draw tweet:
        if (showTweets) {
            drawLastTweet(ofRectangle(ofGetWidth()/2,ofGetHeight()/2,600,10*40));
        }
        
        
    } else { // windowed screen
        
        // (1) First, the remote video (background) and remote contour:
        ofRectangle viewPort(0,0,camWidth, camHeight);
        if (newRemoteVideo) {
            switch(visualMode) {
                case NONE:
                    //if (newRemoteVideo) drawVideo(textureVideoRemote, viewPortFull); // this is just to check...
                    // Or we draw a black square... or BETTER IDEA: FADE the video away while we don't get remote keyframes!!!
                    break;
                case NORMAL:
                    ofSetColor(255);
                    drawVideo(textureVideoRemote, viewPort);
                    break;
                case BW:
                    ofSetColor(255);
                    drawVideo(bwTextureRemote, viewPort);
                    bwImage[1].draw(viewPort);
                    break;
                case SEPIA:
                    ofSetColor(255);
                    sepiaImage[1].draw(viewPort);
                    break;
                case SHADOW:
                    ofSetColor(255);
                    shadowImage[1].draw(viewPort);
                    break;
                case SHADER:
                    shader.begin();
                    // Pass the video texture with ofTexture * getTexture();
                    // if decodeMode == OF_QTKIT_DECODE_PIXELS_ONLY,
                    // the returned pointer will be NULL.
                    // pass other things:
                    shader.setUniformTexture("tex0", colorImage[1].getTextureReference(), 1 );
                    shader.setUniform2f("imageSize",  colorImage[1].getWidth(), colorImage[1].getHeight() );
                    shader.setUniform1f("thresholdGrey", 0.1);
                    shader.setUniform4f("cropBorder", 0.8, 0.8, 1.0, 1.0); // normalized with respect to the image size
                    shader.setUniform1f("fakeFog", ofClamp(.1, 0, 0.5));
                    
                    ofPushMatrix();
                    //ofRotateX(25);
                    //ofRotateY(25);
                    
                    colorImage[1].draw(viewPort);
                    
                    ofPopMatrix();
                    
                    shader.end();
                    
                    break;
                default:
                    break;
            }
        } else {
            ofSetColor(0, 0, 0, alphaFade);//alphaFade);
            ofRect(viewPort);
        }
        
        
        // if (newRemoteContour) {// by commenting this, the old contour is shown, which avoids flicker
        if (contours & receiveContour) {
            ofSetColor(0,255,255);
            drawContour(remoteContour,viewPort);
        }
        //}
        
        if (showTweets) {
        // (6) Check Twitter connection state:
        if (twitterClient.isAuthorized()) {
            ofSetColor(0,255,0);
            myDrawText("Twitter ON",20, 60);
        } else {
             ofSetColor(255,0,0);
            myDrawText("Twitter OFF",20, 60);
        }
        
        // (7) Draw tweet:
        drawLastTweet(ofRectangle(camWidth/2, camHeight/2, 100, 100));
        }
    
    // FPS:
    // ofSetColor(0,255,255);
    // myDrawText(ofToString(ofGetFrameRate()),20,20);
    // myDrawText(ofToString(fpsClientVideo.getFPS(),2),20,40);
    
    // (2) Second, our own local video, processed data and local countours (ALWAYS shown):
    viewPort.set(0,180*3,240,180);
    
    // Normal video and countours:
    ofSetColor(255);
    drawVideo(textureVideoLocal, viewPort);
    ofSetColor(255, 0, 0);
    drawContour(localContour,viewPort);
    
    // BW image:
    ofSetColor(255);
    bwImage[0].draw(240,180*3,240,180);
    
    // Sepia:
    ofSetColor(255);
    sepiaImage[0].draw(240*2,180*3,240,180);
    
    // Shadows:
    ofSetColor(255);
    shadowImage[0].draw(240*3,180*3,240,180);
    
    // Shader:
    shader.begin();
    // Pass the video texture with ofTexture * getTexture();
    // if decodeMode == OF_QTKIT_DECODE_PIXELS_ONLY,
    // the returned pointer will be NULL.
    // pass other things:
    shader.setUniformTexture("tex0", colorImage[0].getTextureReference(), 1 );
    shader.setUniform2f("imageSize",  colorImage[0].getWidth(), colorImage[0].getHeight() );
    shader.setUniform1f("thresholdGrey", 0.1);
    shader.setUniform4f("cropBorder", 0.8, 0.8, 1.0, 1.0); // normalized with respect to the image size
    shader.setUniform1f("fakeFog", ofClamp(.1, 0, 0.5));
    
    ofPushMatrix();
    //ofRotateX(25);
    //ofRotateY(25);
    
    colorImage[0].draw(240*4,180*3,240,180);
    
    ofPopMatrix();
    
    shader.end();
    
    // Indicate the current sending video:
    if (visualMode!=NONE)
    {
        viewPort.set(240*(int)(visualMode-1), 180*3,240,180);
        ofPushStyle();
        ofNoFill();
        //ofSetColor(255,255,255,100);
        //ofRect(viewPort);
        ofSetLineWidth(6);
        ofSetColor(0, 40 ,255);
        ofNoFill();
        ofRect(viewPort.x+6, viewPort.y+6, viewPort.width-12, viewPort.height-12);
        
        // Our local FPS:
        ofSetColor(255,0,0);
        myDrawText(ofToString(fpsRGB.getFPS(),2),240*(int)(visualMode-1)+10,180*3+20);
        ofPopStyle();
    }
    
    
    // (3) The Friends connected:
    drawFriendsConnected(ofRectangle(ofGetWidth()-300,0,300,ofGetHeight()));
    
    // (4) The messages:
    if(showMessages) {
        drawOscMessages(ofRectangle(ofGetWidth()-650,200,300,5*20),5);
        
        //(5) The current typed string:
        drawTypingString(ofRectangle(ofGetWidth()-650,200+5*20+30,300,25));
    }
    
    
    // (5) iTunes state and current song:
    if (showSong) drawCurrentSong(ofRectangle(ofGetWidth()-650,40, 100, 60));
    
    }
    
    //(6) The different GUI views:
	switch(guiState) {
        case HIDDEN:
            break;
        case VIEW_GUI:
            gui_VIEW.draw();
            break;
        case RTP_GUI:
            gui_RTP.draw();
            break;
        default:
            break;
    }
    
    
    // (7) Notification of a call (whatever the window mode):
    if(callingState==ReceivingCall) drawCallNotification();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    // Special commands:
    if (key==OF_KEY_RIGHT) {thresholdGray++;; if (thresholdGray>255) thresholdGray=255;}
    else if(key==OF_KEY_LEFT) {thresholdGray--; if (thresholdGray<0) thresholdGray=0;}
    else if(key==OF_KEY_UP){
		guiState = (GuiState)(guiState+1);
		guiState = (GuiState)(guiState%NumGuiStates);
    }
    else if(key==OF_KEY_F9){
        cout << "ending call" << endl;
        rtp.endCall();
        
        //rtp.setStunServer("132.177.123.6");
        //rtp.addSendVideoChannel(640,480,30);
        //rtp.addSendOscChannel();
        //rtp.addSendAudioChannel();
        
        calling = -1;
        callingState = Disconnected;
    }
    else if (key==OF_KEY_F1) {
        ofToggleFullscreen();
    }
    // Construction of the current message (ENTER will post it):
    else if(key == OF_KEY_DEL || key == OF_KEY_BACKSPACE){
		typeStr = typeStr.substr(0, typeStr.length()-1);
	}
	else if(key == OF_KEY_RETURN ){
		//typeStr += "\n";
        bFirst=true;
        
#ifdef IM_MESSAGES
        // Send the message to... all connected friends with proper capability:
        // NOTE: sendMessage() sends a message to an SPECIFIC client, specified as username or the id of the client
        /// which is usually the username + a hash that we get calling getFriends()
        // void sendMessage(const string & to, const string & message);
        const vector<ofxXMPPUser> & friends = rtp.getXMPP().getFriendsWithCapability("eTango");
		for(int i=0;i<friends.size();i++) {
            rtp.getXMPP().sendMessage(friends[i].userName, typeStr);
        }
#endif
        // Send it as OSC strings (hack):
        // send command to change song to other computer:
        ofxOscMessage msg;
        msg.setAddress("//eTango/message");
        msg.addStringArg(typeStr);
        GstClockTime now = rtp.getServer().getTimeStamp();
        rtp.getServer().newOscMsg(msg,now);
        
        // Save the sent string on the oscMessage container, but indicate it is from "me":
        oscMessage.push_back("RADA: "+typeStr);
        
        // Clear the typing string:
        typeStr.clear();
        
	}
    else if(key==OF_KEY_F2){
		settings_VIEW.serialize(parameters_VIEW);
		settings_VIEW.save("settings_VIEW.xml");
	}
	else if(key==OF_KEY_F3){
		settings_VIEW.load("settings_VIEW.xml");
		settings_VIEW.deserialize(parameters_VIEW);
	}
    else if ((key>31)&&(key<127)) typeStr.append(1, (char)key);
}



//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	ofVec2f mouse(x,y);
	if(callingState==Disconnected &&  ofGetWindowMode()== OF_WINDOW) {
		ofRectangle friendsRect(ofGetWidth()-300,0,300,rtp.getXMPP().getFriendsWithCapability("eTango").size()*20);
		if(friendsRect.inside(mouse)){
			calling = mouse.y/20;
			rtp.call(rtp.getXMPP().getFriendsWithCapability("eTango")[calling]);
			callingState = Calling;
		}
	} else if (callingState==InCall && ofGetWindowMode()== OF_WINDOW){
        ofRectangle friendsRect(ofGetWidth()-300,0,300,rtp.getXMPP().getFriendsWithCapability("eTango").size()*20);
		if(friendsRect.inside(mouse)) {
            cout << "ending call" << endl;
            rtp.endCall();
            
            // rtp.setStunServer("132.177.123.6");
            // rtp.addSendVideoChannel(640,480,30);
            // rtp.addSendOscChannel();
            //rtp.addSendAudioChannel();
            
            calling = -1;
            callingState = Disconnected;
        }
    } else if(callingState == ReceivingCall){
		if(ok.inside(mouse)){
			rtp.acceptCall();
			callingState = InCall;
		}else if(cancel.inside(mouse)){
			rtp.refuseCall();
			callingState = Disconnected;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}

void ofApp::shaderLoad() {
    // ===================================================================
    //SHADER for extraction of background and borders:
    string shaderProgram = STRINGIFY(
                                     //Note: dimensions of the image being displayed NEED to be uniform variables passed to the shader,
                                     // the reason is that the concept of the "size" of the image does not exist here, only rendered points... (size of the screen can
                                     // however be retrieved with GLSL constants...)
                                     
                                     uniform sampler2DRect tex0;
									 uniform vec2 imageSize;
									 uniform float thresholdGrey; // from 0 to 1
									 uniform vec4 cropBorder; // normalized with respect to the image size
                                     uniform float fakeFog;
                                     
                                     void main (void){
                                         
                                         vec2 pos = gl_TexCoord[0].st; // position of the texel (not normalized!)
                                         vec4 src = texture2DRect(tex0, pos);  // texture color at that texel:
                                         vec4 colorOutput = src;
                                         
										 // Convert color to grayscale:
                                         float greyLevel = 0.2989 * src.r + 0.5870 * src.g + 0.1140 * src.b;
                                         
                                         // (a) Thresholding grey (pass the threshold as uniform)
                                         if (greyLevel<thresholdGrey) colorOutput.a = 0.0; // 0 alpha
										 else colorOutput.a = 1.0;// vec4(src.rgb,1.0); // solid color
                                         
                                         // (b) Clip the borders using (note: pass the edges as uniforms)
                                         vec2 dista=2.0*abs(vec2(pos.x-imageSize.x/2.0, pos.y-imageSize.y/2.0))/imageSize;
                                         vec2 edge0=vec2(cropBorder.x, cropBorder.y);
                                         vec2 edge1=vec2(cropBorder.z, cropBorder.w);
                                         vec2 alphaSpace=smoothstep(edge0, edge1, dista);
                                         float alpha2=1.0-max(alphaSpace.x, alphaSpace.y); // pyramidal mask
                                         colorOutput.a=min(colorOutput.a, alpha2);
                                         
                                         // (c) Order independent TRANSPARENCY,
                                         // by discarding rendering = the classical openGL technique:
                                         //glAlphaFunc(GL_GREATER, 0.5);glEnable(GL_ALPHA_TEST);
                                         if(colorOutput.a < 0.1) discard;
                                         
                                         // (d) Fake FOG? (using depth passed as uniform):
                                         vec4 fogcolor=vec4(0.5, 0.0, 0.0,0.4);
                                         // colorOutput=mix(colorOutput, fogcolor, fakeFog);
                                         
                                         gl_FragColor = colorOutput;
                                     }
                                     );
    
    shader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
    shader.linkProgram();
    
}

// ======= TWITTER RELATED (make in a thread in the future) =======


bool ofApp::checkNewTweet() {
    newTweet=false;
    // Get mentions:
    ofxTwitterSearch search;
    search.count=1;
    // If since and max equal 0, then we just retrieve the latest count messages:
    search.since_id=0;
    search.max_id=0;
    twitterClient.getMentions(search);
    
    if(twitterClient.getTotalLoadedTweets() > 0) {
         tweet = twitterClient.getTweetByIndex(0); // 0 is the latest?
        newTweetText = tweet.text;
    }
    if (newTweetText!=oldTweetText) {
        newTweet=true;
        // reset alpha timer for tweet:
        alphaTimerTweet=ofGetElapsedTimeMillis();
    }
    return(newTweet);
}

void ofApp::drawLastTweet(ofRectangle viewPort) {
    //newTweetText="@milongaonline Esto es un test";
    int length=newTweetText.size(); // we want to CENTER the text on the screen
    
    ofPushStyle();
    ofPushMatrix();
    //ofFill();
    //ofSetColor(255,255,255,50);
    //ofRect(viewPort);
    
    unsigned long long t=ofGetElapsedTimeMillis()-alphaTimerTweet-1000; // ofset to have some plateau...
    float alpha=255*1.0/(1.0+exp(1.0*t/5000));
    ofSetColor(255, (unsigned char)alpha);
    // myDrawText(newTweetText, viewPort.x, viewPort.y);
    
    ofTranslate(viewPort.x, viewPort.y); // normally the center of the image
    // ofTranslate(ofGetWindowWidth()-t/5,viewPort.y,-t/10);
    // ofTranslate(viewPort.x,viewPort.y,-t/10);
    //ofScale(2,2);
    ofScale(alpha/255*2, alpha/255*2);
    ofTranslate(-1.0*myFontTweet.getSize()*length/2*0.7,0);
    //myDrawText("@milongaonline Esto es un test", 0,0);
    
    myFontTweet.drawString(newTweetText,0,0);
    
    ofPopStyle();
    ofPopMatrix();
    
    oldTweetText=newTweetText;
}

