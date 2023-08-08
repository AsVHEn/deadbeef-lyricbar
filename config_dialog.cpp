#include "main.h"
#include "ui.h"
#include "utils.h"
#include "lrcspotify.h"
#include "megalobiz.h"
#include "azlyrics.h"
#include <deadbeef/deadbeef.h>
		
#include <filesystem>
#include <vector>
#include <thread>
#include <future>		
#include <iostream>


// - Global

char *home_cache = getenv("HOME");
char *locale_lang = setlocale(LC_CTYPE, NULL);
string local = "/.local/lib/deadbeef/panels.glade";
string Glade_file_path = home_cache + local;


// - Config widgets

GtkWindow		*ConfigWindow;
GtkBuilder		*builder;
GtkColorButton	*Background_color;
GtkColorButton	*Highlight_color;
GtkColorButton	*Text_color;
GtkRadioButton	*Align_left;
GtkRadioButton	*Align_center;
GtkRadioButton	*Align_right;
GtkSpinButton	*Highlight_line;
GtkCheckButton	*Check_bold;
GtkSpinButton	*Edge;
GtkButton		*Apply;
GtkButton		*Cancel;
GtkButton		*OK;
GtkLabel		*Background_color_label;
GtkLabel		*Text_color_label;
GtkLabel		*Highlight_color_label;
GtkLabel		*Align_label;
GtkLabel		*Highlight_line_label;
GtkLabel		*Edge_label;

int Align_int_value;
int Check_bold_int_value;

// - Search widgets

GtkWindow			*SearchWindow;
GtkButton			*Save;
GtkButton			*Search;
GtkButton			*Exit;
GtkEntry			*Artist_input;
GtkEntry			*Song_input;
GtkEntry			*Album_input;
GtkTreeStore		*treeStore;
GtkTreeView			*SongsTree;
GtkTreeViewColumn	*Column_artist;
GtkTreeViewColumn	*Column_song;
GtkTreeViewColumn	*Column_album;
GtkTreeViewColumn	*Column_source;
GtkTreeViewColumn	*Column_searchid;
GtkTreeSelection	*selection;
GtkCellRenderer		*artists;
GtkCellRenderer		*songs;
GtkCellRenderer		*albums;
GtkCellRenderer		*sources;
GtkCellRenderer		*searchids;
GtkLabel			*PreViewLyrics;
GtkLabel			*Artist_label;
GtkLabel			*Song_label;
GtkLabel			*Album_label;

struct parsed_lyrics selected_lyrics = {"",false};

DB_playItem_t *it;
GtkTreeIter iter_populate;

// - Edit widgets

GtkWindow			*EditWindow;
GtkButton			*Edit_apply;
GtkButton			*Edit_cancel;
GtkButton			*Edit_OK;
GtkTextView			*Text_view_lyrics_editor;
GtkLabel			*Playback_time;
GtkButton			*Plus_time;
GtkButton			*Minus_time;
GtkButton			*Apply_offset;
GtkButton			*Insert_timestamp;
GtkCheckButton		*Check_sync;
GtkButton			*Backward;
GtkButton			*Pause_play;
GtkButton			*Forward;
GtkLabel			*Sync_label;
GtkLabel			*Sync_or_not_label;


GtkTextBuffer		*refBuffer;

//---------------------------------------------------------------------
//******************* ConfigWindow ************************************
//---------------------------------------------------------------------

void	on_Background_color_color_set (GtkColorButton *c) {
	GdkRGBA color;
	int Red, Green, Blue;
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(c), &color);
	Red = color.red*255 + 0.5;
	Green = color.green*255 + 0.5;
	Blue = color.blue*255 + 0.5;
	char hexColor[8];
    std::snprintf(hexColor, sizeof hexColor, "#%02x%02x%02x", Red, Green, Blue);
	deadbeef->conf_set_str("lyricbar.backgroundcolor", hexColor);
}

void	on_Highlight_color_color_set (GtkColorButton *c) {
	GdkRGBA color;
	int Red, Green, Blue;
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(c), &color);
	Red = color.red*255 + 0.5;
	Green = color.green*255 + 0.5;
	Blue = color.blue*255 + 0.5;
	char hexColor[8];
    std::snprintf(hexColor, sizeof hexColor, "#%02x%02x%02x", Red, Green, Blue);
	deadbeef->conf_set_str("lyricbar.highlightcolor", hexColor);
	}

