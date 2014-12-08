//
//  iTunesOsascript.h
//  eTango_1
//
//  Created by Alvaro Cassinelli on 7/3/14.
//
//

#ifndef eTango_1_iTunesOsascript_h
#define eTango_1_iTunesOsascript_h

#include "ofMain.h"


class iTunesControl {
public:
    iTunesControl() {this->pause();}
    // ~iTunesControl();
    
    void stop() {this->execSystem("osascript -e 'tell application \"iTunes\" to stop'");}
    void pause() {this->execSystem("osascript -e 'tell application \"iTunes\" to pause'");}
    void resume() {this->execSystem("osascript -e 'tell application \"iTunes\" to play'");}
    void playsong(string songname) {this->execSystem("osascript -e 'tell application \"iTunes\" to play track \""+songname+"\" of playlist \"Library\"'");}
    
    string getNowPlaying() {return(this->execPopen("osascript -e 'tell application \"iTunes\" to name of current track as string'"));}
    string getStatus() {return(this->execPopen("osascript -e 'tell application \"iTunes\" to player state as string'"));}
    
    
private:
    void execSystem(string cmd) { system(cmd.c_str()); }
    string execPopen(string cmdstring); // same than execSystem, but with redirection of the output to the caller program
    
};

#endif
