#include "main.h"

#include <string.h>
#include <stdlib.h>

#include "ui.h"
#include "utils.h"
#include "gettext.h"

static ddb_gtkui_t *gtkui_plugin;
DB_functions_t *deadbeef;

static DB_misc_t plugin;


static int lyricbar_disconnect() {
	if (gtkui_plugin) {
		gtkui_plugin->w_unreg_widget(plugin.plugin.id);
	}
	death_signal = 1;
	return 0;
}

static int lyricbar_stop() {
	death_signal = 1;
    gtkui_plugin = NULL;
	return 0;
}

DB_plugin_action_t remove_action = {
	.name = "remove_lyrics",
	.flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
	.callback2 = remove_from_cache_action,
	.next = NULL,
	.title = "Remove Lyrics From Cache"
};

static DB_plugin_action_t *
lyricbar_get_actions() {
	deadbeef->pl_lock();
	remove_action.flags |= DB_ACTION_DISABLED;
	DB_playItem_t *current = deadbeef->pl_get_first(PL_MAIN);
	while (current) {
		if (deadbeef->pl_is_selected(current) && is_cached(
		            deadbeef->pl_find_meta(current, "artist"),
		            deadbeef->pl_find_meta(current, "title"))) {
			remove_action.flags &= (uint32_t)~DB_ACTION_DISABLED;
			deadbeef->pl_item_unref(current);
			break;
		}
		DB_playItem_t *next = deadbeef->pl_get_next(current, PL_MAIN);
		deadbeef->pl_item_unref(current);
		current = next;
	}
	deadbeef->pl_unlock();
	return &remove_action;
}

static ddb_gtkui_widget_t*
w_lyricbar_create(void) {
	ddb_gtkui_widget_t *widget = malloc(sizeof(ddb_gtkui_widget_t));
	memset(widget, 0, sizeof(ddb_gtkui_widget_t));

	widget->widget  = construct_lyricbar();
	widget->destroy = lyricbar_destroy;
	widget->message = message_handler;

	gtkui_plugin->w_override_signals(widget->widget, widget);
	return widget;
}

static int lyricbar_connect() {
	gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
	if (!gtkui_plugin) {
		fprintf(stderr, "%s: can't find gtkui plugin\n", plugin.plugin.id);
		return -1;
	}
	gtkui_plugin->w_reg_widget("Lyricbar", 0, w_lyricbar_create, "lyricbar", NULL);
	return 0;
}

__attribute__ ((visibility ("default")))
#if GTK_MAJOR_VERSION == 2
DB_plugin_t *ddb_lyricbar_gtk2_load(DB_functions_t *ddb) {
#else
DB_plugin_t *ddb_lyricbar_gtk3_load(DB_functions_t *ddb) {
#endif
	deadbeef = ddb;
	bindtextdomain("deadbeef-lyricbar", "/usr/share/locale");
	textdomain("deadbeef-lyricbar");
	remove_action.title = _(remove_action.title);
	ensure_lyrics_path_exists();
	return DB_PLUGIN(&plugin);
}


static DB_misc_t plugin = {
	.plugin.api_vmajor = 1,
	.plugin.api_vminor = 5,
	.plugin.version_major = 0,
	.plugin.version_minor = 1,
	.plugin.type = DB_PLUGIN_MISC,
	.plugin.name = "Lyricbar",
#if GTK_MAJOR_VERSION == 2
	.plugin.id = "lyricbar-gtk2",
#else
	.plugin.id = "lyricbar-gtk3",
#endif
	.plugin.descr = "Lyricbar plugin for DeadBeeF audio player.\nPlugin for DeaDBeeF audio player that fetches and shows the song’s with scroll on sync lyrics. \n",
	.plugin.copyright = "Copyleft (C) 2015 AsVHEn\n",
	.plugin.website = "https://github.com/asvhen/deadbeef-lyricbar",
	.plugin.connect = lyricbar_connect,
    .plugin.stop = lyricbar_stop,
	.plugin.disconnect = lyricbar_disconnect,
	.plugin.get_actions = lyricbar_get_actions,
	.plugin.configdialog =	"property \"Lyrics alignment type \" select[3] lyricbar.lyrics.alignment 1 left center right; \n"
							"property \"Custom lyrics fetching command \" entry lyricbar.customcmd \"\"; \n"
							"property \"Font scale \" hscale[0,10,0.01] lyricbar.fontscale 1; \n"
							"property \"Highlight lyrics color (#XXXXXX HEX type. Restart needed) \" entry lyricbar.highlightcolor \"\"; \n"
							"property \"Background color (#XXXXXX HEX type. Restart needed) \" entry lyricbar.backgroundcolor \"\"; \n"
							"property \"Highlight lyrics position \" select[5] lyricbar.vpostion 1 top up center down bottom; \n"
							"property \"Highlight lyrics Bold \" checkbox lyricbar.bold 1; \n"
};

