/* SPDX-License-Identifier: Zlib */

#include <string.h>
#include <glib/gi18n.h>

#include "note-popup.h"
#include "zathura.h"

/* Keys for storing data on the popup widget */
#define NOTE_POPUP_KEY_ZATHURA       "zathura-note-popup-zathura"
#define NOTE_POPUP_KEY_TEXT_VIEW     "zathura-note-popup-text-view"
#define NOTE_POPUP_KEY_PAGE          "zathura-note-popup-page"
#define NOTE_POPUP_KEY_X             "zathura-note-popup-x"
#define NOTE_POPUP_KEY_Y             "zathura-note-popup-y"
#define NOTE_POPUP_KEY_NOTE_ID       "zathura-note-popup-note-id"
#define NOTE_POPUP_KEY_CALLBACK_DATA "zathura-note-popup-callback-data"
#define NOTE_POPUP_KEY_CANCELLED     "zathura-note-popup-cancelled"

/* Struct to hold callback and user data together (avoids function pointer cast warnings) */
typedef struct {
  ZathuraNotePopupSaveCallback callback;
  void* user_data;
} NotePopupCallbackData;

/* Minimum popup dimensions */
#define NOTE_POPUP_MIN_WIDTH  200
#define NOTE_POPUP_MIN_HEIGHT 150

/* Forward declarations */
static void cb_popup_closed(GtkPopover* popover, gpointer user_data);
static gboolean cb_text_view_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data);

GtkWidget*
zathura_note_popup_new(zathura_t* zathura) {
  g_return_val_if_fail(zathura != NULL, NULL);

  /* Create the popover */
  GtkWidget* popover = gtk_popover_new(NULL);
  gtk_popover_set_modal(GTK_POPOVER(popover), TRUE);

  /* Create scrolled window for text view */
  GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scrolled, NOTE_POPUP_MIN_WIDTH, NOTE_POPUP_MIN_HEIGHT);

  /* Create text view for multi-line editing */
  GtkWidget* text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 4);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 4);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_view), 4);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_view), 4);

  /* Add text view to scrolled window */
  gtk_container_add(GTK_CONTAINER(scrolled), text_view);

  /* Add scrolled window to popover */
  gtk_container_add(GTK_CONTAINER(popover), scrolled);

  /* Store references on the popover */
  g_object_set_data(G_OBJECT(popover), NOTE_POPUP_KEY_ZATHURA, zathura);
  g_object_set_data(G_OBJECT(popover), NOTE_POPUP_KEY_TEXT_VIEW, text_view);

  /* Connect signals */
  g_signal_connect(popover, "closed", G_CALLBACK(cb_popup_closed), NULL);
  g_signal_connect(text_view, "key-press-event", G_CALLBACK(cb_text_view_key_press), popover);

  return popover;
}

