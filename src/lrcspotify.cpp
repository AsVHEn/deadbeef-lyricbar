#include "lrcspotify.h"

#include <iostream>


using namespace std;

vector<string> results;
vector<string> lyrics;
vector<string> lyrics_split;
vector<string> songs;
string url = "";
string trackid = "";

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

vector<string> spotify_get_songs(string song,string artist){
	struct curl_slist *slist = NULL;
	vector<string> songs_clear;
	string bearer = "authorization: Bearer ";
	string bearer_token = bearer + get_token();
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, bearer_token.c_str());
	url = "https://api.spotify.com/v1/search?q=track%3A" + song + "+artist%3A" + artist + "&type=track";
	songs = split(text_downloader(slist,url), ":");
	string album;

//	Search for Artist, Album, Track and Spotify ID.
	for(size_t i = 0; i < songs.size(); i++) {
		vector<string> songs_split;
		songs_split = split(songs[i],"\n");
		string temp;
		for(size_t j = 0; j < songs_split.size(); j++) {
//			Artist (we only want one, so avoid to add more with (songs_clear.size() % 4 == 0)):
			if ((songs_split[j] == "          \"type\" ") && (songs_clear.size() % 4 == 0)) {
				temp = replace_string(songs_split[j-1],"\"","");
				temp = replace_string(temp,",","");
				temp.erase(0, 1);
				songs_clear.push_back(temp);
			}
//			Album:
			if ((songs_split[j] == "        \"release_date\" ") && (songs_clear.size() % 4 == 1)) {
				temp = replace_string(songs_split[j-1],"\"","");
				temp = replace_string(temp,",","");
				temp.erase(0, 1);
				album = temp;
			}
//			Song:
			if ((songs_split[j] == "      \"popularity\" ") && (songs_clear.size() % 4 == 1)) {
				temp = replace_string(songs_split[j-1],"\"","");
				temp = replace_string(temp,",","");
				temp.erase(0, 1);
				songs_clear.push_back(temp);
				songs_clear.push_back(album);
			}
//			Spotify ID:
			if ((songs_split[j] == "track") && (songs_clear.size() % 4 == 3)) {
				songs_split = split(songs[i+1],"\n");
 				temp = replace_string(songs_split[0],"\"","");
				songs_clear.push_back(temp);
			}
		}
	}
	return songs_clear;
}

struct parsed_lyrics spotify_lyrics_downloader(string trackid){
	struct curl_slist *slist = NULL;
	string bearer = "authorization: Bearer ";
	string bearer_token = bearer + get_token();
	bool synced = false;
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
				string 	line = replace_string(lyrics_split[j+6], "\\u0027", "\'");
						line = replace_string(line, "\\u0026", "&");
				lrc.push_back(line);
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
			synced = true;
			for(size_t i = 0; i < lrc_times.size(); i++) {
				string_lyrics.append(lrc_times[i]);
				string_lyrics.append(lrc[i]);
				string_lyrics.append("\n");
			}
		}
	}
	lrc_times.clear();
	lrc.clear();
	return {string_lyrics, synced};
}

// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics spotify(string song,string artist) {

	vector<string> songs_list = spotify_get_songs(song,artist);

//	for(size_t i = 0; i < songs_list.size(); i++) {
//			cout << songs_list[i] << "\n";
//	}
	struct parsed_lyrics string_lyrics = {"",false};

//	Choose the first ID result.
	if(songs_list.size()>3){
		trackid = songs_list[3];
		string_lyrics = spotify_lyrics_downloader(trackid);
	}
	return string_lyrics;
}
