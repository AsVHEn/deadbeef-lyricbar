#include "utils.h"
#include "debug.h"
#include "gettext.h"
#include "lrcspotify.h"
#include "ui.h"

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <pthread.h>
#include <mutex>


using namespace std;

//Global variables:

//static const char *home_cache = getenv("XDG_CACHE_HOME");
//static const string lyrics_dir = (home_cache ? string(home_cache) : string(getenv("HOME")) + "/.cache") + "/deadbeef/lyrics/";



mutex mtx;

int death_signal = 0;
bool lyricstart = false;
bool syncedlyrics = false;

vector<int> linessizes;


struct sync lrc;

struct timespec ts = {0, 50000000};

// For debug:

bool is_playing(DB_playItem_t *track) {
	DB_playItem_t *pl_track = deadbeef->streamer_get_playing_track();
	if ((pl_track) && (pl_track != track)){
		deadbeef->pl_item_unref(pl_track);
		return false;
	}
	else if (!pl_track){
		return false;
	}
	else{
	deadbeef->pl_item_unref(pl_track);
	return pl_track == track;
	}
}

//---------------------------------------------------------------------
//************ Functions to make scroll on sync lyrics. ***************
//---------------------------------------------------------------------

// Functions to bubble sort lyrics and timestamps.
void swap(double *xp, double *yp)
{
    double temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void swaptext(string *xp, string *yp)
{
    string temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// Fuctions to order lyrics and timestamps.
struct sync bubbleSort(vector<double> arr, vector<string> text ,  int n)
{
	if ((arr.size() == 0) || (text.size() != arr.size())){
		vector<double> nullposition;
		vector<string> errortext;
		nullposition.push_back(0);
		errortext.push_back("Error with lyrics file or metadata");
		cout << "Error with lyrics file or metadata \n";
		struct sync error = {errortext, nullposition};
		return error;
	}
	else{
    	int i, j;
    	for (i = 0; i < n-1; i++){   
    		// Last i elements are already in place.
    		for (j = 0; j < n-i-1; j++){
        		if (arr[j] > arr[j+1]){
        		    swap(&arr[j], &arr[j+1]);
					swaptext(&text[j], &text[j+1]);
				}
			}
		}
	struct sync retur = {text,arr};
	return retur;
	}
}

// Function to parse synced lyrics:
struct sync lyric2vector( string lyrics){
	//cout << "lyric2vector" "\n";
	vector<string> synclyrics;
	vector<double> position;
	string line;
	int squarebracket;
	int repeats = 0; 
	//If last character is a ] add an space to have same number of lyrics and positions.
	if (lyrics.at(lyrics.length() - 1) == ']'){
		lyrics.push_back(' ');
	}
	if (lyrics.length() <= 3){
		position.push_back(0);
		struct sync  emptylyrics = bubbleSort(position, synclyrics, 1);
		return emptylyrics;
	}

	for (unsigned i=0; i < lyrics.length() - 3; ++i){
		if ((lyrics.at(i) == '[') && (lyrics.at(i+1) == '0') && (lyrics.at(i+3) == ':') ) {
  			++repeats;
			position.push_back ((lyrics.at(i + 1) - 48)*600 + (lyrics.at(i + 2) - 48)*60 + (lyrics.at(i + 4) - 48)*10 + (lyrics.at(i + 5) - 48) + (lyrics.at(i + 7) - 48)*0.1 + (lyrics.at(i + 8) - 48)*0.01);
			if ((lyrics.length() > i + 10) && (lyrics.at(i+10) != '[')) {
				line = lyrics.substr(i + 10, lyrics.length() - i - 10);
				squarebracket = line.find_first_of('[');
				if (((lyrics.at(i+8 + squarebracket)) != '\n') && (lyrics.at(i+8 + squarebracket)) != '\r'){
				line = lyrics.substr(i + 10, squarebracket-1);
				}
				else {
					line = lyrics.substr(i + 10, squarebracket-2);
				}
				++repeats;
				while (--repeats ) {
					synclyrics.push_back (line);
				}
					
			}
		}
	}

	int n = position.size();
	struct sync  goodlyrics = bubbleSort(position, synclyrics, n);

// For debug:
//	for (auto k = goodlyrics.position.begin(); k != goodlyrics.position.end(); ++k)
//    	cout << *k << " ";
//
//	for (auto i = goodlyrics.synclyrics.begin(); i != goodlyrics.synclyrics.end(); ++i)
//    	cout << *i << '\n';
	
	return goodlyrics;
}

// Function to scroll and remark line:
void write_synced( DB_playItem_t *it){
	float pos = deadbeef->streamer_get_playpos();
	string past = "";
	string present = "";
	string future = "";
	string padding = "";
	int presentpos = 0;
	int minimuntopad = 0;

	if ( lrc.position.size() > 2) {

		for (unsigned i = 0; i < lrc.position.size()-2; i++){
			if (lrc.position[i] < pos){
				if (lrc.position[i+1] > pos){
					present = " " + lrc.synclyrics[i] + " " + "\n";
					presentpos = i;
				}
			}
			else if (pos < lrc.position[i]){
					future.append(lrc.synclyrics[i] + "\n");
			}
		}

		//Add padding variable at beginning of lyrics to show to make scroll with first lines.
		if (!linessizes.empty()){
			for  (int j = 0; j < (int)((lrc.position[linessizes[1]+1] - pos)/(lrc.position[linessizes[1]+1])*linessizes[0]); j++){
				padding.append("\n");
			}
		}

//cout << "Present position: " << presentpos << " pos: " << pos << " LRC position[1] +1: " << lrc.position[linessizes[1]+1] << " linessizes[0]: " << linessizes[0] << " Operacion: " << ((lrc.position[linessizes[1]+1] - pos)/(lrc.position[linessizes[1]+1])*linessizes[0]) <<"\n";

		//Add padding variable at beginning of lyrics to show to make scroll when removing a past line.
		if (((!linessizes.empty()) && (presentpos - linessizes[1]) > 0)){
			minimuntopad = presentpos - linessizes[1];
			for  (int j = 0 ; j < (int)(((lrc.position[presentpos +1] - pos)/(lrc.position[presentpos+1] -lrc.position[presentpos]))*(linessizes[presentpos - linessizes[1] + 5] -1)); j++){
				padding.append("\n");
			}
		}

		//Removing first past lyrics lines to make scroll.
		for (unsigned i = minimuntopad; lrc.position[i+1] < pos && i < lrc.position.size()-2; i++){
			past.append(lrc.synclyrics[i] + "\n");
		}
	set_lyrics(it, past, present, future, padding);
	}

	else{
		past = " ";
		present = " ";
		future = " ";
		set_lyrics(it, past, present, future, padding);
	}
}

// Main loop to update lyrics on real time.
void thread_listener(DB_playItem_t *track){

	if ((is_playing(track)) && death_signal == 0){
		nanosleep(&ts, NULL);
		write_synced(track);
		thread_listener(track);
	}
	else{
		pthread_exit (NULL);
	}
}

// Main loop thread caller.
void chopset_lyrics(DB_playItem_t *track, string lyrics){
//	cout << "Chopset lyrics" "\n";
	lrc = lyric2vector(lyrics);
	DB_playItem_t *it = deadbeef->streamer_get_playing_track();
	float length = deadbeef->pl_get_item_duration(it);

	lrc.position.push_back((float)length -0.2);
	lrc.position.push_back((float)length);
	lrc.synclyrics.push_back("\n");
	lrc.synclyrics.push_back("\n");
	string prelyrics = "";
	for (unsigned i = 0; i < lrc.synclyrics.size()-2; i++){
			prelyrics.append(lrc.synclyrics[i] + "\n");
	}
	if (track == it){
		linessizes = sizelines(track, prelyrics);
	}

	mtx.lock();
	thread t1(thread_listener, it);
	t1.detach();
	deadbeef->pl_item_unref(it);
	mtx.unlock();
}

//---------------------------------------------------------------------
//*********** /Functions to make scroll on sync lyrics. ***************
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//************ Share functions of lyrics downloaders. *****************
//---------------------------------------------------------------------

//	Function to save lyrics to tag.
void save_meta_data(DB_playItem_t *playing_song, struct parsed_lyrics lyrics){

	deadbeef->pl_lock();
	const char *SYLT = deadbeef->pl_find_meta(playing_song, "SYLT");
	const char *LYRICS = deadbeef->pl_find_meta(playing_song, "LYRICS");
	const char *UNSYNCEDLYRICS = deadbeef->pl_find_meta(playing_song, "UNSYNCEDLYRICS");
	deadbeef->pl_unlock();
	if (lyrics.sync == true){	
		if (SYLT){
			deadbeef->pl_delete_meta(playing_song, "SYLT");
		}

		if (LYRICS){
			deadbeef->pl_delete_meta(playing_song, "LYRICS");
		}
		deadbeef->pl_add_meta(playing_song, "LYRICS", lyrics.lyrics.c_str());	
	}
	else{
		if (UNSYNCEDLYRICS){
			deadbeef->pl_delete_meta(playing_song, "UNSYNCEDLYRICS");
		}
		deadbeef->pl_add_meta(playing_song, "UNSYNCEDLYRICS", lyrics.lyrics.c_str());	
	}
	deadbeef->pl_lock(); 
	const char *dec = deadbeef->pl_find_meta_raw(playing_song, ":DECODER"); 
	char decoder_id[100];

	if (dec){
		strncpy(decoder_id, dec, sizeof(decoder_id));
	}
	int match = playing_song && dec;
	deadbeef->pl_unlock();

	if (match){
		DB_decoder_t *dec = NULL;
		DB_decoder_t **decoders = deadbeef->plug_get_decoder_list();

		for (int i = 0; decoders[i]; i++){

			if (!strcmp(decoders[i]->plugin.id, decoder_id)){
				dec = decoders[i];

				if (dec->write_metadata) {
					dec->write_metadata(playing_song);
				}
				break;
			}
		}
	}
}

// Function to remove special characters.
string specialforplus(const char* text) {
	string counter = string(text);
	for(unsigned i = 0; i < counter.size(); i++)
    {
        if( (counter[i] < 'A' ||  counter[i] > 'Z') &&  (counter[i] < 'a' ||  counter[i] > 'z') &&  (counter[i] < '0' ||  counter[i] > '9')) {  
            counter[i] = '+';
		}
    }
    return counter;
}
// Function to remove replace substrings.
std::string replace_string(string subject, const string& search, const string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
    }
    return subject;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to split strings.
vector<string> split(string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
    res.push_back (s.substr (pos_start));
    return res;
}

// Function to parse webpages.
string text_downloader(curl_slist *slist, string url) {
	CURL *curl_handle;
	curl_handle = curl_easy_init();
	string readBuffer = "";
	if (curl_handle) {	
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 600);
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl_handle, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, false);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, slist);
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_perform(curl_handle);
		curl_easy_cleanup(curl_handle);
	}
	return readBuffer;
}