void
zathura_note_popup_show(GtkWidget* popup, GtkWidget* relative_to,
                        double widget_x, double widget_y,
                        unsigned int page, double x, double y,
                        zathura_note_t* note,
                        ZathuraNotePopupSaveCallback callback, void* data) {
  g_return_if_fail(popup != NULL);
  g_return_if_fail(GTK_IS_POPOVER(popup));
  g_return_if_fail(relative_to != NULL);

  /* Store position and callback info */
  g_object_set_data(G_OBJECT(popup), NOTE_POPUP_KEY_PAGE, GUINT_TO_POINTER(page));

  /* Store x and y as allocated doubles (need proper storage for doubles) */
  double* x_ptr = g_new(double, 1);
  double* y_ptr = g_new(double, 1);
  *x_ptr = x;
  *y_ptr = y;
  g_object_set_data_full(G_OBJECT(popup), NOTE_POPUP_KEY_X, x_ptr, g_free);
  g_object_set_data_full(G_OBJECT(popup), NOTE_POPUP_KEY_Y, y_ptr, g_free);

  /* Store note ID if editing existing note */
  if (note != NULL && note->id != NULL) {
    g_object_set_data_full(G_OBJECT(popup), NOTE_POPUP_KEY_NOTE_ID,
                            g_strdup(note->id), g_free);
  } else {
    g_object_set_data(G_OBJECT(popup), NOTE_POPUP_KEY_NOTE_ID, NULL);
  }

  /* Store callback and user data in a struct */
  NotePopupCallbackData* cb_data = g_new(NotePopupCallbackData, 1);
  cb_data->callback = callback;
  cb_data->user_data = data;
  g_object_set_data_full(G_OBJECT(popup), NOTE_POPUP_KEY_CALLBACK_DATA, cb_data, g_free);

  /* Reset cancelled flag */
  g_object_set_data(G_OBJECT(popup), NOTE_POPUP_KEY_CANCELLED, GINT_TO_POINTER(FALSE));

  /* Get text view and set content */
  GtkWidget* text_view = g_object_get_data(G_OBJECT(popup), NOTE_POPUP_KEY_TEXT_VIEW);
  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

  if (note != NULL && note->content != NULL) {
    gtk_text_buffer_set_text(buffer, note->content, -1);
  } else {
    gtk_text_buffer_set_text(buffer, "", -1);
  }

  /* Set the relative widget and show */
  gtk_popover_set_relative_to(GTK_POPOVER(popup), relative_to);

  /* Set the pointing rectangle to anchor popup at click position */
  GdkRectangle pointing_rect = {
    .x = (gint)widget_x,
    .y = (gint)widget_y,
    .width = 1,
    .height = 1
  };
  gtk_popover_set_pointing_to(GTK_POPOVER(popup), &pointing_rect);

  /* Show all widgets */
  gtk_widget_show_all(popup);

  /* Focus the text view */
  gtk_widget_grab_focus(text_view);
}

void
zathura_note_popup_hide(GtkWidget* popup) {
  g_return_if_fail(popup != NULL);
  g_return_if_fail(GTK_IS_POPOVER(popup));

  /* Mark as cancelled so closed callback doesn't save */
  g_object_set_data(G_OBJECT(popup), NOTE_POPUP_KEY_CANCELLED, GINT_TO_POINTER(TRUE));

  /* Hide the popover */
  gtk_popover_popdown(GTK_POPOVER(popup));
}

/**
 * Callback when popover is closed
 * Calls the save callback unless cancelled
 */
static void
cb_popup_closed(GtkPopover* popover, gpointer GIRARA_UNUSED(user_data)) {
  /* Check if this was a cancel */
  gboolean cancelled = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_CANCELLED));
  if (cancelled) {
    return;
  }

  /* Get stored data */
  zathura_t* zathura = g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_ZATHURA);
  NotePopupCallbackData* cb_data = g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_CALLBACK_DATA);

  if (cb_data == NULL || cb_data->callback == NULL || zathura == NULL) {
    return;
  }

  /* Get position */
  unsigned int page = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_PAGE));
  double* x_ptr = g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_X);
  double* y_ptr = g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_Y);

  if (x_ptr == NULL || y_ptr == NULL) {
    return;
  }

  double x = *x_ptr;
  double y = *y_ptr;

  /* Get note ID if editing */
  const char* note_id = g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_NOTE_ID);

  /* Get text content */
  GtkWidget* text_view = g_object_get_data(G_OBJECT(popover), NOTE_POPUP_KEY_TEXT_VIEW);
  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buffer, &start, &end);
  char* content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

  /* Only save if there's content (or if editing existing note) */
  if (content != NULL && (strlen(content) > 0 || note_id != NULL)) {
    cb_data->callback(zathura, page, x, y, content, note_id, cb_data->user_data);
  }

  g_free(content);
}

/**
 * Key press handler for text view
 * Handles Escape (cancel) and Ctrl+Enter (save and close)
 */
static gboolean
cb_text_view_key_press(GtkWidget* GIRARA_UNUSED(widget), GdkEventKey* event, gpointer user_data) {
  GtkWidget* popover = GTK_WIDGET(user_data);

  /* Escape - cancel without saving */
  if (event->keyval == GDK_KEY_Escape) {
    zathura_note_popup_hide(popover);
    return TRUE;
  }

  /* Ctrl+Enter - save and close */
  if ((event->state & GDK_CONTROL_MASK) &&
      (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter)) {
    /* Don't mark as cancelled - let normal close handle save */
    gtk_popover_popdown(GTK_POPOVER(popover));
    return TRUE;
  }

  return FALSE;
}
