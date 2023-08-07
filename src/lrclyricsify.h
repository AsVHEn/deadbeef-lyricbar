#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>

#include "utils.h"

#ifdef __cplusplus

#include <glibmm/ustring.h>

using namespace std;

extern bool syncedlyrics;

string spotify(string artist, string song);

vector<string> spotify_get_songs(string song,string artist);

string lyrics_downloader(string trackid);

#endif
