#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>

#include "utils.h"
#include "lrcspotify.h"

#ifdef __cplusplus


using namespace std;

extern bool syncedlyrics;

vector<string> azlyrics_get_songs(string song,string artist);

struct parsed_lyrics azlyrics(string artist, string song);

struct parsed_lyrics azlyrics_lyrics_downloader(string trackid);

#endif
