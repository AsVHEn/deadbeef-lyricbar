#include "ui.h"
#include "debug.h"
#include "gettext.h"
#include <deadbeef/deadbeef.h>

#include <vector>
#include <regex>
#include <gtkmm.h>
#include <time.h>
#include <string>
#include <future>
#include <chrono>
#include <atomic>

using namespace std;
using namespace Gtk;
using namespace Glib;

struct linessizes{
	int titlesize;
	int artistsize;
	int newlinesize;
};

const DB_playItem_t *last;

struct timespec tss = {0, 100000000};

// TODO: eliminate all the global objects, as their initialization is not well defined

// Widget destruction flag - set to true when widgets are being destroyed
static atomic<bool> widgets_destroyed{false};

// Simplified stateless smooth scroll animation
static sigc::connection animation_connection;
static const int SCROLL_ANIMATION_INTERVAL = 16;     // ~60 FPS
static const double SCROLL_DAMPING_FACTOR = 0.15;    // Move 15% of distance per frame
static const double SCROLL_THRESHOLD = 0.5;          // Stop when within 0.5px

// NEW: Two-component layout
// Container
static VBox *mainVBox;

// Header (fixed, non-scrolling)
static TextView *headerView;
static RefPtr<TextBuffer> headerBuffer;

// Lyrics (scrollable)
static TextView *lyricsView;
static ScrolledWindow *lyricsScrolled;
static RefPtr<TextBuffer> lyricsBuffer;

// OLD: Keep for backward compatibility during transition
static TextView *lyricView;  // Will point to lyricsView
static ScrolledWindow *lyricbar;  // Will point to mainVBox
static RefPtr<TextBuffer> refBuffer;  // Will point to lyricsBuffer

static RefPtr<TextTag> tagItalic, tagBold, tagLarge, tagCenter, tagSmall, tagForegroundColorHighlight,tagForegroundColorRegular, tagLeftmargin, tagRightmargin, tagRegular;
static vector<RefPtr<TextTag>> tagsTitle, tagsArtist, tagsSyncline, tagsNosyncline, tagPadding;

bool isValidHexaCode(string str) {
    regex hexaCode("^#([a-fA-F0-9]{6}|[a-fA-F0-9]{3})$");
    return regex_match(str, hexaCode);
}

// Stateless smooth scroll animation tick - target is bound to the callback
static bool smooth_scroll_tick(double target) {
	// Check if widgets are valid
	if (widgets_destroyed.load() || !lyricsScrolled) {
		return false; // Stop animation
	}

	auto vadj = lyricsScrolled->get_vadjustment();
	if (!vadj) {
		return false;
	}

	// Get current position
	double current = vadj->get_value();
	double distance = target - current;

	// Check if we're close enough to stop
	if (abs(distance) < SCROLL_THRESHOLD) {
		vadj->set_value(target);
		return false; // Stop animation
	}

	// Apply damping: move a fraction of the distance
	double new_position = current + distance * SCROLL_DAMPING_FACTOR;

	// Clamp to valid range
	double page = vadj->get_page_size();
	double lower = vadj->get_lower();
	double upper = vadj->get_upper();
	double maxv = upper - page;
	if (maxv < lower) maxv = lower;
	if (new_position < lower) new_position = lower;
	if (new_position > maxv) new_position = maxv;

	vadj->set_value(new_position);
	return true; // Continue animation
}

// Start smooth scroll animation to target position
static void start_smooth_scroll(double target) {
	if (widgets_destroyed.load() || !lyricsScrolled) {
		return;
	}

	auto vadj = lyricsScrolled->get_vadjustment();
	if (!vadj) {
		return;
	}

	// Cancel any existing animation
	if (animation_connection.connected()) {
		animation_connection.disconnect();
	}

	// Start new animation with target bound to callback
	animation_connection = Glib::signal_timeout().connect(
		sigc::bind(sigc::ptr_fun(&smooth_scroll_tick), target),
		SCROLL_ANIMATION_INTERVAL
	);
}