//---------------------------------------------------------------------
//************ /Share functions of lyrics downloaders. ****************
//---------------------------------------------------------------------

inline string cached_filename(string artist, string title) {
	return 0;
}

extern "C"
bool is_cached(const char *artist, const char *title) {
	return false;
}

extern "C"
void ensure_lyrics_path_exists() {
}

//---------------------------------------------------------------------
//****** Main: Try to get lyrics from Metadata,file,spotify. **********
//---------------------------------------------------------------------

struct parsed_lyrics get_lyrics_next_to_file(DB_playItem_t *track) {
	bool sync = false;
	ifstream infile;
	string lyrics;

	deadbeef->pl_lock();
	const char *track_location = deadbeef->pl_find_meta(track, ":URI");
	deadbeef->pl_unlock();
	string trackstring = track_location;
	size_t lastindex = trackstring.find_last_of(".");
	trackstring = trackstring.substr(0, lastindex);

	infile.open(trackstring + ".lrc", ios_base::trunc); //ios_base::app
	if(infile.is_open()){
		lyrics = infile.get();
		sync = true;	
	}
	else{
		infile.open(trackstring + ".txt", ios_base::trunc); //ios_base::app
	}

	if(infile.is_open()){
		lyrics = infile.get();
	}
	return{lyrics, sync};	
}

