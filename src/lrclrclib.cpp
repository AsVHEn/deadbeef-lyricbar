#include "lrclrclib.h"

#include <iostream>
#include <cstdlib>
#include <vector>
#include <typeinfo>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <curl/curl.h>


#include <algorithm>

using namespace std;

struct curl_slist *slist = NULL;


vector<string> lrclib_get_songs(string song, string artist){
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;
	string empty_search = "[]";
    string url = "";
    
	url = "https://lrclib.net/api/search?artist_name=" + specialforplus(artist.c_str()) + "&track_name=" + specialforplus(song.c_str());
	bulk_results = text_downloader(slist,url);
	//cout << bulk_results << "\n";
	if (bulk_results.find(empty_search) == std::string::npos){
		//cout << "Entró!!!" <<"\n";
	  	results = split(bulk_results,"},{");
		for(int i = 0; i < results.size(); i++) {
			//cout << results[i] << "\n";
			if (results[i].find("id") != std::string::npos){
				vector<string> sub_results = split(results[i],":");
				vector<string> id = split(sub_results[1],",");
				//cout << "ID: " << id[0] << "\n";
				vector<string> name = split(sub_results[2],",");
				//cout << "Name: " << name[0] << "\n";
				vector<string> trackname = split(sub_results[3],",");
				//cout << "Trackname: " << trackname[0] << "\n";
				vector<string> artist = split(sub_results[4],",");
				//cout << "Artistname: " << artist[0] << "\n";
				vector<string> album = split(sub_results[5],",");
				//cout << "Album: " << album[0] << "\n";
				vector<string> duration = split(sub_results[6],",");
				//cout << "Duration: " << duration[0] << "\n";
				artists_and_songs.push_back(artist[0]);
				artists_and_songs.push_back(album[0]);
				artists_and_songs.push_back(trackname[0]);
				artists_and_songs.push_back(id[0]);
			}
		}
	}
	return artists_and_songs;
}

struct parsed_lyrics lrclib_lyrics_downloader(string trackid){
    vector<string> lyrics;
    vector<string> lyrics_split;
    string url = "";
	struct curl_slist *slist = NULL;
	bool synced = true;
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	url = "https://lrclib.net//api/get/" + trackid;
  	lyrics = split(text_downloader(slist,url),"{");
	lyrics_split = split(lyrics[1],"\",\"syncedLyrics\":\"");
	string string_lyrics = "";
	string_lyrics = replace_string(lyrics_split[1], "\\n", "\n");
    string_lyrics = string_lyrics.substr(0, string_lyrics.size()-2);
	return {string_lyrics, synced};
}
// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics lrclib(string song,string artist) {

	vector<string> songs_list = lrclib_get_songs(specialforplus(song.c_str()), specialforplus(artist.c_str()));

	//for(int i = 0; i < results.size(); i++) {
	    //cout << "Results i: " << results[i] << "\n";
	//}
//	Download first result:
	struct parsed_lyrics string_lyrics = {"",false};
	if (songs_list.size() > 1){
		string_lyrics = lrclib_lyrics_downloader(songs_list[3]);
		//cout << string_lyrics.lyrics << "\n";
	}
	return string_lyrics;
}
