/*
 * ofApp.h
 *
 *  Created on: Jul 19, 2013
 *      Author: arturo castro
 */

#pragma once

#include "ofMain.h"
#include "ofxGstXMPPRTP.h"
#include "ofxFPS.h"
#include "ofxGui.h"
//#include "ofxKinect.h"
#include "ofxOpenCv.h"
#include "ofxTwitter.h"
#include "iTunesCheckThread.h"


// For always accept call:
#define ACCEPT_CALL

#define IM_MESSAGES

#define CURSOR_PERIOD 1000 // in ms

#define CHECK_TIMER_TWEET 10000 // in ms
// ATTENTION: third party twitter applications have a "rate limit" of 100 services per HOUR!!!
// This means I cannot check in a period less than:  3600/100=36 sec

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    void exit();
    
    void drawVideo(ofTexture image, ofRectangle viewPort=ofRectangle(0,0,640,480));
    void drawContour(ofPolyline contour, ofRectangle viewPort=ofRectangle(0,0,640,480));
    void drawCallNotification();
    void drawFriendsConnected(ofRectangle window);
    
    void drawXmppMessages(ofRectangle viewPort, int maxMessages);
    void drawOscMessages(ofRectangle viewPort, int maxMessages);
    void drawTypingString(ofRectangle viewPort);
    
    void drawCurrentSong(ofRectangle viewPort);
    
    void myDrawText(string textString, float x, float y);
    
    bool onFriendSelected(const void *sender);
    void onConnectionStateChanged(ofxXMPPConnectionState & connectionState);
    void onNewMessage(ofxXMPPMessage & msg);
    void onCallReceived(string & from);
    void onCallAccepted(string & from);
    void onCallFinished(ofxXMPPTerminateReason & reason);
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    enum GuiState{
        HIDDEN,
        VIEW_GUI,
        RTP_GUI,
        NumGuiStates
    } guiState;
    
    
    enum VisualModes{
        NONE=0,
        NORMAL,
        BW,
        SEPIA,
        SHADOW,
        SHADER,
        NumVisualModes
    } visualMode;
    
    
    void changeVideoMode_noVideo(bool& mode);
    void changeVideoMode_normalVideo(bool& mode);
    void changeVideoMode_bw(bool& mode);
    void changeVideoMode_sepia(bool& mode);
    void changeVideoMode_shadow(bool& mode);
    void changeVideoMode_shaderView(bool& mode);
    
    ofxCvColorImage convertToSepia(ofxCvColorImage& inputImage);
    
    void screenMode(bool & screenmode);
    void sendContourPlease(bool& send);
    void sendVideoPlease(VisualModes mode);
    
    void shaderLoad();
    
    ofxGstXMPPRTP rtp;
    
    threadedObject iTunesChecker;
    
    ofVideoGrabber vidGrabber;
    int 	camWidth, camHeight, fps;
    ofRectangle viewPortFull, viewPortWindow;
    
    
    // fps counters
    ofxFPS fpsRGB; // local video
    ofxFPS fpsClientVideo; // client video
    
    // place where the textures for remote and local video are stored:
    ofTexture textureVideoRemote, textureVideoLocal;
    ofTexture bwTexture, bwTextureRemote;
    
    int alphaFade;
    
    // Images:
    // Local and from the remote side (we need to recompute things because from the time being, we can only send RGB images on the pipeline)
    // ofImage myImageColor, myImageGray;
    ofxCvColorImage colorImage[2], sepiaImage[2];
    ofxCvGrayscaleImage bwImage[2], shadowImage[2];
    
    ofxCvColorImage backgroundImage;
    ofxCvGrayscaleImage backgroundImageGray;

    ofxCvContourFinder contourFinder;
    ofPolyline remoteContour, localContour;
    
    ofShader    shader;
    
    void updateVisualMode(VisualModes mode);
    
    // Exclusive visual modes (shame there is no GUI object for a dropdown menu!):
    ofParameter<bool> noVideo;
    ofParameter<bool> normalVideo;
    ofParameter<bool> bw;
    ofParameter<bool> sepia;
    ofParameter<bool> shadow;
    ofParameter<bool> shaderView;
    
    ofParameter<bool> contours;
    ofParameter<int> thresholdGray; // for computing the contour and shadows
    ofParameter<bool> invertThreshold;

    ofParameter<bool> steam;
    
    // Others:
    ofParameter<bool> fullScreen;
    ofParameter<bool> showMessages;
    ofParameter<bool> showTweets;
    ofParameter<bool> showSong;
    
    ofParameter<bool> extractContour;

    ofParameterGroup parameters_VIEW;
    
    ofXml settings_VIEW, settings_XMPP;
    ofParameter<int> fpsParam;
    
    ofxPanel gui_RTP, gui_VIEW;
    
    ofTrueTypeFont myFont,myFontTweet;
    
    //ofEasyCam camera;
        
    bool newLocalVideo;
    bool newLocalContour;
    
    bool newRemoteVideo, receiveVideo;
    bool newRemoteContour, receiveContour;
    
    bool newMessage;
    
    enum CallingState{
        MissingSettings,
        Calling,
        ReceivingCall,
        InCall,
        Disconnected
    }callingState;
    string callFrom;
    int calling;
    
    vector<ofxXMPPUser> friends;// = rtp.getXMPP().getFriendsWithCapability("eTango");
    
    
    bool bFirst, cursorMode;
    string typeStr;
    unsigned long long cursorTimer;
    deque<ofxXMPPMessage> xmppMessages;
    vector<string> oscMessage; // this is a hack (ofxXMPPMessage seems not working...)
    
    
    // TWITTER STUFF:
    bool checkNewTweet();
    unsigned long long checkTimerTweet;
    void drawLastTweet(ofRectangle viewPort);
    ofxTwitter twitterClient;
    ofxTwitterTweet tweet;
    int actualTweet,  sinceIndex, maxIndex;
    bool reloadTweet;
    string newTweetText, oldTweetText;
    bool newTweet;
    unsigned long long alphaTimerTweet;
    unsigned long numberOfTweetChecks;
    
    ofSoundPlayer ring;
    unsigned long long lastRing;
    ofRectangle ok,cancel;
    
};