void	on_Text_color_color_set (GtkColorButton *c) {
	GdkRGBA color;
	int Red, Green, Blue;
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(c), &color);
	Red = color.red*255 + 0.5;
	Green = color.green*255 + 0.5;
	Blue = color.blue*255 + 0.5;
	char hexColor[8];
    std::snprintf(hexColor, sizeof hexColor, "#%02x%02x%02x", Red, Green, Blue);
	deadbeef->conf_set_str("lyricbar.regularcolor", hexColor);
	}


void	on_Align_toggled(GtkRadioButton *b) {
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b))){
		if (strcmp(gtk_button_get_label(GTK_BUTTON(b)), "Left") == 0){
			Align_int_value	= 0;
		}
		else if (strcmp(gtk_button_get_label(GTK_BUTTON(b)), "Center") == 0){
			Align_int_value	= 1;
		}
		else if (strcmp(gtk_button_get_label(GTK_BUTTON(b)), "Right") == 0){
			Align_int_value	= 2;
		}
	}
}

void	on_check_bold_toggled(GtkCheckButton *b) {
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b))){
		deadbeef->conf_set_int("lyricbar.bold", 1);
	}
	else{
		deadbeef->conf_set_int("lyricbar.bold", 0);
	}
}

void	on_Cancel_clicked (GtkButton *b, gpointer user_data) {
	gtk_window_close(GTK_WINDOW(user_data));
}

void	on_Apply_clicked (GtkButton *b) {
	deadbeef->conf_set_int("lyricbar.lyrics.alignment", Align_int_value);
	deadbeef->conf_set_int("lyricbar.vpostion", gtk_spin_button_get_value (GTK_SPIN_BUTTON(Highlight_line)));
	deadbeef->conf_set_int("lyricbar.border", gtk_spin_button_get_value (GTK_SPIN_BUTTON(Edge)));
	printf("%s \n", "Apply clicked");
	get_tags();
	//gtk_label_set_text (GTK_LABEL(Title), (const gchar* ) "Apply");
}

void	on_OK_clicked (GtkButton *b, gpointer user_data) {
	on_Apply_clicked (b);
	gtk_window_close(GTK_WINDOW(user_data));
	//gtk_label_set_text (GTK_LABEL(Title), (const gchar* ) "OK");
}

void set_chooser_color(GtkColorButton *chooser, const char *hexColor ){
	int red,green,blue;
	sscanf(hexColor, "#%02x%02x%02x", &red, &green, &blue);
	double Double_red = red;
	Double_red = Double_red/255;
	double Double_green = green;
	Double_green = Double_green/255;
	double Double_blue = blue;
	Double_blue = Double_blue/255;
	double Alpha = 1;

	const GdkRGBA* color = new GdkRGBA{Double_red,Double_green,Double_blue,Alpha};
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(chooser),color);
	
}

extern "C"
int on_button_config (GtkMenuItem *menuitem, gpointer user_data) {

//---------------------------------------------------------------------
// establish contact with xml code used to adjust widget settings
//---------------------------------------------------------------------

	builder = gtk_builder_new_from_file (Glade_file_path.c_str());
 
	ConfigWindow = GTK_WINDOW(gtk_builder_get_object(builder, "ConfigWindow"));

//	Widgets creation:
	Background_color 		= GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "Background_color"));
	Highlight_color 		= GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "Highlight_color"));
	Text_color 				= GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "Text_color"));
	Highlight_line 			= GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "Highlight_line"));
	Check_bold				= GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "Check_bold"));
	Edge 					= GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "Edge"));
	Align_left 				= GTK_RADIO_BUTTON(gtk_builder_get_object(builder, "Align_left"));
	Align_center 			= GTK_RADIO_BUTTON(gtk_builder_get_object(builder, "Align_center"));
	Align_right 			= GTK_RADIO_BUTTON(gtk_builder_get_object(builder, "Align_right"));
	OK 						= GTK_BUTTON(gtk_builder_get_object(builder, "OK"));
	Cancel 					= GTK_BUTTON(gtk_builder_get_object(builder, "Cancel"));
	Apply 					= GTK_BUTTON(gtk_builder_get_object(builder, "Apply"));
	Background_color_label	= GTK_LABEL(gtk_builder_get_object(builder, "Background_color_label"));
	Text_color_label		= GTK_LABEL(gtk_builder_get_object(builder, "Text_color_label"));
	Highlight_color_label	= GTK_LABEL(gtk_builder_get_object(builder, "Highlight_color_label"));
	Align_label				= GTK_LABEL(gtk_builder_get_object(builder, "Align_label"));
	Edge_label				= GTK_LABEL(gtk_builder_get_object(builder, "Edge_label"));
	Highlight_line_label	= GTK_LABEL(gtk_builder_get_object(builder, "Highlight_line_label"));


