//
//  iTunesOsascript.cpp
//  eTango_3
//
//  Created by Alvaro Cassinelli on 7/4/14.
//
//

#include "iTunesOsascript.h"

// this executes a one line osascript and read the output (popen):
 string iTunesControl::execPopen(string cmdstring) {
    char auxCharString[512];
    string answerString;
    FILE *fp = popen(cmdstring.c_str(), "r");
    if (fgets(auxCharString, sizeof(auxCharString), fp) != NULL) answerString=ofToString(auxCharString);
    
    // NOTE: the answers from iTunes are strings finalized by \n, we will strip that:
     answerString=answerString.substr(0, answerString.size()-1);
     
   //  cout<<answerString<<endl;
     
    int status = pclose(fp);
    if (status == -1) {
        /* Error reported by pclose() */
        //...
    } else {
        /* Use macros described under wait() to inspect `status' in order
         to determine success/failure of command executed by popen() */
        //...
    }
     
    return(answerString);
}