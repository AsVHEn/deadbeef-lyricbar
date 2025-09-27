#include "lrcmusic163.h"

#include <vector>

#include <string>
#include <curl/curl.h>
#include "../gettext.h"

#include <iostream>
#include <algorithm>

using namespace std;

vector<string> inside_brackets(string input, char start_bracket, char finish_bracket){
	string result;
	vector<string> res;
	string curr_str = "";
    size_t start = 0;
    size_t end = start;
    int balance = 0;
    if (input.size() == 0){
        return res;
    }
    
    for (size_t i = start; input[i]; i++) {
        if (input[i] == start_bracket) {
            if (balance == 0){
                start = end = i;
            }
            balance++;
        }
        else if (input[i] == finish_bracket) {
            balance--;
            if (balance == 0){
                end = i;
            }
        }
        if (start != end && balance == 0) {
            result = input.substr (start + 1, end-start - 1);
            start = end;
            res.push_back(result);
        }
    }
    return res;
}


vector<string> music_163_get_songs(string song, string artist) {
    struct curl_slist *slist = NULL;
	string artist_and_song;
	string bulk_results;
	vector<string> results;
	vector<string> artists_and_songs, clean_songs;
	string text_left = "text-left visitedlyr";
	string lrc = "/lrc/";
	string empty_search = "{\"songCount\":0}";
    string twohundred_search = "{\"result\":{},\"code\":200}";
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	string url = "https://music.163.com/api/search/get?s=" + urlencode(song) + "+" + urlencode(artist) + "&type=1&offset=0&sub=false&limit=5";
	bulk_results = text_downloader(slist,url, "");
	
	if (bulk_results.find(empty_search) == std::string::npos && bulk_results.find(twohundred_search) == std::string::npos){
  	    string artist;
  	    string album;
        // Get all inside {"songs":[...]}
        bulk_results = inside_brackets(bulk_results, '[', ']')[0];
        // Divide in songs by {...},{...},...
        vector<string> Balanced = inside_brackets(bulk_results, '{', '}');
        
        for (size_t j = 0; j < Balanced.size(); j++) {
            string entry = Balanced[j];
            // Divide every songs in subsections by {...},{...},... too.
            vector<string> entry_chopped = inside_brackets(entry,'{', '}');
            
            for (size_t i = 0; i < entry_chopped.size(); i++) {
               string subbracket = entry_chopped[i];
               // Check if subsection is preceded by "artist" or by "album" to get properly and extract it.
               
                if (entry.substr(entry.find(subbracket) - 9,9) == "tists\":[{") {
                    subbracket = split(subbracket, "\"name\":\"")[1];
                    artist = split(subbracket, "\",\"")[0];
                }
                else if (entry.substr(entry.find(subbracket) - 9,9) == "\"album\":{") {
                    subbracket = split(subbracket, "\"name\":\"")[1];
                    album = split(subbracket, "\",\"")[0];
                }
                entry.erase(entry.find(entry_chopped[i]), entry_chopped[i].size());
            }
            string song_url = split(entry, "\"id\":")[1];
            song_url = split(song_url, ",")[0];
            string song_clean = split(entry, ",\"name\":\"")[1];
            song_clean = split(song_clean, "\",")[0];
            //Artist,song,Album, url to lyrics.
			artists_and_songs.push_back(artist);
			artists_and_songs.push_back(song_clean);
			artists_and_songs.push_back(album);
			artists_and_songs.push_back(song_url);  
        }  
	}

	return artists_and_songs;
}

struct parsed_lyrics music_163_lyrics_downloader(string trackid) {

