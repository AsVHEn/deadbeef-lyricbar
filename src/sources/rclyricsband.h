#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>

#include "../utils.h"
#include "lrcspotify.h"

#ifdef __cplusplus


using namespace std;

extern bool syncedlyrics;

vector<string> rclyricsband_get_songs(string song,string artist);

struct parsed_lyrics rclyricsband(string artist, string song);

struct parsed_lyrics rclyricsband_lyrics_downloader(string url);

#endif
