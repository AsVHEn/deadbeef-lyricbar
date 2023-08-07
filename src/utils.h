#pragma once
#ifndef LYRICBAR_UTILS_H
#define LYRICBAR_UTILS_H

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <curl/curl.h>

#ifndef __cplusplus
#include <stdbool.h>
#else
#include <glibmm/main.h>
#include <experimental/optional>

#include "main.h"

using namespace std;

struct pl_lock_guard {
	pl_lock_guard() { deadbeef->pl_lock(); }
	~pl_lock_guard() { deadbeef->pl_unlock(); }
};

struct id3v2_tag {
	DB_id3v2_tag_t tag{};
	~id3v2_tag() { deadbeef->junk_id3v2_free(&tag); }
};

struct parsed_lyrics{
	std::string lyrics;
	bool sync;
};

string specialforplus(const char* text);

bool is_playing(DB_playItem_t *track);

void update_lyrics(void *tr);

struct parsed_lyrics get_lyrics_from_metadata(DB_playItem_t *track);

void save_meta_data(DB_playItem_t *playing_song, struct parsed_lyrics lyrics);

experimental::optional<Glib::ustring> download_lyrics_from_syair(DB_playItem_t *track);

experimental::optional<Glib::ustring> get_lyrics_from_script(DB_playItem_t *track);

int mkpath(const std::string &name, mode_t mode);

string replace_string(std::string subject, const std::string& search, const std::string& replace);

vector<string> split(string s, string delimiter);

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

string text_downloader(curl_slist *slist, string url);

vector<string> spotify_get_songs(string song,string artist);

extern "C" {
#endif // __cplusplus
int remove_from_cache_action(DB_plugin_action_t *, int ctx);
bool is_cached(const char *artist, const char *title);
void ensure_lyrics_path_exists();

#ifdef __cplusplus
}
#endif
#endif // LYRICBAR_UTILS_H