//	Translation:
	if (strcmp(locale_lang , "es_ES.UTF-8") != 0) {
		gtk_label_set_label(Background_color_label, "Background color: ");
		gtk_label_set_label(Text_color_label, "Text color: ");
		gtk_label_set_label(Highlight_color_label, "Highlight color: ");
		gtk_label_set_label(Align_label, "Align: ");
		gtk_label_set_label(Edge_label, "Edge: ");
		gtk_label_set_label(Highlight_line_label, "Highlight line height %: ");
		gtk_button_set_label(GTK_BUTTON(Check_bold), "Bold");
		gtk_button_set_label(GTK_BUTTON(Align_left), "Left");
		gtk_button_set_label(GTK_BUTTON(Align_center), "Center");
		gtk_button_set_label(GTK_BUTTON(Align_right), "Right");
		gtk_window_set_title(ConfigWindow, "Config");
	}

	gtk_radio_button_join_group(Align_center, Align_left);
	gtk_radio_button_join_group(Align_right, Align_left);

//	Signals:
	g_signal_connect(ConfigWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_builder_connect_signals(builder, NULL);
	g_signal_connect(Background_color, "color-set", G_CALLBACK(on_Background_color_color_set), NULL);
	g_signal_connect(Highlight_color, "color-set", G_CALLBACK(on_Highlight_color_color_set), NULL);
	g_signal_connect(Text_color, "color-set", G_CALLBACK(on_Text_color_color_set), NULL);
	g_signal_connect(Align_left, "toggled", G_CALLBACK(on_Align_toggled), NULL);
	g_signal_connect(Align_center, "toggled", G_CALLBACK(on_Align_toggled), NULL);
	g_signal_connect(Align_right, "toggled", G_CALLBACK(on_Align_toggled), NULL);
	g_signal_connect(Check_bold, "toggled", G_CALLBACK(on_check_bold_toggled), NULL);
	g_signal_connect(OK, "clicked", G_CALLBACK(on_OK_clicked), ConfigWindow);
	g_signal_connect(Cancel, "clicked", G_CALLBACK(on_Cancel_clicked), ConfigWindow);
	g_signal_connect(Apply, "clicked", G_CALLBACK(on_Apply_clicked), NULL);


//	Saved color to choosers:
	set_chooser_color(Background_color, deadbeef->conf_get_str_fast("lyricbar.backgroundcolor", "#F6F6F6"));
	set_chooser_color(Highlight_color, deadbeef->conf_get_str_fast("lyricbar.highlightcolor", "#571c1c"));
	set_chooser_color(Text_color, deadbeef->conf_get_str_fast("lyricbar.regularcolor", "#000000"));

// Saved value to spin buttons:
	gtk_spin_button_set_value(Highlight_line, deadbeef->conf_get_int("lyricbar.vpostion", 50));
	gtk_spin_button_set_value(Edge, deadbeef->conf_get_int("lyricbar.border", 22));

// Saved value to check button:

	Check_bold_int_value = deadbeef->conf_get_int("lyricbar.bold", 1);

	if (Check_bold_int_value == 1){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Check_bold), TRUE);
	}
	else{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Check_bold), FALSE);	
	}

// Saved value to radio buttons:
	Align_int_value = deadbeef->conf_get_int("lyricbar.lyrics.alignment", 1);

	if (Align_int_value == 0){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Align_left), TRUE);		
	}
	else if (Align_int_value == 1){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Align_center), TRUE);
	}
	else if (Align_int_value == 2){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Align_right), TRUE);
	}

	
	gtk_widget_show(GTK_WIDGET(ConfigWindow));

	gtk_main();

	return EXIT_SUCCESS;
}