struct parsed_lyrics get_lyrics_from_metadata(DB_playItem_t *track) {
	syncedlyrics = true;
	deadbeef->pl_lock();
	const char *lyrics = deadbeef->pl_find_meta(track, "lyrics")
							?: deadbeef->pl_find_meta(track, "SYLT");
	if (!lyrics) {
		syncedlyrics = false;
		lyrics = deadbeef->pl_find_meta(track, "unsynced lyrics")
	                  ?: deadbeef->pl_find_meta(track, "UNSYNCEDLYRICS");
		if (!lyrics) {
			lyrics = "";
		}
	}
	deadbeef->pl_unlock();
	string string_lyrics = lyrics;
	return {string_lyrics, syncedlyrics};
}


void save_next_to_file(struct parsed_lyrics lyrics, DB_playItem_t *track) {
	deadbeef->pl_lock();
	const char *track_location = deadbeef->pl_find_meta(track, ":URI");
	deadbeef->pl_unlock();
	ofstream outfile;
	string trackstring = track_location;
	size_t lastindex = trackstring.find_last_of(".");
	trackstring = trackstring.substr(0, lastindex);
	if (lyrics.sync == true) {
		try {
		outfile.open(trackstring + ".lrc", ios_base::trunc);//ios_base::app
		}
		catch (...) {
			std::cout << "Unable to write file\n";
		}
	}
	else {
		try {
		outfile.open(trackstring + ".txt", ios_base::trunc);//ios_base::app
		}
		catch (...)	{
			std::cout << "Unable to write file\n";
		}
	}
	outfile << lyrics.lyrics << endl;
	outfile.close();
}

