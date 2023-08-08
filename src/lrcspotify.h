#pragma once
#include <deadbeef/deadbeef.h>
#include "utils.h"

#include <gtk/gtk.h>

#ifdef __cplusplus
#include <vector>
#include <string>


using namespace std;

extern bool syncedlyrics;

struct parsed_lyrics spotify(string artist, string song);

struct parsed_lyrics spotify_lyrics_downloader(string trackid);

vector<string> spotify_get_songs(string song,string artist);

string timestamps(int number);

#endif
