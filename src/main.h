#pragma once
#ifndef LYRICBAR_MAIN_H
#define LYRICBAR_MAIN_H

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>

extern int death_signal;
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ddb_gtkui_widget_t base;
} widget_lyricbar_t;

extern DB_functions_t * deadbeef;

#ifdef __cplusplus
}
#endif

#endif // LYRICBAR_MAIN_H
