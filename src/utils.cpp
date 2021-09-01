#include "utils.h"

#include <sys/stat.h>
#include <curl/curl.h>

#include <algorithm>
#include <cassert>
#include <cctype> // ::isspace
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <pthread.h>
#include <mutex>

#include <giomm.h>
#include <glibmm/fileutils.h>
#include <glibmm/uriutils.h>

#include "debug.h"
#include "gettext.h"
#include "ui.h"

using namespace std;
using namespace Glib;

const DB_playItem_t *last;

static const char *home_cache = getenv("XDG_CACHE_HOME");
static const std::string lyrics_dir = (home_cache ? string(home_cache) : string(getenv("HOME")) + "/.cache")
                               + "/deadbeef/lyrics/";

std::string txt = lyrics_dir + "lyric.txt";
const char *lyrictxt = txt.c_str();
std::string otxt = lyrics_dir + "options.txt";
const char *optionstxt = otxt.c_str();


std::mutex mtx;

bool lyricstart = false;
bool syncedfound = true;
bool syncedlyrics = false;
std::string str;
float length;
float pos;
int paddstodo = 0;
int paddinglines = 0;

vector<int> linessizes;

struct sync{
	vector<ustring> synclyrics;
	vector<double> position;
};

struct sync lrc;

struct chopped{
	ustring past;
	ustring present;
	ustring future;
};

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

