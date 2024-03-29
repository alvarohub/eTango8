#!/bin/sh
#
#######################################
# iTunes Command Line Control v1.0.0.1
# written by David Schlosnagle
# created 2001.11.08
# modified 2007.01.28 by David West
# modified 2013.08.02 by ootput
# filename=itunes
#######################################
 
showHelp () {
    echo "-----------------------------";
    echo "iTunes Command Line Interface";
    echo "-----------------------------";
    echo "Usage: `basename $0` ";
    echo;
    echo "Options:";
    echo " status = Shows iTunes' status, current artist and track.";
    echo " play = Start playing iTunes.";
    echo " pause = Pause iTunes.";
    echo " next = Go to the next track.";
    echo " prev = Go to the previous track.";
    echo " mute = Mute iTunes' volume.";
    echo " unmute = Unmute iTunes' volume.";
    echo " vol up = Increase iTunes' volume by 10%";
    echo " vol down = Increase iTunes' volume by 10%";
    echo " vol # = Set iTunes' volume to # [0-100]";
    echo " stop = Stop iTunes.";
    echo " search = Run query, populate playlist named foo and play. "
    echo " playlist = Show playlists saved in iTunes.";
    echo " clear = clear playlist foo. ";
    echo " quit = Quit iTunes.";
}
 
if [ $# = 0 ]; then
    showHelp;
fi
 
while [ $# -gt 0 ]; do
    arg=$1;
    #songname=$2;
    #echo $songname;
    case $arg in
        "status" ) state=`osascript -e 'tell application "iTunes" to player state as string'`;
            #echo "iTunes is currently $state.";
            echo "$state";
            #if [ $state = "playing" ]; then
            #    artist=`osascript -e 'tell application "iTunes" to artist of current track as string'`;
            #    track=`osascript -e 'tell application "iTunes" to name of current track as string'`;
            #    echo "Current track $artist: $track";
            #fi
            break ;;

        "nowplaying" ) state=`osascript -e 'tell application "iTunes" to player state as string'`;
            #echo "iTunes is currently $state.";
            #echo "$state";
            if [ $state = "playing" ]; then
                #artist=`osascript -e 'tell application "iTunes" to artist of current track as string'`;
                track=`osascript -e 'tell application "iTunes" to name of current track as string'`;
                #echo "Current track $artist: $track";
                echo "$track";
            fi
            break;;
 
        "play" ) echo "Playing iTunes.";
            osascript -e 'tell application "iTunes" to play';
            break ;; 

        "playsong" ) 
            songname=$2;
            #osascript -e "tell application "iTunes" to play track "$songname" of playlist "Library"";
            #osascript -e "tell application "iTunes" to play track "$songname" of playlist "Library"";
            osascript -e "tell application \"iTunes\" to play track \"$songname\" of playlist \"Library\"";
            #osascript -e 'tell application "iTunes" to play track "If I Were a Bell" of playlist "Library"';
            break ;;

        "sethead" ) 
            #set player position to start_time
            break;;

        "pause" ) echo "Pausing iTunes.";
            osascript -e 'tell application "iTunes" to pause';
            break ;;
 
        "next" ) echo "Going to next track." ;
            osascript -e 'tell application "iTunes" to next track';
            break ;;
 
        "prev" ) echo "Going to previous track.";
            osascript -e 'tell application "iTunes" to previous track';
            break ;;
 
        "mute" ) echo "Muting iTunes volume level.";
            osascript -e 'tell application "iTunes" to set mute to true';
            break ;;
 
        "unmute" ) echo "Unmuting iTunes volume level.";
            osascript -e 'tell application "iTunes" to set mute to false';
            break ;;
 
        "vol" ) echo "Changing iTunes volume level.";
            vol=`osascript -e 'tell application "iTunes" to sound volume as integer'`;
            if [ $2 = "up" ]; then
                newvol=$(( vol+10 ));
            fi
 
            if [ $2 = "down" ]; then
                newvol=$(( vol-10 ));
            fi
 
            if [ $2 -gt 0 ]; then
                newvol=$2;
            fi
            osascript -e "tell application "iTunes" to set sound volume to $newvol";
            break ;;
 
        "stop" ) echo "Stopping iTunes.";
            osascript -e 'tell application "iTunes" to stop';
            break ;;
 
        "quit" ) echo "Quitting iTunes.";
            osascript -e 'tell application "iTunes" to quit';
            exit 1 ;;
 
        "search" ) echo "Searching Library.";
            songname=$2;
            osascript -e "tell application "iTunes"" -e "set searchResults to search playlist "Library" for "$songname"" -e "repeat with aTrack in searchResults" -e "copy aTrack to playlist "foo"" -e "end repeat" -e "play playlist "foo"" -e "end tell";
            break ;;
 
        "playlist" )
            if [ -n "$2" ]; then
                echo "Changing iTunes playlists to '$2'.";
                osascript -e 'tell application "iTunes"' -e "set new_playlist to \"$2\" as string" -e "play playlist new_playlist" -e "end tell";
                break ;
            else
                # Show available iTunes playlists.
                echo "Playlists:";
                osascript -e 'tell application "iTunes"' -e "set allPlaylists to (get name of every playlist)" -e "end tell";
                break;
            fi
            break;;
 
        "clear" ) echo "Clearing Query.";
            osascript -e "tell application "iTunes" to delete tracks of playlist "foo"";
            break ;;
 
        "help" | * ) echo "help:";
            showHelp;
            break ;;
    esac
done