#include "lrclrclib.h"

#include <iostream>
#include <cstdlib>
#include <vector>
#include <typeinfo>
#include <fstream>
#include <string>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

using namespace std;

struct curl_slist *slist = NULL;


vector<string> lrclib_get_songs(string song, string artist, string album, int duration) {
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;
	string empty_search = "[]";
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");
	string url = "https://lrclib.net/api/search?artist_name=" + urlencode(artist) + "&track_name=" + urlencode(song);
	if (album != ""){
	url = url + "&album_name=" + album;
	}
	if (duration != 0){
	    url = url + "&duration=" + to_string(duration);
	}
	bulk_results = text_downloader(slist, url, "");
	//cout << "BULK RESULTS: ---------------------------------------" << "\n";
	//cout << bulk_results << "\n";
	
	if (bulk_results.find(empty_search) == std::string::npos) {
	  	results = split(bulk_results,"},{");
		for(long unsigned int i = 0; i < results.size(); i++) {
			//cout << results[i] << "\n";
			if (results[i].find("id") != std::string::npos) {
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
				artists_and_songs.push_back(trackname[0]);
				artists_and_songs.push_back(album[0]);
				artists_and_songs.push_back(id[0]);
			}
		}
	}
	return artists_and_songs;
}

struct parsed_lyrics lrclib_lyrics_downloader(string trackid) {

    vector<string> lyrics;
    vector<string> lyrics_split;
    string url = "";
	struct curl_slist *slist = NULL;
	bool synced = true;
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");
	url = "https://lrclib.net/api/get/" + trackid;
	//cout << "url:---------------------     " << url << "\n";
	//cout << "PARSED_LYRICS: ---------------------------------------" << "\n";
	//cout << text_downloader(slist,url, "") << "\n";
  	lyrics = split(text_downloader(slist,url, ""),"{");
	lyrics_split = split(lyrics[1],"\",\"syncedLyrics\":\"");
	string string_lyrics = "";
		
	if (lyrics_split.size() > 1) {
		size_t end_pos = lyrics_split[1].find("\",\"");   // another field follows
		if (end_pos == string::npos) {
			end_pos = lyrics_split[1].find("\"}");        // syncedLyrics was the last field
		}
		string_lyrics = replace_string(lyrics_split[1].substr(0, end_pos), "\\n", "\n");
		//string_lyrics = replace_string(lyrics_split[1], "\\n", "\n");
		//string_lyrics = string_lyrics.substr(0, string_lyrics.size()-2);
		//string_lyrics = replace_string(string_lyrics, "\",\"lyricsfile\":nul", "");
    }
    else {
        synced = false;
        lyrics_split = split(lyrics_split[0],"\"plainLyrics\":\"");
        string_lyrics = "";
        if (lyrics_split.size() > 1){
            string_lyrics = lyrics_split[1].substr(0, lyrics_split[1].size()-22);
            string_lyrics = replace_string(string_lyrics, "\\n", "\n");
            string_lyrics = replace_string(string_lyrics, "\",\"syncedLyrics\":n", "");
        }
    }
	return {string_lyrics, synced};
}
// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics lrclib(string song,string artist, string album) {

	vector<string> songs_list = lrclib_get_songs(song, artist, album, 0);

	//cout << "RESULTS: ------------------------------------------------" << "\n";
	
	for(long unsigned int i = 0; i < songs_list.size(); i++) {
	    cout << "Results i: " << songs_list[i] << "\n";
	}
//	Download first result:
	struct parsed_lyrics string_lyrics = {"",false};
	if (songs_list.size() > 3){
		string_lyrics = lrclib_lyrics_downloader(songs_list[3]);
	}
	return string_lyrics;
}
