//
//  iTunesThread.cpp
//  eTango_1
//
//  Created by Alvaro Cassinelli on 6/30/14.
//
//

#include "iTunesThread.h"

#ifndef _THREADED_OBJECT
#define _THREADED_OBJECT

#include "ofMain.h"



class threadedObject : public ofThread{
    
public:
    
    
    int count;  // threaded fucntions that share data need to use lock (mutex)
    // and unlock in order to write to that data
    // otherwise it's possible to get crashes.
    //
    // also no opengl specific stuff will work in a thread...
    // threads can't create textures, or draw stuff on the screen
    // since opengl is single thread safe
    int countSpeed;
    
    //--------------------------
    threadedObject(){
        count = 0;
    }
    
    
    void threadCountSpeed(int _millis) {
        countSpeed=_millis;
    }
    
    void start(){
        startThread(true, false);   // blocking, verbose
    }
    
    void stop(){
        stopThread();
    }
    
    //--------------------------
    void threadedFunction(){
        
        while( isThreadRunning() != 0 ){
            if( lock() ){
                count++;
                if(count > 5000) count = 0;
                unlock();
                ofSleepMillis(1 * countSpeed);
            }
        }
    }
    
    //--------------------------
    void draw(int x, int y){
        
        string str = "I am a slowly increasing thread. \nmy current count is: ";
        
        if( lock() ){
            str += ofToString(count);
            unlock();
        }else{
            str = "can't lock!\neither an error\nor the thread has stopped";
        }
        ofDrawBitmapString(str, x, y);//50, 56);
    }
    
    
    
};

#endif