	vector<string> results;
	string all_text;
	struct curl_slist *slist = NULL;
	string string_lyrics = "";
	bool synced = false;
	slist = curl_slist_append(slist, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");
	slist = curl_slist_append(slist, "app-platform: WebPlayer");
	string url = "https://music.163.com/api/song/lyric?_nmclfl=1";
	results = inside_brackets(text_downloader(slist, url, "id=" + trackid + "&tv=-1&lv=-1&rv=-1&kv=-1"), '{', '}');
	all_text = results[0];
	results = inside_brackets(results[0], '{', '}');
	for (size_t i = 0; i < results.size(); i++) {
	    if (all_text.substr(all_text.find(results[i]) - 7,7) == "\"lrc\":{") {
	        string_lyrics = split(results[i], "\"lyric\":\"")[1];
	    }
	}
	results = split(string_lyrics,"\\n");
	string_lyrics = replace_string(string_lyrics, "\\n", "\n");
    string_lyrics.pop_back();
    
    int total_lines = 1;
	int timestamped_lines = 1;
    bool first_timestamped = false;
    
    if(string_lyrics != ""){
		for (size_t i = 0; i < results.size(); i++) {
		    if (results[i].length() > 4){
		        if ((results[i].at(0) == '[') && (results[i].at(3) == ':') && (results[i].at(6) == '.') ) {
		            if (results[i].length() > 10){
		                if (results[i].at(10) == ']' && results[i].at(9) != ']'){
		                    results[i].erase(9,1);
		                }
		            first_timestamped = true;
		            timestamped_lines += 1;
		            }
		        }
		    }
		    if (first_timestamped == true){
		        total_lines += 1;
		    }
		}
		if ((timestamped_lines*100/total_lines > 75) && (total_lines != 1)){
			synced = true;
			string_lyrics = "";
			for (size_t i = 0; i < results.size(); i++) {
			    string_lyrics.append(results[i]);
			    string_lyrics.append("\n");
			}
		}
	}
    		
    string_lyrics = replace_string(string_lyrics, "作词 : ", _("LYRICS: "));
    string_lyrics = replace_string(string_lyrics, "作曲 : ", _("COMPOSER: "));
    string_lyrics = replace_string(string_lyrics, "录音 : ", _("RECORDING ENGINEER: "));
    string_lyrics = replace_string(string_lyrics, "音频工程师 : ", _("AUDIO ENGINEER: "));
    string_lyrics = replace_string(string_lyrics, "音频助理 : ", _("ASSISTANT ENGINEER: "));
    string_lyrics = replace_string(string_lyrics, "混音师 : ", _("MIXING: "));
    string_lyrics = replace_string(string_lyrics, "演奏 : ", _("MUSICAL PERFORMER: "));
    string_lyrics = replace_string(string_lyrics, "母带工程师 : ", _("MASTER ENGINEER: "));
    string_lyrics = replace_string(string_lyrics, "制作人 : ", _("PRODUCER: "));
    string_lyrics = replace_string(string_lyrics, "音频后期制作 : ", _("POST-PRODUCER: "));
    string_lyrics = replace_string(string_lyrics, "贝斯 : ", _("BASS: "));
    string_lyrics = replace_string(string_lyrics, "附加制作 : ", _("ADDITIONAL PRODUCTION: "));
    string_lyrics = replace_string(string_lyrics, "人声 : ", _("VOICE: "));
    string_lyrics = replace_string(string_lyrics, "吉他 : ", _("GUITAR: "));
    string_lyrics = replace_string(string_lyrics, "键盘 : ", _("KEYBOARDS: "));
    string_lyrics = replace_string(string_lyrics, "鼓 : ", _("DRUMS: "));
    string_lyrics = replace_string(string_lyrics, "编程 : ", _("PROGRAMMING: "));
    string_lyrics = replace_string(string_lyrics, "混音助理 : ", _("MIX ASSISTANT: "));
    
	return {string_lyrics, synced};
}
// ------------------------------------- MAIN ------------------------------------------- 

struct parsed_lyrics music_163(string song,string artist) {

	vector<string> songs_list = music_163_get_songs(song, artist);

//	for(int i = 0; i < songs_list.size(); i++) {
//	    cout << "Results i: " << songs_list[i] << "\n";
//	}
//	Download first result:
	struct parsed_lyrics string_lyrics = {"",false};
	if (songs_list.size() > 1){
		string_lyrics = music_163_lyrics_downloader(songs_list[3]);
	}
	return string_lyrics;
}
