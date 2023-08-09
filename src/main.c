#include "main.h"

#include <string.h>
#include <stdlib.h>

#include "ui.h"
#include "lrcspotify.h"
#include "utils.h"
#include "gettext.h"
#include "config_dialog.h"

#include <gtk/gtk.h>
#include <deadbeef/gtkui_api.h>
#include <stdint.h>


static ddb_gtkui_t *gtkui_plugin;
DB_functions_t *deadbeef;

static gboolean _pop (GtkTextView *text_view, GtkWidget *popup, gpointer user_data) {
	GtkWidget *popup_config;
	GtkWidget *popup_search;
	GtkWidget *popup_edit;

	if (strcmp(setlocale(LC_CTYPE, NULL) , "es_ES.UTF-8") == 0) {
		popup_config = gtk_menu_item_new_with_label("Configurar");
		popup_search = gtk_menu_item_new_with_label("Buscar");
		popup_edit = gtk_menu_item_new_with_label("Editar");
	}
	else {
		popup_config = gtk_menu_item_new_with_label("Config");
		popup_search = gtk_menu_item_new_with_label("Search");
		popup_edit = gtk_menu_item_new_with_label("Edit");	
	}

	GList *children = gtk_container_get_children(GTK_CONTAINER(popup));
	gtk_container_remove(GTK_CONTAINER(popup),children->data);
	while ((children = g_list_next(children)) != NULL) {
//		printf("%s \n",gtk_menu_item_get_label(children->data));
		gtk_container_remove(GTK_CONTAINER(popup),children->data);
	}
	gtk_menu_attach(GTK_MENU(popup),popup_config,0,1,0,1);
	gtk_menu_attach(GTK_MENU(popup),popup_search,0,1,1,2);
	gtk_menu_attach(GTK_MENU(popup),popup_edit,0,1,2,3);

	gtk_widget_show(popup_config);
	gtk_widget_show(popup_search);
	gtk_widget_show(popup_edit);
	DB_playItem_t *track = deadbeef->streamer_get_playing_track();
	if (track) {
		deadbeef->pl_item_unref(track);
	}
	else {
		gtk_widget_set_sensitive (popup_edit,FALSE);
		gtk_widget_set_sensitive (popup_search,FALSE);
	}
	
	g_signal_connect_after(popup_config, "activate", G_CALLBACK(on_button_config), user_data);
	g_signal_connect_after(popup_search, "activate", G_CALLBACK(on_button_search), user_data);
	g_signal_connect_after(popup_edit, "activate", G_CALLBACK(on_button_edit), user_data);	
    return TRUE;
}

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

static DB_plugin_action_t *lyricbar_get_actions() {
	deadbeef->pl_lock();
	remove_action.flags |= DB_ACTION_DISABLED;
	DB_playItem_t *current = deadbeef->pl_get_first(PL_MAIN);
	while (current) {
		deadbeef->pl_lock();
		if (deadbeef->pl_is_selected(current) && is_cached(
		            deadbeef->pl_find_meta(current, "artist"),
		            deadbeef->pl_find_meta(current, "title"))) {
			remove_action.flags &= (uint32_t)~DB_ACTION_DISABLED;
			deadbeef->pl_item_unref(current);
			break;
		}
		deadbeef->pl_unlock();
		DB_playItem_t *next = deadbeef->pl_get_next(current, PL_MAIN);
		deadbeef->pl_item_unref(current);
		current = next;
	}
	deadbeef->pl_unlock();
	return &remove_action;
}

static ddb_gtkui_widget_t *w_lyricbar_create(void) {
	widget_lyricbar_t *widget = malloc(sizeof(widget_lyricbar_t));
	memset(widget, 0, sizeof(widget_lyricbar_t));

	widget->base.widget  = construct_lyricbar();
	widget->base.destroy = lyricbar_destroy;
	widget->base.message = message_handler;
	GList *children = gtk_container_get_children(GTK_CONTAINER(widget->base.widget));
	gtkui_plugin->w_override_signals(widget->base.widget, widget);
	g_signal_connect(children->data, "populate_popup", G_CALLBACK (_pop), children->data);

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
	.plugin.copyright = "Copyleft (C) 2023 AsVHEn\n",
	.plugin.website = "https://github.com/asvhen/deadbeef-lyricbar",
	.plugin.connect = lyricbar_connect,
    .plugin.stop = lyricbar_stop,
	.plugin.disconnect = lyricbar_disconnect,
	.plugin.get_actions = lyricbar_get_actions,
	.plugin.configdialog =	"property \"Font scale: \" hscale[0,10,0.01] lyricbar.fontscale 1; \n"
							"property \"End url search (AZlyrics) (&x=...): \" entry lyricbar.end_url_search \"\"; \n"
							"property \"SP-DC cookie (Spotify): \" entry lyricbar.sp_dc_cookie \"\"; \n"
};