//---------------------------------------------------------------------
//******************* \ConfigWindow ***********************************
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//******************* SearchWindow ************************************
//---------------------------------------------------------------------

string specialtoplus(const char* text) {
	string counter = string(text);
	for(unsigned i = 0; i < counter.size(); i++)
    {
        if( (counter[i] < 'A' ||  counter[i] > 'Z') &&  (counter[i] < 'a' ||  counter[i] > 'z') &&  (counter[i] < '0' ||  counter[i] > '9')) {  
            counter[i] = '+';
		}
    }
    return counter;
}

void populate_tree_view(vector<string> songs, string source) {
	for(size_t i = 0; i < songs.size(); i+=4) {
		gtk_tree_store_append (treeStore, &iter_populate, NULL);
		gtk_tree_store_set(treeStore, &iter_populate, 0, songs[i].c_str(), -1);
		gtk_tree_store_set(treeStore, &iter_populate, 1, songs[i+1].c_str(), -1);
		gtk_tree_store_set(treeStore, &iter_populate, 2, songs[i+2].c_str(), -1);
		gtk_tree_store_set(treeStore, &iter_populate, 3, source.c_str(), -1);
		gtk_tree_store_set(treeStore, &iter_populate, 4, songs[i+3].c_str(), -1);
	}
	
}

void	on_Save_clicked (GtkButton *b, gpointer user_data) {
	deadbeef->pl_lock();
	DB_playItem_t *track = static_cast<DB_playItem_t *>(user_data);
	deadbeef->pl_unlock(); 
	if (track){
		save_meta_data( track, selected_lyrics);
		DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track();
		if (playing_track){
			if (playing_track == track){
				death_signal = 1;
				auto tid = deadbeef->thread_start(update_lyrics, track);
				deadbeef->thread_detach(tid);
			}
			else{
				deadbeef->pl_item_unref(playing_track);
			}
		}
		deadbeef->pl_item_unref(track);
		//gtk_label_set_text (GTK_LABEL(Title), (const gchar* ) "OK");
	}
}


void	on_row_double_clicked (GtkButton *b) {
	selected_lyrics = {"",false};
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(SongsTree));
	gchar *provider, *value;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &iter) == FALSE){
		return;
	}
	gtk_tree_model_get(model, &iter, 3, &provider,  -1);
	gtk_tree_model_get(model, &iter, 4, &value,  -1);

	if (strcmp(provider, "Spotify") == 0){
		selected_lyrics = spotify_lyrics_downloader(value);
	}
	else if (strcmp(provider, "Megalobiz") == 0){
		selected_lyrics = megalobiz_lyrics_downloader(value);
	}
	else if (strcmp(provider, "AZlyrics") == 0){
		cout << "Value: " << value << "\n";
		selected_lyrics = azlyrics_lyrics_downloader(value);
	}

	gtk_label_set_label(PreViewLyrics,selected_lyrics.lyrics.c_str());
}

void thread_listener_slow(string text_song, string text_artist){
	vector<string> megalobiz_songs = megalobiz_get_songs(text_song, text_artist);
	populate_tree_view(megalobiz_songs, "Megalobiz");
}

void thread_listener_fast(string text_song, string text_artist){
	vector<string> spotify_songs = 	spotify_get_songs(text_song,text_artist);
	populate_tree_view(spotify_songs, "Spotify");

	vector<string> azlyrics_songs = azlyrics_get_songs(text_song, text_artist);
	populate_tree_view(azlyrics_songs, "AZlyrics");
}


void	on_Search_clicked (GtkButton *b) {
	gtk_tree_store_clear(treeStore);
	string text_artist, text_song, text_album;

	text_artist = specialtoplus(gtk_entry_get_text(Artist_input));
	text_song = specialtoplus(gtk_entry_get_text(Song_input));
	text_album = specialtoplus(gtk_entry_get_text(Album_input));

	thread t1(thread_listener_fast, text_song,text_artist);
	thread t2(thread_listener_slow, text_song,text_artist);


//	printf("Songs  = %s\n; ", songs);
	//gtk_label_set_text (GTK_LABEL(Title), (const gchar* ) "OK");

	t1.detach();
	t2.detach();

}

