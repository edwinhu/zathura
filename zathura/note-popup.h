/* SPDX-License-Identifier: Zlib */

#ifndef NOTE_POPUP_H
#define NOTE_POPUP_H

#include <gtk/gtk.h>
#include "types.h"

typedef struct zathura_s zathura_t;

/**
 * Callback signature for when note is saved
 *
 * @param zathura The zathura instance
 * @param page Page number (0-indexed)
 * @param x X coordinate (PDF units)
 * @param y Y coordinate (PDF units)
 * @param content Note text content
 * @param note_id Existing note ID if editing, NULL if new note
 * @param data User data passed to callback
 */
typedef void (*ZathuraNotePopupSaveCallback)(zathura_t* zathura, unsigned int page,
                                              double x, double y, const char* content,
                                              const char* note_id, void* data);

/**
 * Create a new note popup widget
 *
 * @param zathura The zathura instance
 * @return GtkPopover widget, or NULL on error
 */
GtkWidget* zathura_note_popup_new(zathura_t* zathura);

/**
 * Show popup for new note or editing existing note
 *
 * @param popup The popup widget created by zathura_note_popup_new
 * @param relative_to Widget to position popup relative to
 * @param widget_x X coordinate in widget space (for popup positioning)
 * @param widget_y Y coordinate in widget space (for popup positioning)
 * @param page Page number (0-indexed)
 * @param x X coordinate (PDF units)
 * @param y Y coordinate (PDF units)
 * @param note Existing note to edit, or NULL for new note
 * @param callback Callback to invoke when note is saved
 * @param data User data to pass to callback
 */
void zathura_note_popup_show(GtkWidget* popup, GtkWidget* relative_to,
                              double widget_x, double widget_y,
                              unsigned int page, double x, double y,
                              zathura_note_t* note,
                              ZathuraNotePopupSaveCallback callback, void* data);

/**
 * Hide the popup without saving
 *
 * @param popup The popup widget
 */
void zathura_note_popup_hide(GtkWidget* popup);

#endif /* NOTE_POPUP_H */