void swap(double *xp, double *yp)
{
    double temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void swaptext(ustring *xp, ustring *yp)
{
    ustring temp = *xp;
    *xp = *yp;
    *yp = temp;
}
 
// A function to bubble sort lyrics and timestamps.
struct sync bubbleSort(vector<double> arr, vector<ustring> text ,  int n)
{
	if ((arr.size() == 0) || (text.size() != arr.size())){
		vector<double> nullposition;
		vector<ustring> errortext;
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
struct sync lyric2vector( ustring lyrics){
	cout << "lyric2vector" "\n";
	vector<ustring> synclyrics;
	vector<double> position;
	ustring line;
	int squarebracket;
	int repeats = 0;
		
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
//    	std::cout << *k << " ";

//	for (auto i = goodlyrics.synclyrics.begin(); i != goodlyrics.synclyrics.end(); ++i)
//    	std::cout << *i << '\n';
	
	return goodlyrics;
}

// Function to remark line:
void write_synced( DB_playItem_t *it){
	pos = deadbeef->streamer_get_playpos();
	ustring past = "";
	ustring present = "";
	ustring future = "";
	ustring padding = "";
	int presentpos = 0;
	int minimuntopad = 0;
	int numlines = deadbeef->conf_get_int("lyrics.paddinglines", 15);

	if ( lrc.position.size() > 2) {

		for (unsigned i = 0; i < lrc.position.size()-2; i++){
			if (lrc.position[i] < pos){
				if (lrc.position[i+1] > pos){
					present = lrc.synclyrics[i] + "\n";
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

// Loop to update lyrics on real time.
void thread_listener(DB_playItem_t *track){

	if (is_playing(track)){
		nanosleep(&ts, NULL);
		write_synced(track);
		thread_listener(track);
	}
	else{
		pthread_exit (NULL);

	}


}
void chopset_lyrics(DB_playItem_t *track, ustring lyrics){
	cout << "Chopset lyrics" "\n";
	lrc = lyric2vector(lyrics);
	DB_playItem_t *it = deadbeef->streamer_get_playing_track();
	float length = deadbeef->pl_get_item_duration(it);
	lrc.position.push_back((float)length -0.2);
	lrc.position.push_back((float)length);
	lrc.synclyrics.push_back("\n");
	lrc.synclyrics.push_back("\n");
	mtx.lock();
	std::thread t1(thread_listener, it);
	t1.detach();
	if (track == it){
		linessizes = sizelines(track, lyrics);
	}
	deadbeef->pl_item_unref(it);

	mtx.unlock();
	
}


// Function to capture html.

void get_page(std::string url , const char* file_name) {
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

// Function to lowercase and remove special characters.

std::string shorter(char* text) {

	std::string temp = "";
	std::string counter = std::string(text);
	transform(counter.begin(), counter.end(), counter.begin(), ::tolower);

	for (unsigned i = 0; i <  counter.size(); i++) {
        // Finding the character whose ASCII value fall under this range
        if ( (counter[i] >= 'A' &&  counter[i] <= 'Z') ||  (counter[i] >= 'a' &&  counter[i] <= 'z') ||  (counter[i] >= '0' &&  counter[i] <= '9')) {  
            // erase function to erase the character
            temp = temp + counter[i];
        }
    }
	return temp;
}

std::string specialforspace(const char* text) {
	std::string counter = std::string(text);
	for(unsigned i = 0; i < counter.size(); i++)
    {
        if( (counter[i] < 'A' ||  counter[i] > 'Z') &&  (counter[i] < 'a' ||  counter[i] > 'z') &&  (counter[i] < '0' ||  counter[i] > '9')) {  
            counter[i] = ' ';
		}
    }
    return counter;
}

// Function to extract lyrics from an azlyrics html.

void azlyrics(std::ifstream &file, std::ofstream &outFile){
	lyricstart = false;
	while (std::getline(file, str)) {

		if (4 < str.length() && str.substr(str.length() - 5) == "<br>\r"){
		str.resize(str.size() - 1);
		}

		if (3 < str.length() && str.substr(str.length() - 4) == "<br>" && lyricstart == true){
			str.resize(str.size() - 4);
			outFile << str << std::endl;
		}
		else{
			if (lyricstart == true){
				outFile << str << std::endl;
				remove( lyrictxt);
				break;
			}
		}
		if (str.compare(0,6,"<!-- U") == 0){
			lyricstart = true;
		}
	}
}

// Function to extract lyrics from a syair html.

void syair(std::ifstream &file, std::ofstream &outFile, std::string artisttitle){

	std::string outFileName = lyrics_dir + artisttitle + ".lrc";

	// Possible urls from website search results.

	std::ofstream options(optionstxt);
	while (std::getline(file, str)) {
		istringstream iss(str);
		while ( getline( iss, str, ' ' ) ) {
			if (std::string(str.c_str()).compare(0,13,std::string ("href=" "\"" "/lyrics", 13) ) == 0){
				options << std::string(str.c_str()) << std::endl;
			}
		}
	}

	// Download first search result.	

	std::ifstream syairoptions(optionstxt);
	std::getline(syairoptions, str);


	if (1 < std::string(str.c_str()).length()){
		str.erase(0,6);
		str.erase(str.size() - 1);
		for (unsigned i = 0; i <  str.size()-2; i++) {
			transform(artisttitle.begin(), artisttitle.end(), artisttitle.begin(), ::tolower);
			if (str[i + 8] != '-'){
            	if (str[i + 8] == artisttitle[i]){
				}
				else if ((str[i + 8] == '/') && (str[i + 9] == artisttitle[i+3]) && (str[i + 10] == artisttitle[i+4])){
					std::string url = "https://www.syair.info" + std::string(str);
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
	
	std::ifstream syairraw(lyrictxt);
	while (std::getline(syairraw, str)) {
		if (str.compare(0,4,"[ar:") == 0 || str.compare(0,4,"[ti:") == 0  ){
			lyricstart = true;
		}
		if (lyricstart == true){
			istringstream iss(str);
			while ( getline( iss, str, ' ' ) ) {

			if (4 < std::string(str.c_str()).length() && std::string(str.c_str()).substr(std::string(str.c_str()).length() - 5) == "<br>\r") {
				str.resize(str.size() - 5);
				outFile << str << std::endl;
			}

			else if (3 < std::string(str.c_str()).length() && std::string(str.c_str()).substr(std::string(str.c_str()).length() - 4) == "<div"){
				str.resize(str.size() - 4);
				outFile << str << std::endl;
				break;

			}
			else if (3 < std::string(str.c_str()).length() && std::string(str.c_str()).substr(std::string(str.c_str()).length() - 4) != "<br>"){
				outFile << str << " " ;
			}
			else if (3 >= std::string(str.c_str()).length()){
				outFile << str << " ";
			}
		}
		}
	}
}








static experimental::optional<ustring>(*const providers[])(DB_playItem_t *) = {&get_lyrics_from_script, &download_lyrics_from_syair};

inline string cached_filename(string artist, string title) {
	replace(artist.begin(), artist.end(), '/', '_');
	replace(title.begin(), title.end(), '/', '_');
	if (syncedlyrics == true){
	return lyrics_dir + artist + " - " + title + ".lrc";
	}
	else {
	return lyrics_dir + artist + " - " + title + ".txt";
	}
}

extern "C"
bool is_cached(const char *artist, const char *title) {
	return artist && title && access(cached_filename(artist, title).c_str(), 0) == 0;
}

extern "C"
void ensure_lyrics_path_exists() {
	mkpath(lyrics_dir, 0755);
}

/**
 * Loads the cached lyrics
 * @param artist The artist name
 * @param title  The song title
 * @note         Have no idea about the encodings, so a bug possible here
 */
experimental::optional<ustring> load_cached_lyrics(const char *artist, const char *title) {
	string filename = cached_filename(artist, title);
	debug_out << "filename = '" << filename << "'\n";
	try {
		return {file_get_contents(filename)};
	} catch (const FileError& error) {
		debug_out << error.what();
		return {};
	}
}

bool save_cached_lyrics(const string &artist, const string &title, const string &lyrics) {
	string filename = cached_filename(artist, title);
	std::cout << "save_cached sync: " << syncedlyrics << "\n";
	ofstream t(filename);
	if (!t) {
		cerr << "lyricbar: could not open file for writing: " << filename << endl;
		return false;
	}
	t << lyrics;
	return true;
}


static
experimental::optional<ustring> get_lyrics_from_metadata(DB_playItem_t *track) {
	pl_lock_guard guard;
	syncedlyrics = true;
	const char *lyrics = deadbeef->pl_find_meta(track, "lyrics");
	if (!lyrics){
		syncedlyrics = false;
		lyrics = deadbeef->pl_find_meta(track, "unsynced lyrics")
	                  ?: deadbeef->pl_find_meta(track, "UNSYNCEDLYRICS");
	}
	if (lyrics){
		return ustring{lyrics};
	}
	else return {};
}

experimental::optional<ustring> get_lyrics_from_script(DB_playItem_t *track) {
	std::string buf = std::string(4096, '\0');
	deadbeef->conf_get_str("lyricbar.customcmd", nullptr, &buf[0], buf.size());
	if (!buf[0]) {
		return {};
	}
	auto tf_code = deadbeef->tf_compile(buf.data());
	if (!tf_code) {
		std::cerr << "lyricbar: Invalid script command!\n";
		return {};
	}
	ddb_tf_context_t ctx{};
	ctx._size = sizeof(ctx);
	ctx.it = track;

	int command_len = deadbeef->tf_eval(&ctx, tf_code, &buf[0], buf.size());
	deadbeef->tf_free(tf_code);
	if (command_len < 0) {
		std::cerr << "lyricbar: Invalid script command!\n";
		return {};
	}

	buf.resize(command_len);

	std::string script_output;
	int exit_status = 0;
	try {
		spawn_command_line_sync(buf, &script_output, nullptr, &exit_status);
	} catch (const Glib::Error &e) {
		std::cerr << "lyricbar: " << e.what() << "\n";
		return {};
	}

	if (script_output.empty() || exit_status != 0) {
		return {};
	}

	auto res = ustring{std::move(script_output)};
	if (!res.validate()) {
		cerr << "lyricbar: script output is not a valid UTF8 string!\n";
		return {};
	}
	return {std::move(res)};
}

void update_lyrics(void *tr) {
	DB_playItem_t *track = static_cast<DB_playItem_t*>(tr);
	linessizes.clear();
	if (auto lyrics = get_lyrics_from_metadata(track)) {
		if (syncedlyrics == true){
			chopset_lyrics(track, *lyrics);
		}
		else{
			set_lyrics(track, "", "", *lyrics, "");
		}

		return;
	}

	const char *artist;
	const char *title;
	{
		pl_lock_guard guard;
		artist = deadbeef->pl_find_meta(track, "artist");
		title  = deadbeef->pl_find_meta(track, "title");
	}

	if (artist && title) {
		if (auto lyrics = load_cached_lyrics(artist, title)) {
		if (syncedlyrics == true){
			chopset_lyrics(track, *lyrics);
		}
		else{
			set_lyrics(track, "", "", *lyrics, "");
		}
			return;
	}

		set_lyrics(track, "", "", _("Loading..."), "");

		// No lyrics in the tag or cache; try to get some and cache if succeeded


		std::string artistnospecial = specialforspace(artist);
		std::string titlenoespecial =  specialforspace(title);
		std::string artistandtitle = artistnospecial + " - " + titlenoespecial;
		std::string synclyrics = lyrics_dir + artistnospecial + " - " + titlenoespecial + ".lrc";
		std::replace( artistnospecial.begin(), artistnospecial.end(), ' ', '+');
		std::replace( titlenoespecial.begin(), titlenoespecial.end(), ' ', '+');
		std::string url = "https://www.syair.info/search?q=" + artistnospecial + "+" + titlenoespecial;
		std::ofstream outsyair(synclyrics);
		get_page( url , lyrictxt);
		std::ifstream syairraw(lyrictxt);
		syair(syairraw, outsyair, artistandtitle);
		if (syncedfound == true){
			experimental::optional<ustring> lyrics = file_get_contents(synclyrics);
			chopset_lyrics(track, *lyrics);
			save_cached_lyrics(artist, title, *lyrics);
			return;
		}

	}
	ustring info = "";
    int count  = deadbeef->pl_find_meta_int(track, "PLAY_COUNTER", -1);
    const char *las  = deadbeef->pl_find_meta (track, "LAST_PLAYED");
    const char *firs  = deadbeef->pl_find_meta (track, "FIRST_PLAYED");
	int bitrate = deadbeef->streamer_get_apx_bitrate();
	int playcount = deadbeef->playqueue_get_count();

	info.append("\n \n \n \n");


	if (count == -1){
		count = 0;
	}

	if (count == 1){
		info.append("Escuchado una vez \n \n \n");
	}
	else {
		info.append("Escuchado ");
		info.append(std::to_string(count));
		info.append(" veces \n \n \n");
	}

	if (firs != NULL) {
		ustring first = firs;	
		info.append("Por primera vez el ");
		info.append(first.substr(0,10));
		info.append("\n a las ");
		info.append(first.substr(11, first.length() -1));
		info.append("\n \n \n");
	}
	else{
		info.append("No se ha escuchado previamente \n \n \n");
	}

	if (las != NULL) {
		ustring last = las;
		info.append("Por última vez el ");
		info.append(last.substr(0,10));
		info.append("\n a las ");
		info.append( last.substr(11, last.length() -1));
		info.append("\n \n \n");
	}


	info.append("Bitrate: ");
	info.append(std::to_string(bitrate));
	info.append(" kbps\n \n \n");
	if (playcount == 1){
		info.append("Hay un archivo en cola\n \n \n");
	}
	else{
	info.append("Hay ");
	info.append(std::to_string(playcount));
	info.append(" archivos en cola\n \n \n");
	}


	set_lyrics(track, info, "",  "", "");
}

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
		pl_lock_guard guard;

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
				deadbeef->pl_item_unref(current);
				current = next;
			}
			deadbeef->plt_unref(playlist);
		}
	}
	return 0;
}
