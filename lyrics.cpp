#include "lrcfinder.h"

#include <iostream>
#include <cstdlib>
#include <vector>
#include <typeinfo>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <curl/curl.h>

using namespace std;

vector<string> results;
vector<string> lyrics;
vector<string> lyrics_split;
vector<string> songs;
string url = "";
string trackid = "";

std::string replace_string(string subject, const string& search, const string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
    }
    return subject;
}

string timestamps(int number){
	string timestamp;
	string min;
	string sec;
	string mil;

	int seconds = number / 1000;
	number = number/10;
	number %= 100;
	int minutes = seconds / 60;
	seconds %= 60;

	if (minutes < 10){
		min = "0" + to_string(minutes);
	}
	else{
		min = to_string(minutes);
	}
	if (seconds < 10){
		sec = "0" + to_string(seconds);
	}
	else{
		sec = to_string(seconds);
	}
	if (number < 10){
		mil = "0" + to_string(number);
	}
	else{
		mil = to_string(number);
	}
	timestamp = "[" + min + ":" + sec + "." + mil + "]";

	return timestamp;
}
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

vector<string> split(string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
    res.push_back (s.substr (pos_start));
    return res;
}

string text_downloader(curl_slist *slist, string url) {
	CURL *curl_handle;
	CURLcode res;
	curl_handle = curl_easy_init();
	string readBuffer = "";
	if (curl_handle) {	
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 600);
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl_handle, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, false);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, slist);
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl_handle);
		curl_easy_cleanup(curl_handle);
	}
	return readBuffer;
}

string get_token(){
	struct curl_slist *slist = NULL;
	string token = "";
	url = "https://open.spotify.com/get_access_token?reason=transport&productType=web_player";
	string cookie = "cookie: sp_dc=";
	string sp_dc = deadbeef->conf_get_str_fast("lyricbar.sp_dc_cookie", "");
	string cookie_end = ";";
	string cookie_sp_dc = cookie + sp_dc + cookie_end ;
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.0.0 Safari/537.36");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, "content-type: text/html; charset=utf-8");
	slist = curl_slist_append(slist, cookie_sp_dc.c_str());

  	results = split(text_downloader(slist,url),"\"");
    
//	Search for token.
	for(size_t i = 0; i < results.size(); i++) {
		if (results[i]=="accessToken"){
			token = results[i+2];
		}
	}
	return token;
}

vector<string> get_songs(string song,string artist){
	struct curl_slist *slist = NULL;
	vector<string> songs_clear;
	string bearer = "authorization: Bearer ";
	string bearer_token = bearer + get_token();
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, bearer_token.c_str());
	url = "https://api.spotify.com/v1/search?q=track%3A" + song + "+artist%3A" + artist + "&type=track";
	songs = split(text_downloader(slist,url), ":");

//	Search for Artist, Album, Track and Spotify ID.
	for(size_t i = 0; i < songs.size(); i++) {
		vector<string> songs_split;
		songs_split = split(songs[i],"\n");
		string temp;
		for(size_t j = 0; j < songs_split.size(); j++) {
			if (songs_split[j] == "track"){
				songs_split = split(songs[i+1],"\n");
 				temp = replace_string(songs_split[0],"\"","");
				songs_clear.push_back(temp);
			}
			if (songs_split[j] == "          \"type\" "){
				temp = replace_string(songs_split[j-1],"\"","");
				temp = replace_string(temp,",","");
				temp.erase(0, 1);
				songs_clear.push_back(temp);
			}
			if (songs_split[j] == "        \"release_date\" "){
				temp = replace_string(songs_split[j-1],"\"","");
				temp = replace_string(temp,",","");
				temp.erase(0, 1);
				songs_clear.push_back(temp);
			}
			if (songs_split[j] == "      \"popularity\" "){
				temp = replace_string(songs_split[j-1],"\"","");
				temp = replace_string(temp,",","");
				temp.erase(0, 1);
				songs_clear.push_back(temp);
			}
		}
	}
	return songs_clear;
}

string lyrics_downloader(string trackid){
	struct curl_slist *slist = NULL;
	string bearer = "authorization: Bearer ";
	string bearer_token = bearer + get_token();
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, bearer_token.c_str());
	url = "https://spclient.wg.spotify.com/color-lyrics/v2/track/" + trackid + "?format=json&market=from_token";
	vector<string> lrc;
	vector<string> lrc_times;
  	lyrics = split(text_downloader(slist,url),"{");
	for(size_t i = 0; i < lyrics.size(); i++) {	
//		cout << "position: " << i << " " << lyrics[i] << "\n";
		lyrics_split = split(lyrics[i],"\"");
		for(size_t j = 0; j < lyrics_split.size(); j++) {
			if (lyrics_split[j] == "startTimeMs"){
				lrc_times.push_back(timestamps(stoi(lyrics_split[j+2].c_str())));
				lrc.push_back(replace_string(lyrics_split[j+6], "\\u0027", "\'"));
			} 
		}
	}
	// Define lrc or txt lyrics style string.
	string string_lyrics = "";
	if (!lrc_times.empty()){
		if ((lrc_times.back() == "[00:00.00]") && (lrc_times[0] == "[00:00.00]")){
			for(size_t i = 0; i < lrc_times.size(); i++) {
				string_lyrics.append(lrc[i]);
				string_lyrics.append("\n");
			}
		}
		else{
			syncedlyrics = true;
			for(size_t i = 0; i < lrc_times.size(); i++) {
				string_lyrics.append(lrc_times[i]);
				string_lyrics.append(lrc[i]);
				string_lyrics.append("\n");
			}
		}
	}
	lrc_times.clear();
	lrc.clear();
	return string_lyrics;
}

// ------------------------------------- MAIN ------------------------------------------- 

string spotify(string song,string artist) {

	cout << "ARTIST AND SONG ---------------------- "<< artist << " - " << song << "\n";
	vector<string> songs_list = get_songs(song,artist);


	for(size_t i = 0; i < songs_list.size(); i++) {
			cout << songs_list[i] << "\n";
	}
	string string_lyrics = "";

//	Choose the first ID result.
	if(songs.size()>3){
		trackid = songs_list[3];
		string_lyrics = lyrics_downloader(trackid);
	}
	return string_lyrics;
}
