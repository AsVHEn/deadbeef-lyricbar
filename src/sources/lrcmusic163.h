#pragma once
#include <deadbeef/deadbeef.h>
#include "../utils.h"

#include <gtk/gtk.h>

#ifdef __cplusplus
#include <vector>
#include <string>


using namespace std;

extern bool syncedlyrics;

struct parsed_lyrics music_163(string artist, string song);

struct parsed_lyrics music_163_lyrics_downloader(string trackid);

vector<string> music_163_get_songs(string song,string artist);

#endif
