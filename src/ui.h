#pragma once
#ifndef LYRICBAR_UI_H
#define LYRICBAR_UI_H
#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <vector>

#include "utils.h"

#ifdef __cplusplus

#include <glibmm/ustring.h>

using namespace std;

void set_lyrics(DB_playItem_t * track, Glib::ustring past, Glib::ustring present, Glib::ustring future, Glib::ustring padding);


vector<int> sizelines(DB_playItem_t * track, Glib::ustring lyrics);

extern int width;

extern "C" {
#endif

GtkWidget *construct_lyricbar();

int message_handler(struct ddb_gtkui_widget_s *, uint32_t id, uintptr_t ctx, uint32_t, uint32_t);

void lyricbar_destroy();

#ifdef __cplusplus
}
#endif

#endif // LYRICBAR_UI_H
