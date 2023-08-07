#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>

#include "utils.h"

#ifdef __cplusplus


using namespace std;

extern bool syncedlyrics;

struct parsed_lyrics spotify(string artist, string song);

struct parsed_lyrics spotify_lyrics_downloader(string trackid);

string timestamps(int number);

#endif
