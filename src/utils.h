#pragma once
#ifndef LYRICBAR_UTILS_H
#define LYRICBAR_UTILS_H

#include <deadbeef/deadbeef.h>
#include <curl/curl.h>


#include "main.h"
#include "ui.h"

#ifndef __cplusplus
#include <stdbool.h>
#else

#include <string>
#include <vector>

using namespace std;

struct id3v2_tag {
	DB_id3v2_tag_t tag{};
	~id3v2_tag() { deadbeef->junk_id3v2_free(&tag); }
};

struct parsed_lyrics{
	std::string lyrics;
	bool sync;
};

struct sync{
	vector<string> synclyrics;
	vector<double> position;
};

struct chopped{
	string past;
	string present;
	string future;
};


string specialforplus(const char* text);

bool is_playing(DB_playItem_t *track);

void update_lyrics(void *tr);

struct parsed_lyrics get_lyrics_from_metadata(DB_playItem_t *track);

void save_meta_data(DB_playItem_t *playing_song, struct parsed_lyrics lyrics);

int mkpath(const std::string &name, mode_t mode);

string replace_string(std::string subject, const std::string& search, const std::string& replace);

vector<string> split(string s, string delimiter);

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

string text_downloader(curl_slist *slist, string url);

extern "C" {
#endif // __cplusplus
int remove_from_cache_action(DB_plugin_action_t *, int ctx);
bool is_cached(const char *artist, const char *title);
void ensure_lyrics_path_exists();

#ifdef __cplusplus
}
#endif

#endif // LYRICBAR_UTILS_H