// "Size" lyrics to be able to make lines "dissapear" on top.
// INTERNAL: This function MUST only be called from the main GTK thread!
static vector<int> sizelines_internal(DB_playItem_t * track, string lyrics) {
	// Check if widgets are being destroyed
	if (widgets_destroyed.load()) {
		return vector<int>();
	}

	// Validate widget pointers
	if (!lyricView || !lyricbar || !refBuffer) {
		return vector<int>();
	}

	if (!is_playing(track)) {
		return vector<int>();
	}

	string artist, title;
	deadbeef->pl_lock();
	artist = deadbeef->pl_find_meta(track, "artist") ?: _("Unknown Artist");
	title  = deadbeef->pl_find_meta(track, "title") ?: _("Unknown Title");
	deadbeef->pl_unlock();

	refBuffer->erase(refBuffer->begin(), refBuffer->end());
	refBuffer->insert_with_tags(refBuffer->begin(), title, tagsTitle);
	if (g_utf8_validate(lyrics.c_str(),-1,NULL)){
		refBuffer->insert_with_tags(refBuffer->end(), string{"\n"} + artist + "\n\n", tagsArtist);
		refBuffer->insert_with_tags(refBuffer->end(), lyrics, tagsNosyncline);
	}

//	std::cout << "Sizelines" << "\n";
	death_signal = 1;
	int sumatory = 0;
	int temporaly = 0;
	vector<int>  values;

//	I didn't found another way to be sure lyrics are displayed than wait millisenconds with nanosleep.
	nanosleep(&tss, NULL);

	// Re-check after sleep
	if (widgets_destroyed.load() || !lyricView || !lyricbar || !refBuffer) {
		death_signal = 0;
		return vector<int>();
	}

	deadbeef->pl_lock();
	values.push_back(lyricbar->get_allocation().get_height()*(deadbeef->conf_get_int("lyricbar.vpostion", 50))/100);
	deadbeef->pl_unlock();
	values.push_back(0);
	Gdk::Rectangle rectangle;
	for (int i = 2; i < refBuffer->get_line_count()-1; i++) {
		lyricView->get_iter_location(refBuffer->get_iter_at_line(i-2), rectangle);
		values.push_back(rectangle.get_y() - temporaly);
		temporaly = rectangle.get_y();
	}

	values[1] = values.size()-3;

	for (unsigned i = 2; i < values.size()-2; i++) {
		sumatory += values[i];
		if (sumatory > (values[0] - values[3] - values[4])) {
			values[1] = i-2;
			break;
		}
	}
//	std::cout << "Sizelines finished" << "\n";
	death_signal = 0;

	return values;
}

// Thread-safe wrapper for sizelines - can be called from any thread
vector<int> sizelines(DB_playItem_t * track, string lyrics) {
	// Check if widgets are destroyed before even trying
	if (widgets_destroyed.load()) {
		return vector<int>();
	}

	// Create promise/future for result
	auto promise_ptr = make_shared<promise<vector<int>>>();
	future<vector<int>> result_future = promise_ptr->get_future();

	// Marshal to main GTK thread
	signal_idle().connect_once([track, lyrics, promise_ptr]() {
		try {
			vector<int> result = sizelines_internal(track, lyrics);
			promise_ptr->set_value(result);
		} catch (...) {
			// If any exception occurs, return empty vector
			promise_ptr->set_value(vector<int>());
		}
	});

	// Wait for result with timeout (5 seconds)
	if (result_future.wait_for(chrono::seconds(5)) == future_status::timeout) {
		// Timeout - return empty vector
		return vector<int>();
	}

	return result_future.get();
}