void	on_Exit_clicked (GtkButton *b, gpointer user_data) {
	gtk_window_close(GTK_WINDOW(user_data));
}

extern "C"
int on_button_search (GtkMenuItem *menuitem, gpointer user_data) {

	builder = gtk_builder_new_from_file (Glade_file_path.c_str());

	SearchWindow = GTK_WINDOW(gtk_builder_get_object(builder, "SearchWindow"));

//	Widgets creation:
	Artist_input		= GTK_ENTRY(gtk_builder_get_object(builder, "Artist_input"));
	Song_input			= GTK_ENTRY(gtk_builder_get_object(builder, "Song_input"));
	Album_input			= GTK_ENTRY(gtk_builder_get_object(builder, "Album_input"));
	treeStore			= GTK_TREE_STORE(gtk_builder_get_object(builder, "treeStore"));
	SongsTree			= GTK_TREE_VIEW(gtk_builder_get_object(builder, "SongsTree"));
	Column_artist		= GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "Column_artist"));
	Column_song			= GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "Column_song"));
	Column_album		= GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "Column_album"));
	Column_source		= GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "Column_source"));
	Column_searchid		= GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "Column_searchid"));
	artists				= GTK_CELL_RENDERER(gtk_builder_get_object(builder, "artists"));
	songs				= GTK_CELL_RENDERER(gtk_builder_get_object(builder, "songs"));
	albums				= GTK_CELL_RENDERER(gtk_builder_get_object(builder, "albums"));
	sources				= GTK_CELL_RENDERER(gtk_builder_get_object(builder, "sources"));
	searchids			= GTK_CELL_RENDERER(gtk_builder_get_object(builder, "searchids"));
	selection			= GTK_TREE_SELECTION(gtk_builder_get_object(builder, "selection"));
	PreViewLyrics		= GTK_LABEL(gtk_builder_get_object(builder, "PreViewLyrics"));
	Artist_label		= GTK_LABEL(gtk_builder_get_object(builder, "Artist_label"));
	Song_label			= GTK_LABEL(gtk_builder_get_object(builder, "Song_label"));
	Album_label			= GTK_LABEL(gtk_builder_get_object(builder, "Album_label"));
	Save				= GTK_BUTTON(gtk_builder_get_object(builder, "Save"));
	Exit				= GTK_BUTTON(gtk_builder_get_object(builder, "Exit"));
	Search				= GTK_BUTTON(gtk_builder_get_object(builder, "Search"));

//	Translation:
	if (strcmp(locale_lang , "es_ES.UTF-8") != 0) {
		gtk_label_set_text (PreViewLyrics, (const gchar* ) "Lyrics preview");
		gtk_label_set_label(Artist_label, "Artist: ");
		gtk_label_set_label(Song_label, "Song: ");
		gtk_label_set_label(Album_label, "Album ");
		gtk_button_set_label(GTK_BUTTON(Search), "Search");
		gtk_window_set_title(SearchWindow, "Search");
		gtk_tree_view_column_set_title(Column_artist, "Artist");
		gtk_tree_view_column_set_title(Column_song, "Song");
		gtk_tree_view_column_set_title(Column_album, "Album");
		gtk_tree_view_column_set_title(Column_source, "Source");	}
	else{
		gtk_label_set_text (PreViewLyrics, (const gchar* ) "Ventana de previsualización de las letras");
	}

