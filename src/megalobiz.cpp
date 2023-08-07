#include "megalobiz.h"

using namespace std;

int countFreq(string& pat, string& txt)
{
    int M = pat.length();
    int N = txt.length();
    int res = 0;
 
    /* A loop to slide pat[] one by one */
    for (int i = 0; i <= N - M; i++) {
        /* For current index i, check for
           pattern match */
        int j;
        for (j = 0; j < M; j++)
            if (txt[i + j] != pat[j])
                break;
 
        // if pat[0...M-1] = txt[i, i+1, ...i+M-1]
        if (j == M) {
            res++;
        }
    }
    return res;
}

vector<string> megalobiz_get_songs(string song,string artist){
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;
	string empty_search = "Megalobiz could not find any result for";
	string lrc = "/lrc/";

	struct curl_slist *slist = NULL;
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.0.0 Safari/537.36");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	slist = curl_slist_append(slist, "content-type: text/html; charset=utf-8");
	string url = "https://www.megalobiz.com/search/all?qry=" + specialforplus(song.c_str()) + "+" + specialforplus(artist.c_str()) + "&searchButton.x=0&searchButton.y=0";
	bulk_results = text_downloader(slist,url);
	if (bulk_results.find(empty_search) == std::string::npos){
	  	results = split(bulk_results,"\n");
    
		for(size_t i = 0; i < results.size(); i++) {
			if (results[i]=="		<div class=\"entity_full_member_image\" >"){
				vector<string> sub_results = split(results[i+1],"\"");
				artists_and_songs.push_back(sub_results[1]);
				artists_and_songs.push_back(sub_results[3]);
			}
		}
	}
	else {
		artists_and_songs.push_back("Fail");
		artists_and_songs.push_back("Fail");
	}
	string hyphen = " - ";
	string by = " by ";
	string lrc_space = "Lrc ";
	for(size_t i = 0; i < artists_and_songs.size(); i+=2) {
		if (artists_and_songs[i].find(lrc) != string::npos){

			int hyphen_freq = countFreq(hyphen,artists_and_songs[i+1]);
			int by_freq = countFreq(by,artists_and_songs[i+1]);
			if ((hyphen_freq = 0) || (by_freq > 0)){
				vector<string> a_s;
				if (hyphen_freq > 0){
					a_s = split(artists_and_songs[i+1],hyphen);
				}
				if (by_freq > 0){
					a_s = split(artists_and_songs[i+1],by);
				}
//				Artist,song,Album (empty), url to lyrics.
				clean_songs.push_back(a_s[1]);
				clean_songs.push_back(replace_string(a_s[0],lrc_space,""));
				clean_songs.push_back("");
				clean_songs.push_back(artists_and_songs[i]);
			}
		}
	}
	return clean_songs;

}

struct parsed_lyrics megalobiz_lyrics_downloader(string partial_url){
	vector<string> results;
	struct curl_slist *slist = NULL;
	string string_lyrics = "";
	bool synced = true;
	string span = "</span>"; 
	string url = "https://www.megalobiz.com" + partial_url;


	results = split(text_downloader(slist, url),"\n");

	for(size_t i = 0; i < results.size(); i++) {
		if (results[i]=="[re:www.megalobiz.com/lrc/maker]<br>"){
			size_t j = i + 1;
			while (results[j].find(span) == std::string::npos){
				j++;
				string_lyrics.append(results[j]);
				string_lyrics.append("\n");
			}
			break;
		}
	}
	string_lyrics = replace_string(string_lyrics, "<br>","");
	string_lyrics = replace_string(string_lyrics, span,"");

	return {string_lyrics, synced};
}
// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics megalobiz(string song,string artist) {

	vector<string> results;
	
	results = megalobiz_get_songs(specialforplus(song.c_str()), specialforplus(artist.c_str()));
//	cout << results[0] << "\n";

//	Download first result:
	struct parsed_lyrics string_lyrics = {"",false};
	
	string_lyrics = megalobiz_lyrics_downloader(results[0]);



	return string_lyrics;
}