//Sets track info if no lyrics founded.
void set_info(DB_playItem_t *track) {
	string info = "";
	char *locale = setlocale(LC_CTYPE, NULL);
    int count  = deadbeef->pl_find_meta_int(track, "PLAY_COUNTER", -1);
	if (count == -1){
		count  = deadbeef->pl_find_meta_int(track, "play_count", -1);
	}
    const char *las  = deadbeef->pl_find_meta (track, "LAST_PLAYED");
    const char *firs  = deadbeef->pl_find_meta (track, "FIRST_PLAYED");
	int playcount = deadbeef->playqueue_get_count();
	int bitrate = deadbeef->streamer_get_apx_bitrate();
	while (bitrate == -1){
		bitrate = deadbeef->streamer_get_apx_bitrate();
	}


	info.append("\n \n \n \n");

	if (count == -1){
		count = 0;
	}
	
	if (count == 1){
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			info.append("Escuchado una vez \n \n \n");
		}
		else{
			info.append("Listened once \n \n \n");
		}
	}
	else {
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			info.append("Escuchado ");
			info.append(to_string(count));
			info.append(" veces \n \n \n");
		}
		else{
			info.append("Listened ");
			info.append(to_string(count));
			info.append(" times \n \n \n");
		}

	}

	if (firs != NULL) {
		string first = firs;	
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			info.append("Por primera vez el ");
			info.append(first.substr(0,10));
			info.append("\n a las ");
			info.append(first.substr(11, first.length() -1));
			info.append("\n \n \n");
		}
		else{
			info.append("For the first time ");
			info.append(first.substr(0,10));
			info.append("\n at ");
			info.append(first.substr(11, first.length() -1));
			info.append("\n \n \n");
		}
	}
	else{
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			info.append("No se ha escuchado previamente \n \n \n");
		}
		else{
			info.append("Never listened before \n \n \n");
		}
	}

	if (las != NULL) {
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			string last = las;
			info.append("Por última vez el ");
			info.append(last.substr(0,10));
			info.append("\n a las ");
			info.append( last.substr(11, last.length() -1));
			info.append("\n \n \n");
		}
		else{
			string last = las;
			info.append("For the last time the");
			info.append(last.substr(0,10));
			info.append("\n at ");
			info.append( last.substr(11, last.length() -1));
			info.append("\n \n \n");
		}
	}


	info.append("Bitrate: ");
	info.append(to_string(bitrate));
	info.append(" kbps\n \n \n");
	if (playcount == 1){
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			info.append("Hay un archivo en cola\n \n \n");
		}
		else{
			info.append("There is one file in line \n \n \n");
		}
	}
	else{
		if (strcmp(locale , "es_ES.UTF-8") == 0) {
			info.append("Hay ");
			info.append(to_string(playcount));
			info.append(" archivos en cola\n \n \n");
		}
		else{
			info.append("There are \n \n \n");
			info.append(to_string(playcount));
			info.append(" files in line\n \n \n");
		}
	}

	set_lyrics(track, info, "",  "", "");
}