//	Playing track or selected on playlist to labels:
	DB_playItem_t *track = deadbeef->streamer_get_playing_track();
	if (track){
		deadbeef->pl_lock();
		gtk_entry_set_text(Artist_input, deadbeef->pl_find_meta(track, "artist"));
		gtk_entry_set_text(Song_input, deadbeef->pl_find_meta(track, "title"));
		gtk_entry_set_text(Album_input, deadbeef->pl_find_meta(track, "album"));	
		deadbeef->pl_unlock();
		deadbeef->pl_item_unref(track);
	}
	else{
		ddb_playlist_t *plt = deadbeef->plt_get_curr();
		if (plt) {
			int cursor = deadbeef->pl_get_cursor (PL_MAIN);
            track = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
			deadbeef->pl_lock();
			gtk_entry_set_text(Artist_input, deadbeef->pl_find_meta(track, "artist"));
			gtk_entry_set_text(Song_input, deadbeef->pl_find_meta(track, "title"));
			gtk_entry_set_text(Album_input, deadbeef->pl_find_meta(track, "album"));	
			deadbeef->pl_unlock();
			deadbeef->pl_item_unref(track);
			deadbeef->plt_unref(plt);
		}
	}
	

	gtk_tree_view_set_activate_on_single_click(SongsTree, FALSE);
	gtk_tree_view_column_add_attribute(Column_artist, artists, "text", 0);
	gtk_tree_view_column_add_attribute(Column_song, songs, "text", 1);
	gtk_tree_view_column_add_attribute(Column_album, albums, "text", 2);
	gtk_tree_view_column_add_attribute(Column_source, sources, "text", 3);
	gtk_tree_view_column_add_attribute(Column_searchid, searchids, "text", 4);	
	gtk_tree_view_column_set_visible(Column_searchid,FALSE);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(SongsTree));

//	Signals:
	g_signal_connect(SearchWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_builder_connect_signals(builder, NULL);
	g_signal_connect(SongsTree, "row-activated", G_CALLBACK(on_row_double_clicked), selection);
	g_signal_connect(Save, "clicked", G_CALLBACK(on_Save_clicked), track);
	g_signal_connect(Exit, "clicked", G_CALLBACK(on_Exit_clicked), SearchWindow);
	g_signal_connect(Search, "clicked", G_CALLBACK(on_Search_clicked), NULL);

	gtk_widget_show(GTK_WIDGET(SearchWindow));

	gtk_main();
	
	return EXIT_SUCCESS;
}


//---------------------------------------------------------------------
//******************* \SearchWindow ***********************************
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//******************* EditWindow **************************************
//---------------------------------------------------------------------

void splitstr(string text, string delimiter = " ") {
    int start = 0;
    int end = text.find(delimiter);
    while (end != -1) {
        start = end + delimiter.size();
        end = text.find(delimiter, start);
    }
}

string apply_delay(string lyrics, int delay){

	string lyrics_delayed;
	string line, time_line, text_line;
	string delimiter = "\n";
	int start = 0;
	int end = lyrics.find(delimiter);
	lyrics.append("\n");
    while (end != -1) {
		line = lyrics.substr(start, end - start);
		if (line.length() > 9 && (line.at(0) == '[') && (line.at(3) == ':') && (line.at(6) == '.')) {
			int seconds;
			int minutes;
			minutes = (line.at(1) - 48)*10 + (line.at(2) - 48);
			seconds = (line.at(4) - 48)*1000 + (line.at(5) - 48)*100 + (line.at(7) - 48)*10 + (line.at(8) - 48) + delay;
			time_line = timestamps( minutes*60000 + seconds*10);
			text_line = line.substr(10, line.length() - 10); 
			lyrics_delayed.append(time_line + text_line + "\n");
		}
		else{
			lyrics_delayed.append(line + "\n");
		}
        start = end + delimiter.size();
        end = lyrics.find(delimiter, start);
    }
	return lyrics_delayed;
}

void update_text_textview(string text) {
	GtkTextIter Start_iter, Finish_iter;
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(refBuffer), &Start_iter);
	gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER(refBuffer), &Finish_iter);
	gtk_text_buffer_delete(GTK_TEXT_BUFFER(refBuffer),&Start_iter, &Finish_iter);

	gtk_text_buffer_insert(GTK_TEXT_BUFFER(refBuffer), &Start_iter,  text.c_str(), -1);

}

string get_text_textview() {

	GtkTextIter Start_iter, Finish_iter;
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(refBuffer), &Start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(refBuffer), &Finish_iter);
	string text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(refBuffer),&Start_iter, &Finish_iter, false);
	return text;

}

int power(int base, int exponent) {
	int i = 0, result = 1;
    while (i < exponent) {
        result = result * base;
        i++;
    }
	return result;
}