// TWO-COMPONENT VERSION: Updates header and lyrics separately
void set_lyrics(DB_playItem_t *track, string past, string present, string future, string padding) {
	signal_idle().connect_once([track, past, present, future, padding ] {

		// Check if widgets are destroyed
		if (widgets_destroyed.load() || !headerBuffer || !lyricsBuffer) {
			return;
		}

		if (!is_playing(track)) {
			return;
		}
		string artist, title;
		deadbeef->pl_lock();
		artist = deadbeef->pl_find_meta(track, "artist") ?: _("Unknown Artist");
		title  = deadbeef->pl_find_meta(track, "title") ?: _("Unknown Title");
		deadbeef->pl_unlock();

		// Update HEADER buffer (fixed, non-scrolling)
		headerBuffer->erase(headerBuffer->begin(), headerBuffer->end());
		headerBuffer->insert_with_tags(headerBuffer->begin(), title, tagsTitle);
		headerBuffer->insert_with_tags(headerBuffer->end(), "\n" + artist, tagsArtist);

		// Update LYRICS buffer (scrollable)
		lyricsBuffer->erase(lyricsBuffer->begin(), lyricsBuffer->end());

		if (g_utf8_validate(future.c_str(),-1,NULL)){
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(), padding, tagPadding);
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(),past, tagsNosyncline);
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(),present, tagsSyncline);
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(),future, tagsNosyncline);
		}
		else{
			death_signal = 1;
			string error = "Wrong character encoding";
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(), padding, tagPadding);
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(),error, tagsSyncline);
		}




	});
}

// New simpler scroll implementation - centers current line
// TWO-COMPONENT VERSION: Updates header and lyrics separately
void set_lyrics_with_scroll(DB_playItem_t *track, const vector<string>& all_lyrics, int current_line_index) {
	signal_idle().connect_once([track, all_lyrics, current_line_index] {

		// Check if widgets are destroyed
		if (widgets_destroyed.load() || !headerBuffer || !lyricsBuffer || !lyricsView) {
			return;
		}

		if (!is_playing(track)) {
			return;
		}

		string artist, title;
		deadbeef->pl_lock();
		artist = deadbeef->pl_find_meta(track, "artist") ?: _("Unknown Artist");
		title  = deadbeef->pl_find_meta(track, "title") ?: _("Unknown Title");
		deadbeef->pl_unlock();

		// Update HEADER buffer (fixed, non-scrolling)
		headerBuffer->erase(headerBuffer->begin(), headerBuffer->end());
		headerBuffer->insert_with_tags(headerBuffer->begin(), title, tagsTitle);
		headerBuffer->insert_with_tags(headerBuffer->end(), "\n" + artist, tagsArtist);

		// Update LYRICS buffer (scrollable)
		lyricsBuffer->erase(lyricsBuffer->begin(), lyricsBuffer->end());

		// Calculate padding: add empty lines to fill half viewport before/after
		// This ensures first and last lines can be centered
		int padding_lines = 0;
		if (lyricsScrolled && lyricsView) {
			// Get viewport height
			int viewport_height = lyricsScrolled->get_allocation().get_height();

			// Estimate line height from a sample line
			if (all_lyrics.size() > 0) {
				// Insert a temporary line to measure height
				lyricsBuffer->insert_with_tags(lyricsBuffer->begin(), "X\n", tagsNosyncline);
				Gdk::Rectangle rect;
				lyricsView->get_iter_location(lyricsBuffer->get_iter_at_line(0), rect);
				int line_height = rect.get_height();

				// Clear the temp line
				lyricsBuffer->erase(lyricsBuffer->begin(), lyricsBuffer->end());

				// Calculate how many lines fit in half viewport
				if (line_height > 0) {
					padding_lines = (viewport_height / 2) / line_height + 1;
				}
			}
		}

		// Fallback: use reasonable default if calculation fails
		if (padding_lines <= 0) {
			padding_lines = 10;  // Default to ~10 lines of padding
		}

		// Insert padding before lyrics
		for (int i = 0; i < padding_lines; i++) {
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(), "\n", tagsNosyncline);
		}

		// Insert all lyrics with appropriate tags
		for (size_t i = 0; i < all_lyrics.size(); i++) {
			if ((int)i == current_line_index) {
				// Current line - highlighted
				lyricsBuffer->insert_with_tags(lyricsBuffer->end(), all_lyrics[i] + "\n", tagsSyncline);
			} else {
				// Other lines - normal
				lyricsBuffer->insert_with_tags(lyricsBuffer->end(), all_lyrics[i] + "\n", tagsNosyncline);
			}
		}

		// Insert padding after lyrics
		for (int i = 0; i < padding_lines; i++) {
			lyricsBuffer->insert_with_tags(lyricsBuffer->end(), "\n", tagsNosyncline);
		}

		// Scroll lyrics to center current line
		// Schedule via idle to ensure layout is complete (GTK3 best practice)
		if (current_line_index >= 0 && current_line_index < (int)all_lyrics.size()) {
			signal_idle().connect_once([current_line_index, padding_lines]() {
				// Check widgets still valid
				if (widgets_destroyed.load() || !lyricsView || !lyricsScrolled || !lyricsBuffer) {
					return;
				}

				// Get iterator at the current lyric line (accounting for padding offset)
				int buffer_line = padding_lines + current_line_index;
				auto iter = lyricsBuffer->get_iter_at_line(buffer_line);

				// Get line position in buffer coordinates
				Gdk::Rectangle rect;
				lyricsView->get_iter_location(iter, rect);

				// Convert buffer coords to widget coords
				int wx, wy;
				lyricsView->buffer_to_window_coords(TEXT_WINDOW_WIDGET, rect.get_x(), rect.get_y(), wx, wy);

				// Calculate line center in widget coordinates
				double line_center = (double)wy + rect.get_height() / 2.0;

				// Get vertical adjustment
				auto vadj = lyricsScrolled->get_vadjustment();
				double page = vadj->get_page_size();
				double lower = vadj->get_lower();
				double upper = vadj->get_upper();

				// Calculate target scroll position to center the line
				// target = line_center - page/2
				double target = line_center - page / 2.0;

				// Clamp to valid range [lower, upper - page]
				double maxv = upper - page;
				if (maxv < lower) maxv = lower;
				if (target < lower) target = lower;
				if (target > maxv) target = maxv;

				// Start smooth scroll animation to target
				start_smooth_scroll(target);
			});
		}
	});
}

