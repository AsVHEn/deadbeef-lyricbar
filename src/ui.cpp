#include "ui.h"

#include <memory>
#include <vector>

#include <glibmm/main.h>
#include <gtkmm/main.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textview.h>
#include <gtkmm/widget.h>

#include <gtkmm.h>
#include <time.h>

#include "debug.h"
#include "gettext.h"
#include <deadbeef/deadbeef.h>

using namespace std;
using namespace Gtk;
using namespace Glib;

struct linessizes{
	int titlesize;
	int artistsize;
	int newlinesize;
};

struct timespec tss = {0, 100000000};

int width;

// TODO: eliminate all the global objects, as their initialization is not well defined
static TextView *lyricView;
static ScrolledWindow *lyricbar;
static RefPtr<TextBuffer> refBuffer;
static RefPtr<TextTag> tagItalic, tagBold, tagLarge, tagCenter, tagSmall, tagForegroundColor, tagLeftmargin, tagRightmargin;
static vector<RefPtr<TextTag>> tagsTitle, tagsArtist, tagsSyncline, tagsNosyncline, tagPadding;
vector<RefPtr<TextTag>> tags;

vector<int> sizelines(DB_playItem_t * track, Glib::ustring lyrics){
	std::cout << "Sizelines" << "\n";
	set_lyrics(track, lyrics,"","","");
	nanosleep(&tss, NULL);
	int sumatory = 0;
	int temporaly = 0;
	vector<int>  values;
	values.push_back(lyricbar->get_allocation().get_height()/3);
	values.push_back(0);
	Gdk::Rectangle rectangle;
	for (int i = 2; i < refBuffer->get_line_count()-1; i++){
		lyricView->get_iter_location(refBuffer->get_iter_at_line(i-2), rectangle);
		values.push_back(rectangle.get_y() - temporaly);
		temporaly = rectangle.get_y();
	}
	values[1] = values.size()-3;
	for (unsigned i = 2; i < values.size()-2; i++){
		sumatory += values[i];
		if (sumatory > (values[0] - values[3] - values[4])){
			values[1] = i-2;
			break;
		}
	}
return values;
	

}

void set_lyrics(DB_playItem_t *track, ustring past, ustring present, ustring future, ustring padding) {
	signal_idle().connect_once([track, past, present, future, padding ] {
		ustring artist, title;
		{
			pl_lock_guard guard;

			if (!is_playing(track))
				return;
			artist = deadbeef->pl_find_meta(track, "artist") ?: _("Unknown Artist");
			title  = deadbeef->pl_find_meta(track, "title") ?: _("Unknown Title");
		}
	
		refBuffer->erase(refBuffer->begin(), refBuffer->end());
		refBuffer->insert_with_tags(refBuffer->begin(), title, tagsTitle);
		refBuffer->insert_with_tags(refBuffer->end(), ustring{"\n"} + artist + "\n\n", tagsArtist);
	
		vector<RefPtr<TextTag>> tags;
		refBuffer->insert_with_tags(refBuffer->end(), padding, tagPadding);
		refBuffer->insert_with_tags(refBuffer->end(),past, tagsNosyncline);
		refBuffer->insert_with_tags(refBuffer->end(),present, tagsSyncline);
		refBuffer->insert_with_tags(refBuffer->end(),future, tagsNosyncline);

		last = track;
	});
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


extern "C"

GtkWidget *construct_lyricbar() {

	Gtk::Main::init_gtkmm_internals();
	refBuffer = TextBuffer::create();

	tagItalic = refBuffer->create_tag();
	tagItalic->property_style() = Pango::STYLE_ITALIC;

	tagLeftmargin = refBuffer->create_tag();
	tagLeftmargin->property_left_margin() = 22;

	tagRightmargin = refBuffer->create_tag();
	tagRightmargin->property_right_margin() = 22;

	tagBold = refBuffer->create_tag();
	tagBold->property_weight() = Pango::WEIGHT_BOLD;

	tagLarge = refBuffer->create_tag();
	tagLarge->property_scale() = Pango::SCALE_LARGE;

	tagSmall = refBuffer->create_tag();
	tagSmall->property_scale() = 0.0001;

	tagCenter = refBuffer->create_tag();
	tagCenter->property_justification() = JUSTIFY_CENTER;

	tagForegroundColor = refBuffer->create_tag();
	tagForegroundColor->property_foreground() = deadbeef->conf_get_str_fast("lyricbar.highlightcolor", "#571c1c");

	tagsTitle = {tagLarge, tagBold, tagCenter};
	tagsArtist = {tagItalic, tagCenter};
	tagsSyncline = {tagBold, tagCenter, tagForegroundColor};
	tagsNosyncline = {tagLeftmargin, tagRightmargin};
	tagPadding = {tagSmall, tagCenter};


	lyricView = new TextView(refBuffer);
	
	lyricView->set_editable(false);
	lyricView->set_can_focus(false);
	lyricView->set_name("lyricView");
	lyricView->set_left_margin(2);
	lyricView->set_right_margin(2);
	lyricView->set_justification(get_justification());
    lyricView->get_vadjustment()->set_value(1000);
	lyricView->set_wrap_mode(WRAP_WORD_CHAR);
        if (get_justification() == JUSTIFY_LEFT) {
           lyricView->set_left_margin(20);
        }
	lyricView->show();
	lyricbar = new ScrolledWindow();
	lyricbar->add(*lyricView);
	lyricbar->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);

	/**********/

	
	//load css
	auto data = g_strdup_printf("\
  		#lyricView text {\
  		background-color: #F6F6F6;\
		}\
	");


	Glib::RefPtr<Gtk::CssProvider> cssProvider = Gtk::CssProvider::create();
	cssProvider->load_from_data(data);
	
	Glib::RefPtr<Gtk::StyleContext> styleContext = Gtk::StyleContext::create();
	
	//get default screen
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	
	//add provider for screen in all application
	styleContext->add_provider_for_screen(screen, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


	return GTK_WIDGET(lyricbar->gobj());
}




extern "C"
int message_handler(struct ddb_gtkui_widget_s*, uint32_t id, uintptr_t ctx, uint32_t, uint32_t) {
	auto event = reinterpret_cast<ddb_event_track_t *>(ctx);

	switch (id) {
		case DB_EV_CONFIGCHANGED:
			debug_out << "CONFIG CHANGED\n";
			signal_idle().connect_once([]{ lyricView->set_justification(get_justification()); });
			break;
		case DB_EV_SONGSTARTED:
			debug_out << "SONG STARTED\n";
//		case DB_EV_TRACKINFOCHANGED:
			if (!event->track || event->track == last || deadbeef->pl_get_item_duration(event->track) <= 0)
				return 0;
			//std::cout << "TRACKINFOCHANGED" << "\n";
			auto tid = deadbeef->thread_start(update_lyrics, event->track);
			deadbeef->thread_detach(tid);
			break;
	}

	return 0;
}

extern "C"
void lyricbar_destroy() {
	delete lyricbar;
	delete lyricView;
	tagsArtist.clear();
	tagsSyncline.clear();
	tagsNosyncline.clear();
	tagsTitle.clear();
	tagLarge.reset();
	tagSmall.reset();
	tagBold.reset();
	tagItalic.reset();
	tagRightmargin.reset();
	tagLeftmargin.reset();
	refBuffer.reset();
}