string apply_offset(string lyrics) {

	string offset = "[offset:";
	string squarebracket = "]";
	string lyrics_delayed;
	string line;
	int delay = 0;
	int start = 0;
	int end = lyrics.find(offset);

	line = lyrics.substr(start, end - start);
	lyrics_delayed.append(line);

	if (lyrics.size() != line.size()) {
		start = end + offset.size();
		end = lyrics.find(squarebracket, start);
		line = lyrics.substr(start, end - start);
		for (unsigned i=0; i < line.length(); ++i){
			delay = delay + (line.at(i) - 48)*power(10,line.length() - 1 - i);
		}
		delay = delay/10;
		start = end + squarebracket.size();
		end = lyrics.find(offset, start);
		line = lyrics.substr(start, end - start);
		lyrics_delayed.append(line);
		lyrics_delayed = apply_delay(lyrics_delayed, delay);
	}
	return lyrics_delayed;
}


void	on_Backward_clicked (GtkButton *b) {
	float percent = deadbeef->playback_get_pos();
	if (percent > 0.5){
		deadbeef->playback_set_pos( percent - 0.5 );
	}
}

void	on_Pause_play_clicked (GtkButton *b) {
	deadbeef->sendmessage( DB_EV_TOGGLE_PAUSE, 0, 0, 0 );
}

void	on_Forward_clicked (GtkButton *b) {
	float percent = deadbeef->playback_get_pos();
	if (percent < 99.5){
		deadbeef->playback_set_pos( percent + 0.5 );
	}
}

void	on_Plus_time_clicked (GtkButton *b) {
	string text = get_text_textview();
	text = apply_delay(text, 10);
	update_text_textview(text);
}

void	on_Minus_time_clicked (GtkButton *b) {
	string text = get_text_textview();
	text = apply_delay(text, -10);
	update_text_textview(text);
}

void	on_Apply_offset_clicked (GtkButton *b) {
	string text = get_text_textview();
	text = apply_offset(text);
	update_text_textview(text);
}

gchar *actual_track_time() {
	gchar *text;
	float pos = deadbeef->streamer_get_playpos();
	int minutes = pos / 60;
	pos = pos - minutes*60;
	if (minutes < 10){
 		text = g_strdup_printf ("0%i:%05.2f", minutes,pos);
	}
	else{
		text = g_strdup_printf ("%i:%05.2f", minutes,pos);
	}
	return text;
}

void	on_Insert_timestamp_clicked (GtkButton *b) {
	GtkTextIter iter;
	string time = actual_track_time();
	time = "\n[" + time + "]";
	gtk_text_buffer_get_iter_at_mark(refBuffer, &iter, gtk_text_buffer_get_insert(refBuffer));
	gtk_text_iter_forward_to_line_end (&iter);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(refBuffer), &iter, time.c_str(), -1);
}

static gboolean on_timeout (gpointer user_data) {
	GtkLabel *label = GTK_LABEL(user_data);
	gchar *text = actual_track_time();
	string playback_time = text;
	playback_time = "Playback: " + playback_time + " ";
	gtk_label_set_label (label, playback_time.c_str());
	g_free (text);

	return G_SOURCE_CONTINUE; /* or G_SOURCE_REMOVE when you want to stop */
}



void	on_Edit_cancel_clicked (GtkButton *b, gpointer user_data) {
	gtk_window_close(GTK_WINDOW(user_data));
}

void	on_Edit_apply_clicked (GtkButton *b) {
	bool sync = false;
	string text = get_text_textview();
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Check_sync))){
		sync = true;
	}
	DB_playItem_t *track = deadbeef->streamer_get_playing_track();
	save_meta_data(track, {text, sync});
	deadbeef->pl_item_unref(track);
}

void	on_Edit_OK_clicked (GtkButton *b, gpointer user_data) {
	
	on_Edit_apply_clicked (b);
	gtk_window_close(GTK_WINDOW(user_data));
}