//Main function:
void update_lyrics(void *tr) {
	linessizes.clear();
	DB_playItem_t *track = static_cast<DB_playItem_t*>(tr);
	struct parsed_lyrics meta_lyrics = get_lyrics_from_metadata(track);
	if (meta_lyrics.lyrics != "") {
		if (meta_lyrics.sync == true) {
			chopset_lyrics(track, meta_lyrics.lyrics);
		}
		else{
			set_lyrics(track, "", "", meta_lyrics.lyrics, "");
		}
		sync_or_unsync(syncedlyrics);
	return;
	}

	const char *artist;
	const char *title;
	{
		deadbeef->pl_lock();
		artist = deadbeef->pl_find_meta(track, "artist");
		title  = deadbeef->pl_find_meta(track, "title");
		deadbeef->pl_unlock();
	}



	if (artist && title) {
		struct parsed_lyrics cached_lyrics = get_lyrics_next_to_file(track);

		if (cached_lyrics.lyrics != "") {
			if (cached_lyrics.sync == true) {
				chopset_lyrics(track, cached_lyrics.lyrics);
			}
			else{
				set_lyrics(track, "", "", cached_lyrics.lyrics, "");
			}
			sync_or_unsync(syncedlyrics);
			return;
		}
	}
	set_lyrics(track, "", "", _("Loading..."), "");

//	 No lyrics in the tag or cache; try to get some and cache if succeeded.
//	Search for lyrics on spotify:	
	struct parsed_lyrics spoty_lyrics = {"",false};
	spoty_lyrics = spotify(specialforplus(title), specialforplus(artist));

	if (spoty_lyrics.lyrics != "") {
//		save_next_to_file(spoty_lyrics, track);
		if (spoty_lyrics.sync){
			chopset_lyrics(track, spoty_lyrics.lyrics);

		}
		else{
			set_lyrics(track, "", "", spoty_lyrics.lyrics, "");
		}
		sync_or_unsync(syncedlyrics);
		save_meta_data(track, spoty_lyrics);
		return;
	}

//	If no lyrics founded in any site, show track info:
	set_info(track);
}

//---------------------------------------------------------------------
//****** /Main: Try to get lyrics from Metadata,file,spotify. *********
//---------------------------------------------------------------------


/**
 * Creates the directory tree.
 * @param name the directory path, including trailing slash
 * @return 0 on success; errno after mkdir call if something went wrong
 */

int mkpath(const string &name, mode_t mode) {
	string dir;
	size_t pos = 0;
	while ((pos = name.find_first_of('/', pos)) != string::npos){
		dir = name.substr(0, pos++);
		if (dir.empty())
			continue; // ignore the leading slash
		if (mkdir(dir.c_str(), mode) && errno != EEXIST)
			return errno;
	}
	return 0;
}

int remove_from_cache_action(DB_plugin_action_t *, int ctx) {
	if (ctx == DDB_ACTION_CTX_SELECTION) {
		deadbeef->pl_lock();
		ddb_playlist_t *playlist = deadbeef->plt_get_curr();
		if (playlist) {
			DB_playItem_t *current = deadbeef->plt_get_first(playlist, PL_MAIN);
			while (current) {
				if (deadbeef->pl_is_selected (current)) {
					const char *artist = deadbeef->pl_find_meta(current, "artist");
					const char *title  = deadbeef->pl_find_meta(current, "title");
					if (is_cached(artist, title))
						remove(cached_filename(artist, title).c_str());
				}
				DB_playItem_t *next = deadbeef->pl_get_next(current, PL_MAIN);
				deadbeef->pl_unlock();
				deadbeef->pl_item_unref(current);
				current = next;
			}
			deadbeef->plt_unref(playlist);
		}
	}
	return 0;
}
