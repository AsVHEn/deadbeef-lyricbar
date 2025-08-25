#include "azlyrics.h"

#include <iostream>
#include <algorithm>



using namespace std;

vector<string> azlyrics_get_songs(string song,string artist){
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;
	string lrc = "/lrc/";
	string empty_search = "\"songs\":[]";
    struct curl_slist *slist = NULL;
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, "content-type: text/html; charset=utf-8");
	string url = "https://search.azlyrics.com/suggest.php?q=" + urlencode(song) + "+" + urlencode(artist);
	//string url = "https://www.azlyrics.com/search.com/suggest.php?q=" + urlencode(song) + "+" + urlencode(artist);
	bulk_results = text_downloader(slist,url, "");
	
	if (bulk_results.find(empty_search) == std::string::npos){
	  	results = split(bulk_results,"{\"url\":\"");
		for(size_t i = 0; i < results.size() -1; i++) {
				vector<string> sub_results = split(results[i+1],"\",\"autocomplete\":\"\\\"");
				string song_url = sub_results[0];
				vector<string> artist = split(sub_results[1], "\\\"  - ");
				string artist_clean = artist[1];
				artist_clean = split(artist_clean,"\"}")[0];
				string song_clean = artist[0];
				song_clean.erase(remove(song_clean.begin(), song_clean.end(), '\\'), song_clean.end());
				song_url.erase(remove(song_url.begin(), song_url.end(), '\\'), song_url.end());
//				Artist,song,Album (empty), url to lyrics.
				artists_and_songs.push_back(artist_clean);
				artists_and_songs.push_back(song_clean);
				artists_and_songs.push_back("");
				artists_and_songs.push_back(song_url);

		}
	}

	return artists_and_songs;
}

struct parsed_lyrics azlyrics_lyrics_downloader(string url){
	vector<string> results;
	struct curl_slist *slist = NULL;
	string string_lyrics = "";
	bool synced = false;

	results = split(text_downloader(slist, url, ""),"<div>");
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
	results = azlyrics_get_songs(song, artist);
	
//	Download first result:
	if (results.size() > 1){
		string_lyrics = azlyrics_lyrics_downloader(results[3]);
	}


	return string_lyrics;
}
