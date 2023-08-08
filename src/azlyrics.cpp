#include "azlyrics.h"



#include <iostream>


using namespace std;

vector<string> azlyrics_get_songs(string song,string artist){
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;
	string text_left = "text-left visitedlyr";
	string lrc = "/lrc/";
	string end_url_search = deadbeef->conf_get_str_fast("lyricbar.end_url_search", "");
	string empty_search = "Sorry, your search returned <b>no results</b>";
	struct curl_slist *slist = NULL;
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.0.0 Safari/537.36");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, "content-type: text/html; charset=utf-8");
	string url = "https://search.azlyrics.com/search.php?q=" + specialforplus(song.c_str()) + "+" + specialforplus(artist.c_str()) + end_url_search;
	bulk_results = text_downloader(slist,url);
	if (bulk_results.find(empty_search) == std::string::npos){
	  	results = split(bulk_results,"\n");
		for(size_t i = 0; i < results.size(); i++) {
			if (results[i].find(text_left) != std::string::npos){
//				cout << "RESULTS [i+1]: " << results[i+1] << "\n";
				vector<string> sub_results = split(results[i+1],"\"");
				vector<string> artist = split(sub_results[4],"<b>");
//				cout << artist[0] << " " << artist[1];
				string artist_clean = artist[1];
				artist_clean = split(artist_clean,"<")[0];
//				Artist,song,Album (empty), url to lyrics.
				artists_and_songs.push_back(artist_clean);
				artists_and_songs.push_back(sub_results[3]);
				artists_and_songs.push_back("");
				artists_and_songs.push_back(sub_results[1]);
			}
		}
	}

	return artists_and_songs;
}

struct parsed_lyrics azlyrics_lyrics_downloader(string url){
	vector<string> results;
	struct curl_slist *slist = NULL;
	string string_lyrics = "";
	bool synced = false;

	results = split(text_downloader(slist, url),"<div>");
	results = split(results[1],"</div>");
	results = split(results[0],"\n");

	for (size_t i = 2; i < results.size() - 1; i++) {
		string_lyrics.append(results[i]);
		string_lyrics.append("\n");
	}
	string_lyrics = replace_string(string_lyrics, "<br>","");

	return {string_lyrics, synced};
}
// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics azlyrics(string song,string artist) {

	struct parsed_lyrics string_lyrics = {"",false};
	vector<string> results;
	results = azlyrics_get_songs(specialforplus(song.c_str()), specialforplus(artist.c_str()));
//	for(int i = 0; i < results.size(); i+=3) {
//		cout << "artist: " << results[i] << " Song: " << results[i+1] << " url: " << results[i+2] << "\n";
//	}

//	Download first result:
	if (results.size() > 1){
		string_lyrics = azlyrics_lyrics_downloader(results[2]);
	}


	return string_lyrics;
}
