 //
//  iTunesCheckThread.h
//  eTango_1
//
//  Created by Alvaro Cassinelli on 6/30/14.
//
//
// NOTE: this works with the bash script itunes2.sh that should be in the system bin folder
//
// NOTE: how to see in FINDER (hidden/no hidden files):
// >defaults write com.apple.finder AppleShowAllFiles TRUE
//  and then > killAll Finder
//NOTE: By default, *.sh files are opened in a text editor (Xcode or TextEdit).
//To create a shell script that will execute in Terminal when you open it, name it with the “command” extension, e.g., file.command. By
//default, these are sent to Terminal, which will execute the file as a shell script.
// Remember to give it persmissions: sudo chmod 777 itunes2.sh (or chmod +x file.command)
// The states (queried with > ./itunes2.sh status) are: "playing", "stopped" and "paused"

#ifndef eTango_1_iTunesCheckThread_h
#define eTango_1_iTunesCheckThread_h

#include "ofMain.h"
#include "iTunesOsascript.h"


class threadedObject : public ofThread{
    
public:
    
    //--------------------------
    threadedObject() {
        checkPeriod=2000; // in milliseconds
        currentPlayingTrack=""; oldPlayingTrack="";
        oldState="stopped"; currentState="stopped";
        iTunesStateChange=false;
        iTunesSongChange=false;
        
        iTunes.pause(); // default state of the itunes controller object
    }
    
    void setCheckPeriod(int _millis) {
        checkPeriod=_millis;
    }
    
    void startThreadChecker(){
        startThread(true, false);   // blocking, verbose
    }
    
    void stopThreadChecker (){
        stopThread();
    }
    
    //--------------------------
    // THIS IS THE MAIN FUNCTION OF THE THREAD:
    void threadedFunction(){
        
        while( isThreadRunning() != 0 ) {
            if( lock() ) {
                
                // cout << "checking itunes" << endl;
                currentState=iTunes.getStatus();
                if (oldState!=currentState) {iTunesStateChange=true; oldState=currentState;}
                
                
                //2) In case it is playing, check the current song, and see if it changed:
                if (currentState=="playing") {
                    currentPlayingTrack=iTunes.getNowPlaying();
                    if (oldPlayingTrack!=currentPlayingTrack) {iTunesSongChange=true; oldPlayingTrack=currentPlayingTrack;}
                }
                
            }
            unlock();
            
            // Check this every checkSpeed milliseconds only!! (pooling, bad but I don't know how to do an event handler
            // for iTunes state change).
            // cout << "Checking iTunes state" << endl;
            ofSleepMillis(checkPeriod);
        }
    }
    
    // Commands to iTunes:
    // NOTE that when this is issued, we need to avoid the checker to signal a change in the next test.
    // That also means we need to lock the thread...
    void playSong(string songname) {
        if( lock() ) {
            iTunes.playsong(songname);
            currentPlayingTrack=oldPlayingTrack=songname;
            iTunesSongChange=false;
            oldState=currentState="playing";
            iTunesStateChange=false;
         }
        unlock();
    }
    void pause() {
        if( lock() ) {
            iTunes.pause();
            oldState=currentState="paused";
            iTunesStateChange=false;
        }
        unlock();
    }
    void stop() {
        if( lock() ) {
        iTunes.stop();
        oldState=currentState="stopped";
        iTunesStateChange=false;
        }
        unlock();
    }
    void resume() {
        if( lock() ) {
        iTunes.resume();
        oldState=currentState="playing";
        iTunesStateChange=false;
    }
        unlock();
    }
    
    
    
    //--------------------------
    string getCurrentTrack(){
        // do we need to do a lock?
        return(currentPlayingTrack);
    }
    
    string getOldTrack(){
        // do we need to do a lock?
        return(oldPlayingTrack);
    }
    
    string getCurrentState(){
        // do we need to do a lock?
        return(currentState);
    }
    
    string getOldState(){
        // do we need to do a lock?
        return(oldState);
    }
    
    bool stateChange() {
        // do we need to do a lock?
        return(iTunesStateChange);
    }
    
    void resetStateChange() {iTunesStateChange=false;}
    
    bool songChange() {
        // do we need to do a lock?
        return(iTunesSongChange);
    }
    
    void resetSongChange() {iTunesSongChange=false;}
    
    
private:
    iTunesControl iTunes;
    unsigned long checkPeriod; // in ms
    bool iTunesStateChange, iTunesSongChange;
    string currentState, oldState;
    string currentPlayingTrack, oldPlayingTrack;
    
};

#endif