// To have scroll bars or not when lyrics are synced or not.
// TWO-COMPONENT VERSION: Only affects lyrics ScrolledWindow
void sync_or_unsync(bool syncedlyrics) {
	if (syncedlyrics == true) {
		lyricsScrolled->set_policy(POLICY_EXTERNAL, POLICY_EXTERNAL);
	}
	else{
		lyricsScrolled->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	}
}

Justification get_justification() {

	int align = deadbeef->conf_get_int("lyricbar.lyrics.alignment", 1);
	switch (align) {
		case 0:
			return JUSTIFY_LEFT;
		case 2:
			return JUSTIFY_RIGHT;
		default:
			return JUSTIFY_CENTER;
	}
}

	void get_tags() {
	// Create tags for HEADER buffer
	tagItalic = headerBuffer->create_tag();
	tagItalic->property_style() = Pango::STYLE_ITALIC;
	tagItalic->property_scale() = deadbeef->conf_get_float("lyricbar.fontscale", 1);

	tagBold = headerBuffer->create_tag();
	tagBold->property_scale() = deadbeef->conf_get_float("lyricbar.fontscale", 1);
	tagBold->property_weight() = Pango::WEIGHT_BOLD;

	tagLarge = headerBuffer->create_tag();
	tagLarge->property_scale() = 1.2*deadbeef->conf_get_float("lyricbar.fontscale", 1);

	tagCenter = headerBuffer->create_tag();
	tagCenter->property_justification() = JUSTIFY_CENTER;

	tagsTitle = {tagLarge, tagBold, tagCenter};
	tagsArtist = {tagItalic, tagCenter};

	// Create tags for LYRICS buffer
	tagRegular = lyricsBuffer->create_tag();
	tagRegular->property_scale() = deadbeef->conf_get_float("lyricbar.fontscale", 1);

	tagLeftmargin = lyricsBuffer->create_tag();
	tagLeftmargin->property_left_margin() = deadbeef->conf_get_float("lyricbar.border", 22)*deadbeef->conf_get_float("lyricbar.fontscale", 1);

	tagRightmargin = lyricsBuffer->create_tag();
	tagRightmargin->property_right_margin() = deadbeef->conf_get_float("lyricbar.border", 22)*deadbeef->conf_get_float("lyricbar.fontscale", 1);

	tagSmall = lyricsBuffer->create_tag();
	tagSmall->property_scale() = 0.0001;

	tagForegroundColorHighlight = lyricsBuffer->create_tag();
	tagForegroundColorRegular = lyricsBuffer->create_tag();

	if (isValidHexaCode(deadbeef->conf_get_str_fast("lyricbar.highlightcolor", "#571c1c"))){
		tagForegroundColorHighlight->property_foreground() = deadbeef->conf_get_str_fast("lyricbar.highlightcolor", "#571c1c");
	}
	else{
		tagForegroundColorHighlight->property_foreground() = "#571c1c";
	}

	if (isValidHexaCode(deadbeef->conf_get_str_fast("lyricbar.regularcolor", "#000000"))){
		tagForegroundColorRegular->property_foreground() = deadbeef->conf_get_str_fast("lyricbar.regularcolor", "#000000");
	}
	else{
		tagForegroundColorRegular->property_foreground() = "#000000";
	}

	tagsSyncline = {tagBold, tagForegroundColorHighlight};

	if (deadbeef->conf_get_int("lyricbar.bold", 1) == 1) {
		tagsSyncline = {tagBold, tagForegroundColorHighlight};
		tagsNosyncline = {tagRegular, tagLeftmargin, tagRightmargin, tagForegroundColorRegular};
	}
	else{
		tagsSyncline = {tagRegular, tagForegroundColorHighlight};
		tagsNosyncline = {tagRegular, tagForegroundColorRegular};
    	}

    if (get_justification() == JUSTIFY_LEFT) {
		tagRightmargin->property_right_margin() = deadbeef->conf_get_float("lyricbar.border", 22)*deadbeef->conf_get_float("lyricbar.fontscale", 1);
		tagsNosyncline = {tagRegular, tagRightmargin};
    }
	if (get_justification() == JUSTIFY_RIGHT) {
    	tagLeftmargin->property_left_margin() = deadbeef->conf_get_float("lyricbar.border", 22)*deadbeef->conf_get_float("lyricbar.fontscale", 1);
		tagsNosyncline = {tagRegular, tagLeftmargin};
    }

	tagPadding = {tagSmall};
}

