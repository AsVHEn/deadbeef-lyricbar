#include "rclyricsband.h"

#include <iostream>
#include <algorithm>


using namespace std;

string htmlEntitiesDecode (string str) {

	string subs[] = {"&#034;", "&quot;", "&#039;", "&apos;", "&#038;", "&amp;","&#060;", "&lt;", "&#062;", "&gt;", "&034;", "&039;", "&038;", "&060;", "&062;"};
	
	string reps[] = {"\"", "\"", "'", "'", "&", "&", "<", "<", ">", ">", "\"", "'", "&", "<", ">"};
	
	size_t found;
	for(int j = 0; j < 15; j++) {
		do {
			found = str.find(subs[j]);
	  		if (found != string::npos)
		    		str.replace (found,subs[j].length(),reps[j]);
    		} while (found != string::npos);
  	}
	return str;
}

vector<string> rclyricsband_get_songs(string song,string artist){
    struct curl_slist *slist = NULL;
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;

	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	string url = "https://rclyricsband.com/";
	bulk_results = text_downloader(slist,url, "search=" + song + " " + artist);
	
	if (bulk_results.find("<h2 class=\"head_results\">Songs</h2>") != std::string::npos){
	    int start_position = bulk_results.find("<h2 class=\"head_results\">Songs</h2>");
	    int finish_position = bulk_results.find("</a></li>            </div>");
	    int range = finish_position - start_position;
//	    To eliminate also "<h2 class=\"head_results\">Songs</h2>" is neccesary to add an remove 35 characters.
		bulk_results = bulk_results.substr(start_position + 35, range - 35);
	  	results = split(bulk_results,"<li class='list_results'><a href='");
	  	
		for(size_t i = 0; i < results.size() - 1; i++) {
		    vector<string> sub_results = split(results[i+1],"' class='song_search'>");
			string song_url = sub_results[0];
			vector<string> artist = split(sub_results[1], " - ");
			string artist_clean = artist[1];
			artist_clean = split(artist_clean,"</")[0];
			string song_clean = artist[0];
//			Artist,song,Album (empty), url to lyrics.
			artists_and_songs.push_back(artist_clean);
			artists_and_songs.push_back(song_clean);
			artists_and_songs.push_back("");
			artists_and_songs.push_back(song_url);
        }
	}

	return artists_and_songs;
}

struct parsed_lyrics rclyricsband_lyrics_downloader(string url){
	vector<string> results;
	struct curl_slist *slist = NULL;
	string string_lyrics = "";
	bool synced = false;
	slist = curl_slist_append(slist, "content-type: text/html; charset=utf-8");
	slist = curl_slist_append(slist, "Accept-Charset: utf-8");
	url = "https://rclyricsband.com/" + url;
	results = split(text_downloader(slist, url, ""),"<p id='lrc_text' class='lrc_text_format'>");
	results = split(results[1],"RCLyricsBand.Com</p>");
	string_lyrics = replace_string(results[0], "<br>", "\n");
	string_lyrics = htmlEntitiesDecode(string_lyrics);

	return {string_lyrics, synced};
}
// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics rclyricsband(string song,string artist){
	vector<string> results;
	cout << "Artist: " << artist << "\n";
	cout << "Song: " << song << "\n";
	results = rclyricsband_get_songs(song, artist);
	struct parsed_lyrics string_lyrics = {"",true};
	
	if (results.size() > 1){
		string_lyrics = rclyricsband_lyrics_downloader(results[3]);
		cout << string_lyrics.lyrics << "\n";
	}

	return string_lyrics;
}
