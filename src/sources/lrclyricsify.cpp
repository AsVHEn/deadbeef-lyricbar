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


bool syncedfound = false;

using namespace std;

// Function to capture html.

void get_page(string url , const char* file_name) {

	CURL* easyhandle = curl_easy_init();
	curl_easy_setopt( easyhandle, CURLOPT_URL, url.c_str() ) ;
	FILE* file = fopen( file_name, "w");
	curl_easy_setopt( easyhandle, CURLOPT_WRITEDATA, file) ;
	curl_easy_perform( easyhandle );
	curl_easy_cleanup( easyhandle );

    // disable cert validation
    curl_easy_setopt (easyhandle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt (easyhandle, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt (easyhandle, CURLOPT_SSLVERSION, 3); // not sure if this line is needed, and whether it would do any good
	fclose(file);
}

string syair(ifstream &file, ofstream &outFile, string artisttitle){

	string outFileName = lyrics_dir + artisttitle + ".lrc";

	// Possible urls from website search results.

	ofstream options(optionstxt);
	while (getline(file, str)) {
		istringstream iss(str);
		while ( getline( iss, str, ' ' ) ) {
			if (string(str.c_str()).compare(0,13,string ("href=" "\"" "/lyrics", 13) ) == 0){
				options << string(str.c_str()) << endl;
			}
		}
	}

	// Download first search result.	

	ifstream syairoptions(optionstxt);
	getline(syairoptions, str);


	if (1 < string(str.c_str()).length()){
		str.erase(0,6);
		str.erase(str.size() - 1);
		for (unsigned i = 0; i <  str.size()-2; i++) {
			transform(artisttitle.begin(), artisttitle.end(), artisttitle.begin(), ::tolower);
			if (str[i + 8] != '-'){
            	if (str[i + 8] == artisttitle[i]){
				}
				else if ((str[i + 8] == '/') && (str[i + 9] == artisttitle[i+3]) && (str[i + 10] == artisttitle[i+4])){
					string url = "https://www.lyricsify.com/search?q=" + string(str);
					get_page(url, lyrictxt);
					syncedfound = true;
					syncedlyrics = true;
					break;
				}
				else {
					remove(outFileName.c_str());
					syncedfound = false;
					break;
				}
			}
        }
	}
	else {
		remove(optionstxt);
		remove(outFileName.c_str());
		syncedfound = false;
		return;
	}

	lyricstart = false;
	
	ifstream syairraw(lyrictxt);
	while (getline(syairraw, str)) {
		if (str.compare(0,4,"[ar:") == 0 || str.compare(0,4,"[ti:") == 0  ){
			lyricstart = true;
		}
		if (lyricstart == true){
			istringstream iss(str);
			while ( getline( iss, str, ' ' ) ) {

			if (4 < string(str.c_str()).length() && string(str.c_str()).substr(string(str.c_str()).length() - 5) == "<br>\r") {
				str.resize(str.size() - 5);
				outFile << str << endl;
			}

			else if (3 < string(str.c_str()).length() && string(str.c_str()).substr(string(str.c_str()).length() - 4) == "<div"){
				str.resize(str.size() - 4);
				outFile << str << endl;
				break;

			}
			else if (3 < string(str.c_str()).length() && string(str.c_str()).substr(string(str.c_str()).length() - 4) != "<br>"){
				outFile << str << " " ;
			}
			else if (3 >= string(str.c_str()).length()){
				outFile << str << " ";
			}
		}
		}
	}
	return lyrics;
}