extern "C"

GtkWidget *construct_lyricbar() {
	Gtk::Main::init_gtkmm_internals();

	// Create TWO separate buffers
	headerBuffer = TextBuffer::create();
	lyricsBuffer = TextBuffer::create();
	get_tags();

	// Create HEADER view (fixed, non-scrolling)
	headerView = new TextView(headerBuffer);
	headerView->set_editable(false);
	headerView->set_can_focus(false);
	headerView->set_name("headerView");
	headerView->set_justification(JUSTIFY_CENTER);
	headerView->set_wrap_mode(WRAP_WORD_CHAR);
	// Set fixed height for approximately 2 lines
	headerView->set_size_request(-1, 60);  // Fixed height
	headerView->show();

	// Create LYRICS view (scrollable)
	lyricsView = new TextView(lyricsBuffer);
	lyricsView->set_editable(false);
	lyricsView->set_can_focus(false);
	lyricsView->set_name("lyricsView");
	lyricsView->set_left_margin(2);
	lyricsView->set_right_margin(2);
	lyricsView->set_justification(get_justification());
	lyricsView->set_wrap_mode(WRAP_WORD_CHAR);
    if (get_justification() == JUSTIFY_LEFT) {
    	lyricsView->set_left_margin(20);
    }
	lyricsView->show();

	// Wrap lyrics in ScrolledWindow
	lyricsScrolled = new ScrolledWindow();
	lyricsScrolled->add(*lyricsView);
	lyricsScrolled->set_name("lyricsScrolled");
	lyricsScrolled->set_policy(POLICY_EXTERNAL, POLICY_EXTERNAL);
	lyricsScrolled->show();

	// Create VBox container
	mainVBox = new VBox(false, 0);  // not homogeneous, no spacing
	mainVBox->pack_start(*headerView, PACK_SHRINK, 0);  // Fixed size
	mainVBox->pack_start(*lyricsScrolled, PACK_EXPAND_WIDGET, 0);  // Expand to fill
	mainVBox->show();

	// Set backward compatibility pointers
	lyricView = lyricsView;
	refBuffer = lyricsBuffer;

	/**********/

	//Create css config text for both views
	std::string cssconfig("\
  		#headerView text {\
  		background-color: ");
	if (isValidHexaCode(deadbeef->conf_get_str_fast("lyricbar.backgroundcolor", "#F6F6F6"))){
		cssconfig.append(deadbeef->conf_get_str_fast("lyricbar.backgroundcolor", "#F6F6F6"));
	}
	else{
		cssconfig.append("#F6F6F6");
	}
	cssconfig.append(";\
		}\
		#lyricsView text {\
  		background-color: ");
	if (isValidHexaCode(deadbeef->conf_get_str_fast("lyricbar.backgroundcolor", "#F6F6F6"))){
		cssconfig.append(deadbeef->conf_get_str_fast("lyricbar.backgroundcolor", "#F6F6F6"));
	}
	else{
		cssconfig.append("#F6F6F6");
	}
	cssconfig.append(";\
		}\
	");

	//load css
	auto data = g_strdup_printf("%s", cssconfig.c_str());

	RefPtr<Gtk::CssProvider> cssProvider = Gtk::CssProvider::create();
	cssProvider->load_from_data(data);

	RefPtr<Gtk::StyleContext> styleContext = Gtk::StyleContext::create();

	//get default screen
	RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();

	//add provider for screen in all application
	styleContext->add_provider_for_screen(screen, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


	return GTK_WIDGET(mainVBox->gobj());
}




extern "C"
int message_handler(struct ddb_gtkui_widget_s*, uint32_t id, uintptr_t ctx, uint32_t, uint32_t) {
	auto event = reinterpret_cast<ddb_event_track_t *>(ctx);
	switch (id) {
		case DB_EV_CONFIGCHANGED:
			debug_out << "CONFIG CHANGED\n";
			get_tags();
			signal_idle().connect_once([]{ lyricView->set_justification(get_justification()); });
			break;
		case DB_EV_TERMINATE:
			debug_out << "DEADBEEF CLOSED \n";
			death_signal = 1;
			break;
//		case DB_EV_TRACKINFOCHANGED:
//			debug_out << "TRACKINFOCHANGED" << "\n";
//			break;
		case DB_EV_PLUGINSLOADED:
		case DB_EV_SONGSTARTED:
			debug_out << "SONG STARTED\n";
			if (!event->track || event->track == last || deadbeef->pl_get_item_duration(event->track) <= 0.0){
//				std::cout << "if in" << "\n";
				return 0;
			}
			last = event->track;
//			std::cout << "SONG STARTED" << "\n";
			auto tid = deadbeef->thread_start(update_lyrics, event->track);
			deadbeef->thread_detach(tid);
			break;
	}

	return 0;
}

extern "C"
void lyricbar_destroy() {
	// Set destruction flag FIRST to prevent any new GTK operations
	widgets_destroyed.store(true);

	// Stop smooth scroll animation
	if (animation_connection.connected()) {
		animation_connection.disconnect();
	}

	// Give pending operations a moment to check the flag
	struct timespec brief_wait = {0, 10000000}; // 10ms
	nanosleep(&brief_wait, NULL);

	// Delete new widgets (VBox will auto-delete children)
	delete mainVBox;  // This deletes headerView and lyricsScrolled
	// Note: lyricsView is deleted by lyricsScrolled
	// Note: headerView is deleted by mainVBox

	// Clear tag vectors
	tagsArtist.clear();
	tagPadding.clear();
	tagsSyncline.clear();
	tagsNosyncline.clear();
	tagRegular.clear();
	tagsTitle.clear();

	// Reset tag RefPtrs
	tagLarge.reset();
	tagSmall.reset();
	tagBold.reset();
	tagItalic.reset();
	tagRightmargin.reset();
	tagLeftmargin.reset();
	tagCenter.reset();
	tagForegroundColorHighlight.reset();
	tagForegroundColorRegular.reset();

	// Reset buffer RefPtrs
	headerBuffer.reset();
	lyricsBuffer.reset();

	// Reset backward compatibility pointers (don't delete, already deleted)
	lyricView = nullptr;
	lyricbar = nullptr;
	refBuffer.reset();

	// Reset flag for potential re-initialization
	widgets_destroyed.store(false);
}