extern "C"
int on_button_edit (GtkMenuItem *menuitem, gpointer user_data) {

	builder = gtk_builder_new_from_file (Glade_file_path.c_str());

	EditWindow = GTK_WINDOW(gtk_builder_get_object(builder, "EditWindow"));

//	Widgets creation:
	Text_view_lyrics_editor = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "Text_view_lyrics_editor"));
	Backward				= GTK_BUTTON(gtk_builder_get_object(builder, "Backward"));
	Pause_play				= GTK_BUTTON(gtk_builder_get_object(builder, "Pause_play"));
	Forward					= GTK_BUTTON(gtk_builder_get_object(builder, "Forward"));
	Playback_time			= GTK_LABEL(gtk_builder_get_object(builder, "Playback_time"));
	Plus_time				= GTK_BUTTON(gtk_builder_get_object(builder, "Plus_time"));
	Minus_time				= GTK_BUTTON(gtk_builder_get_object(builder, "Minus_time"));
	Apply_offset			= GTK_BUTTON(gtk_builder_get_object(builder, "Apply_offset"));
	Insert_timestamp		= GTK_BUTTON(gtk_builder_get_object(builder, "Insert_timestamp"));
	Check_sync				= GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "Check_sync"));
	Edit_apply				= GTK_BUTTON(gtk_builder_get_object(builder, "Edit_apply"));
	Edit_cancel				= GTK_BUTTON(gtk_builder_get_object(builder, "Edit_cancel"));
	Edit_OK					= GTK_BUTTON(gtk_builder_get_object(builder, "Edit_OK"));
	Sync_label				= GTK_LABEL(gtk_builder_get_object(builder, "Highlight_line_label"));
	Sync_or_not_label		= GTK_LABEL(gtk_builder_get_object(builder, "Highlight_line_label"));

//	Translation:
	if (strcmp(locale_lang , "es_ES.UTF-8") != 0) {
		gtk_label_set_label(Sync_label, "Syncronization ");
		gtk_label_set_label(Sync_or_not_label, "Sync or not ");
		gtk_button_set_label(GTK_BUTTON(Plus_time), "+ 0.1 seconds");
		gtk_button_set_label(GTK_BUTTON(Minus_time), "- 0.1 seconds");
		gtk_button_set_label(GTK_BUTTON(Apply_offset), "Apply offset");
		gtk_button_set_label(GTK_BUTTON(Insert_timestamp), "Insert timestamp");
		gtk_button_set_label(GTK_BUTTON(Check_sync), "Synced lyrics");
		gtk_window_set_title(EditWindow, "Edit");
	}

	DB_playItem_t *track = deadbeef->streamer_get_playing_track();
	struct parsed_lyrics track_lyrics;

	if (track){
		track_lyrics = get_lyrics_from_metadata(track);
		deadbeef->pl_item_unref(track);	
	}

	refBuffer = gtk_text_view_get_buffer(Text_view_lyrics_editor);
	update_text_textview(track_lyrics.lyrics);


//	Signals:
	g_signal_connect(EditWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_builder_connect_signals(builder, NULL);

	g_signal_connect(Backward, "clicked", G_CALLBACK(on_Backward_clicked), NULL);
	g_signal_connect(Pause_play, "clicked", G_CALLBACK(on_Pause_play_clicked), NULL);
	g_signal_connect(Forward, "clicked", G_CALLBACK(on_Forward_clicked), NULL);
	g_timeout_add (10 /* milliseconds */, on_timeout, Playback_time);
	g_signal_connect(Plus_time, "clicked", G_CALLBACK(on_Plus_time_clicked), NULL);
	g_signal_connect(Minus_time, "clicked", G_CALLBACK(on_Minus_time_clicked), NULL);
	g_signal_connect(Apply_offset, "clicked", G_CALLBACK(on_Apply_offset_clicked), NULL);
	g_signal_connect(Insert_timestamp, "clicked", G_CALLBACK(on_Insert_timestamp_clicked), NULL);
	g_signal_connect(Edit_apply, "clicked", G_CALLBACK(on_Edit_apply_clicked), NULL);
	g_signal_connect(Edit_cancel, "clicked", G_CALLBACK(on_Edit_cancel_clicked), EditWindow);
	g_signal_connect(Edit_OK, "clicked", G_CALLBACK(on_Edit_OK_clicked), EditWindow);

// Sync value to check button:
	if (track_lyrics.sync == true){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Check_sync), TRUE);
	}
	else{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Check_sync), FALSE);	
	}

	gtk_widget_show(GTK_WIDGET(EditWindow));

	gtk_main();
	
	return EXIT_SUCCESS;
}


//---------------------------------------------------------------------
//******************* \EditWindow *************************************
//---------------------------------------------------------------------
