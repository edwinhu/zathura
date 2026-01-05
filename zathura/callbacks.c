/* SPDX-License-Identifier: Zlib */

#include <girara/statusbar.h>
#include <girara/session.h>
#include <girara/settings.h>
#include <girara/utils.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>
#include <math.h>

#include "callbacks.h"
#include "links.h"
#include "zathura.h"
#include "render.h"
#include "document.h"
#include "document-widget.h"
#include "utils.h"
#include "shortcuts.h"
#include "page-widget.h"
#include "page.h"
#include "adjustment.h"
#include "synctex.h"
#include "dbus-interface.h"
#include "database.h"
#include "types.h"

gboolean cb_destroy(GtkWidget* UNUSED(widget), zathura_t* zathura) {
  if (zathura_has_document(zathura) == true) {
    document_close(zathura, false);
  }

  gtk_main_quit();
  return TRUE;
}

void cb_buffer_changed(girara_session_t* session) {
  g_return_if_fail(session != NULL);
  g_return_if_fail(session->global.data != NULL);

  zathura_t* zathura = session->global.data;

  char* buffer = girara_buffer_get(session);
  if (buffer != NULL) {
    girara_statusbar_item_set_text(session, zathura->ui.statusbar.buffer, buffer);
    free(buffer);
  } else {
    girara_statusbar_item_set_text(session, zathura->ui.statusbar.buffer, "");
  }
}

void update_visible_pages(zathura_t* zathura) {
  zathura_document_t* document       = zathura_get_document(zathura);
  const unsigned int number_of_pages = zathura_document_get_number_of_pages(document);

  for (unsigned int page_id = 0; page_id < number_of_pages; page_id++) {
    zathura_page_t* page             = zathura_document_get_page(document, page_id);
    GtkWidget* page_widget           = zathura_page_get_widget(zathura, page);
    ZathuraPage* zathura_page_widget = ZATHURA_PAGE(page_widget);

    if (page_is_visible(document, page_id) == true) {
      /* make page visible */
      if (zathura_page_get_visibility(page) == false) {
        zathura_page_set_visibility(page, true);
        zathura_page_widget_update_view_time(zathura_page_widget);
        zathura_renderer_page_cache_add(zathura->sync.render_thread, zathura_page_get_index(page));
      }

    } else {
      /* make page invisible */
      if (zathura_page_get_visibility(page) == true) {
        zathura_page_set_visibility(page, false);
        /* If a page becomes invisible, abort the render request. */
        zathura_page_widget_abort_render_request(zathura_page_widget);
      }

      /* reset current search result */
      girara_list_t* results   = NULL;
      GObject* obj_page_widget = G_OBJECT(page_widget);
      g_object_get(obj_page_widget, "search-results", &results, NULL);
      if (results != NULL) {
        g_object_set(obj_page_widget, "search-current", 0, NULL);
      }
    }
  }
}

void cb_view_hadjustment_value_changed(GtkAdjustment* adjustment, gpointer data) {
  zathura_t* zathura = data;
  if (zathura_has_document(zathura) == false) {
    return;
  }

  /* Do nothing in index mode */
  if (girara_mode_get(zathura->ui.session) == zathura->modes.index) {
    return;
  }

  update_visible_pages(zathura);

  zathura_document_t* document = zathura_get_document(zathura);
  const double position_x      = zathura_document_widget_get_ratio(zathura, adjustment, true);
  const double position_y      = zathura_document_get_position_y(document);
  unsigned int page_id         = position_to_page_number(document, position_x, position_y);

  zathura_document_set_position_x(document, position_x);
  zathura_document_set_position_y(document, position_y);
  zathura_document_set_current_page_number(document, page_id);

  statusbar_page_number_update(zathura);
}

void cb_view_vadjustment_value_changed(GtkAdjustment* adjustment, gpointer data) {
  zathura_t* zathura = data;
  if (zathura_has_document(zathura) == false) {
    return;
  }

  /* Do nothing in index mode */
  if (girara_mode_get(zathura->ui.session) == zathura->modes.index) {
    return;
  }

  update_visible_pages(zathura);

  zathura_document_t* document = zathura_get_document(zathura);
  const double position_x      = zathura_document_get_position_x(document);
  const double position_y      = zathura_document_widget_get_ratio(zathura, adjustment, false);
  const unsigned int page_id   = position_to_page_number(document, position_x, position_y);

  zathura_document_set_position_x(document, position_x);
  zathura_document_set_position_y(document, position_y);
  zathura_document_set_current_page_number(document, page_id);

  statusbar_page_number_update(zathura);
}

static void cb_view_adjustment_changed(GtkAdjustment* adjustment, zathura_t* zathura, bool width) {
  /* Do nothing in index mode */
  if (girara_mode_get(zathura->ui.session) == zathura->modes.index) {
    return;
  }

  zathura_document_t* document            = zathura_get_document(zathura);
  const zathura_adjust_mode_t adjust_mode = zathura_document_get_adjust_mode(document);

  /* Don't scroll, we're focusing the inputbar. */
  if (adjust_mode == ZATHURA_ADJUST_INPUTBAR) {
    return;
  }

  /* Save the viewport size */
  const unsigned int size = floor(gtk_adjustment_get_page_size(adjustment));
  if (width == true) {
    zathura_document_set_viewport_width(document, size);
  } else {
    zathura_document_set_viewport_height(document, size);
  }

  /* reset the adjustment, in case bounds have changed */
  const double ratio =
      width == true ? zathura_document_get_position_x(document) : zathura_document_get_position_y(document);

  zathura_document_widget_set_value_from_ratio(zathura, adjustment, ratio, width);
}

void cb_view_hadjustment_changed(GtkAdjustment* adjustment, gpointer data) {
  zathura_t* zathura = data;
  g_return_if_fail(zathura != NULL);

  cb_view_adjustment_changed(adjustment, zathura, true);
}

void cb_view_vadjustment_changed(GtkAdjustment* adjustment, gpointer data) {
  zathura_t* zathura = data;
  g_return_if_fail(zathura != NULL);

  cb_view_adjustment_changed(adjustment, zathura, false);
}

void cb_refresh_view(GtkWidget* GIRARA_UNUSED(view), gpointer data) {
  zathura_t* zathura = data;
  if (zathura_has_document(zathura) == false) {
    return;
  }

  zathura_document_t* document = zathura_get_document(zathura);
  unsigned int page_id         = zathura_document_get_current_page_number(document);
  zathura_page_t* page         = zathura_document_get_page(document, page_id);
  if (page == NULL) {
    return;
  }

  if (zathura->pages != NULL && zathura->pages[page_id] != NULL) {
    ZathuraPage* page_widget = ZATHURA_PAGE(zathura->pages[page_id]);
    if (page_widget != NULL) {
      if (zathura_page_widget_have_surface(page_widget)) {
        document_predecessor_free(zathura);
      }
    }
  }

  GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(zathura->ui.view));
  GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(zathura->ui.view));

  const double position_x = zathura_document_get_position_x(document);
  const double position_y = zathura_document_get_position_y(document);

  zathura_document_widget_set_value_from_ratio(zathura, vadj, position_y, false);
  zathura_document_widget_set_value_from_ratio(zathura, hadj, position_x, true);

  statusbar_page_number_update(zathura);
}

void cb_monitors_changed(GdkScreen* screen, gpointer data) {
  girara_debug("signal received");

  zathura_t* zathura = data;
  if (screen == NULL || zathura == NULL) {
    return;
  }

  zathura_update_view_ppi(zathura);
}

void cb_widget_screen_changed(GtkWidget* widget, GdkScreen* previous_screen, gpointer data) {
  girara_debug("called");

  zathura_t* zathura = data;
  if (widget == NULL || zathura == NULL) {
    return;
  }

  /* disconnect previous screen handler if present */
  if (previous_screen != NULL && zathura->signals.monitors_changed_handler > 0) {
    g_signal_handler_disconnect(previous_screen, zathura->signals.monitors_changed_handler);
    zathura->signals.monitors_changed_handler = 0;
  }

  if (gtk_widget_has_screen(widget)) {
    GdkScreen* screen = gtk_widget_get_screen(widget);

    /* connect to new screen */
    zathura->signals.monitors_changed_handler =
        g_signal_connect(G_OBJECT(screen), "monitors-changed", G_CALLBACK(cb_monitors_changed), zathura);
  }

  zathura_update_view_ppi(zathura);
}

gboolean cb_widget_configured(GtkWidget* UNUSED(widget), GdkEvent* UNUSED(event), gpointer data) {
  zathura_t* zathura = data;
  if (zathura == NULL) {
    return false;
  }

  zathura_update_view_ppi(zathura);

  return false;
}

void cb_scale_factor(GObject* object, GParamSpec* UNUSED(pspec), gpointer data) {
  zathura_t* zathura = data;
  if (zathura_has_document(zathura) == false) {
    return;
  }

  int new_factor = gtk_widget_get_scale_factor(GTK_WIDGET(object));
  if (new_factor == 0) {
    girara_debug("Ignoring new device scale factor = 0");
    return;
  }

  zathura_document_t* document     = zathura_get_document(zathura);
  zathura_device_factors_t current = zathura_document_get_device_factors(document);
  if (fabs(new_factor - current.x) >= DBL_EPSILON || fabs(new_factor - current.y) >= DBL_EPSILON) {
    zathura_document_set_device_factors(document, new_factor, new_factor);
    girara_debug("New device scale factor: %d", new_factor);
    zathura_update_view_ppi(zathura);
    render_all(zathura);
  }
}

void cb_page_layout_value_changed(girara_session_t* session, const char* name, girara_setting_type_t UNUSED(type),
                                  const void* value, void* UNUSED(data)) {
  g_return_if_fail(value != NULL);
  g_return_if_fail(session != NULL);
  g_return_if_fail(session->global.data != NULL);
  zathura_t* zathura = session->global.data;

  /* pages-per-row must not be 0 */
  if (g_strcmp0(name, "pages-per-row") == 0) {
    unsigned int pages_per_row = *((const unsigned int*)value);
    if (pages_per_row == 0) {
      pages_per_row = 1;
      girara_setting_set(session, name, &pages_per_row);
      girara_notify(session, GIRARA_WARNING, _("'%s' must not be 0. Set to 1."), name);
      return;
    }
  }

  if (zathura_has_document(zathura) == false) {
    /* no document has been openend yet */
    return;
  }

  unsigned int pages_per_row = 1;
  girara_setting_get(session, "pages-per-row", &pages_per_row);

  /* get list of first_page_column settings */
  g_autofree char* first_page_column_list = NULL;
  girara_setting_get(session, "first-page-column", &first_page_column_list);

  /* find value for first_page_column */
  unsigned int first_page_column = find_first_page_column(first_page_column_list, pages_per_row);

  unsigned int page_v_padding = 1;
  girara_setting_get(zathura->ui.session, "page-v-padding", &page_v_padding);

  unsigned int page_h_padding = 1;
  girara_setting_get(zathura->ui.session, "page-h-padding", &page_h_padding);

  bool page_right_to_left = false;
  girara_setting_get(zathura->ui.session, "page-right-to-left", &page_right_to_left);

  zathura_document_set_page_layout(zathura_get_document(zathura), page_v_padding, page_h_padding, pages_per_row,
                                   first_page_column);
  zathura_document_widget_set_mode(zathura, page_right_to_left);
}

void cb_index_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* UNUSED(column), void* data) {
  zathura_t* zathura = data;
  if (tree_view == NULL || zathura == NULL || zathura->ui.session == NULL) {
    return;
  }

  GtkTreeModel* model;
  GtkTreeIter iter;

  g_object_get(G_OBJECT(tree_view), "model", &model, NULL);

  if (gtk_tree_model_get_iter(model, &iter, path)) {
    zathura_index_element_t* index_element;
    gtk_tree_model_get(model, &iter, 3, &index_element, -1);

    if (index_element == NULL) {
      return;
    }

    sc_toggle_index(zathura->ui.session, NULL, NULL, 0);
    zathura_link_evaluate(zathura, index_element->link);
  }

  g_object_unref(model);
}

void cb_highlights_row_activated(GtkTreeView* tree_view, GtkTreePath* path,
                                  GtkTreeViewColumn* UNUSED(column), void* data) {
  zathura_t* zathura = data;
  GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter(model, &iter, path)) {
    zathura_highlight_t* highlight = NULL;
    gtk_tree_model_get(model, &iter, 3, &highlight, -1);

    if (highlight != NULL) {
      // Jump to page (do NOT close panel)
      page_set(zathura, highlight->page);
    }
  }
}

gboolean cb_highlights_key_press(GtkWidget* widget, GdkEventKey* event, void* data) {
  zathura_t* zathura = data;
  GtkTreeView* tree_view = GTK_TREE_VIEW(widget);

  // Handle Escape before selection check (works even with empty list)
  if (event->keyval == GDK_KEY_Escape) {
    gtk_widget_hide(zathura->ui.highlights);
    gtk_widget_grab_focus(zathura->ui.view);
    return TRUE;
  }

  // Get selected row
  GtkTreeSelection* selection = gtk_tree_view_get_selection(tree_view);
  GtkTreeModel* model;
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
    return FALSE;
  }

  zathura_highlight_t* highlight = NULL;
  gtk_tree_model_get(model, &iter, 3, &highlight, -1);

  if (highlight == NULL) {
    return FALSE;
  }

  const char* file_path = zathura_document_get_path(zathura->document);

  switch (event->keyval) {
    case GDK_KEY_d:
    case GDK_KEY_x: {
      // Delete highlight
      char* id_copy = g_strdup(highlight->id);
      if (zathura_db_remove_highlight(zathura->database, file_path, id_copy)) {
        // Remove from page widget
        zathura_page_t* page = zathura_document_get_page(zathura->document, highlight->page);
        if (page != NULL) {
          GtkWidget* page_widget = zathura_page_get_widget(zathura, page);
          if (page_widget != NULL) {
            zathura_page_widget_remove_highlight(ZATHURA_PAGE(page_widget), id_copy);
          }
        }
        // Refresh panel by hiding and showing
        gtk_widget_hide(zathura->ui.highlights);
        sc_toggle_highlights(zathura->ui.session, NULL, NULL, 0);
        girara_notify(zathura->ui.session, GIRARA_INFO, _("Highlight deleted"));
      }
      g_free(id_copy);
      return TRUE;
    }

    case GDK_KEY_c: {
      // Cycle color
      zathura_highlight_color_t new_color = (highlight->color + 1) % 4;
      highlight->color = new_color;
      zathura_db_add_highlight(zathura->database, file_path, highlight);

      // Also update the page widget's highlight
      zathura_page_t* page = zathura_document_get_page(zathura->document, highlight->page);
      if (page != NULL) {
        GtkWidget* page_widget = zathura_page_get_widget(zathura, page);
        if (page_widget != NULL) {
          girara_list_t* page_highlights = zathura_page_widget_get_highlights(ZATHURA_PAGE(page_widget));
          if (page_highlights != NULL) {
            GIRARA_LIST_FOREACH_BODY(page_highlights, zathura_highlight_t*, h,
              if (h->id != NULL && highlight->id != NULL && g_strcmp0(h->id, highlight->id) == 0) {
                h->color = new_color;
              }
            );
          }
          gtk_widget_queue_draw(page_widget);  // Trigger redraw with new color
        }
      }

      // Refresh panel and view
      gtk_widget_hide(zathura->ui.highlights);
      sc_toggle_highlights(zathura->ui.session, NULL, NULL, 0);
      render_all(zathura);

      const char* color_names[] = {"Yellow", "Green", "Blue", "Red"};
      girara_notify(zathura->ui.session, GIRARA_INFO, _("Color changed to %s"), color_names[new_color]);
      return TRUE;
    }

  }

  return FALSE;
}

void cb_highlights_search_changed(GtkEditable* UNUSED(editable), void* data) {
  GtkTreeModelFilter* filter = GTK_TREE_MODEL_FILTER(data);
  gtk_tree_model_filter_refilter(filter);
}

gboolean cb_highlights_search_key_press(GtkWidget* widget, GdkEventKey* event, void* data) {
  GtkEntry* entry = GTK_ENTRY(widget);
  GtkTreeModelFilter* filter = GTK_TREE_MODEL_FILTER(data);
  zathura_t* zathura = g_object_get_data(G_OBJECT(widget), "zathura");

  // Escape to close highlights panel
  if (event->keyval == GDK_KEY_Escape) {
    if (zathura != NULL && zathura->ui.highlights != NULL) {
      gtk_widget_hide(zathura->ui.highlights);
      if (zathura->ui.view != NULL) {
        gtk_widget_grab_focus(zathura->ui.view);
      }
    }
    return TRUE;
  }

  // Toggle fuzzy/substring mode on Tab key
  if (event->keyval == GDK_KEY_Tab) {
    // Get current mode
    gboolean fuzzy_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "fuzzy_mode"));

    // Toggle mode
    fuzzy_mode = !fuzzy_mode;
    g_object_set_data(G_OBJECT(entry), "fuzzy_mode", GINT_TO_POINTER(fuzzy_mode));

    // Update placeholder text
    if (fuzzy_mode) {
      gtk_entry_set_placeholder_text(entry, "Search [Tab: exact]...");
    } else {
      gtk_entry_set_placeholder_text(entry, "Search [Tab: fuzzy]...");
    }

    // Refilter to apply new mode
    gtk_tree_model_filter_refilter(filter);

    return TRUE;  // Consume the event
  }

  return FALSE;  // Let other handlers process the event
}

void cb_notes_row_activated(GtkTreeView* tree_view, GtkTreePath* path,
                            GtkTreeViewColumn* UNUSED(column), void* data) {
  zathura_t* zathura = data;
  GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter(model, &iter, path)) {
    zathura_note_t* note = NULL;
    gtk_tree_model_get(model, &iter, 2, &note, -1);

    if (note != NULL) {
      // Jump to page (do NOT close panel)
      page_set(zathura, note->page);

      // Optionally: open popup for editing
      // For now, just jump to page
    }
  }
}

gboolean cb_notes_key_press(GtkWidget* widget, GdkEventKey* event, void* data) {
  zathura_t* zathura = data;
  GtkTreeView* tree_view = GTK_TREE_VIEW(widget);

  // Handle Escape before selection check (works even with empty list)
  if (event->keyval == GDK_KEY_Escape) {
    gtk_widget_hide(zathura->ui.notes);
    gtk_widget_grab_focus(zathura->ui.view);
    return TRUE;
  }

  // Get selected row
  GtkTreeSelection* selection = gtk_tree_view_get_selection(tree_view);
  GtkTreeModel* model;
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
    return FALSE;
  }

  zathura_note_t* note = NULL;
  gtk_tree_model_get(model, &iter, 2, &note, -1);

  if (note == NULL) {
    return FALSE;
  }

  const char* file_path = zathura_document_get_path(zathura->document);

  switch (event->keyval) {
    case GDK_KEY_x: {
      // Delete note - save coordinates before any operations
      char* id_copy = g_strdup(note->id);
      unsigned int note_page = note->page;
      double note_x = note->x;
      double note_y = note->y;

      if (zathura_db_remove_note(zathura->database, file_path, id_copy)) {
        // Remove from page widget
        zathura_page_t* page = zathura_document_get_page(zathura->document, note_page);
        if (page != NULL) {
          GtkWidget* page_widget = zathura_page_get_widget(zathura, page);
          if (page_widget != NULL) {
            zathura_page_widget_remove_note(ZATHURA_PAGE(page_widget), id_copy);
            gtk_widget_queue_draw(page_widget);
          }

          // Also delete from embedded PDF (blue annotation)
          // Create a small rectangle around the note position for deletion
          girara_list_t* rects = girara_list_new2(g_free);
          zathura_rectangle_t* rect = g_malloc(sizeof(zathura_rectangle_t));
          // Note icons are typically small - use a 1x1 point around the position
          rect->x1 = note_x;
          rect->y1 = note_y;
          rect->x2 = note_x + 1.0;
          rect->y2 = note_y + 1.0;
          girara_list_append(rects, rect);
          zathura_error_t err = zathura_page_delete_annotation(page, rects);
          girara_list_free(rects);
          if (err == ZATHURA_ERROR_OK) {
            zathura_document_save_as(zathura->document, file_path);
          }
        }
        // Refresh panel by hiding and showing
        gtk_widget_hide(zathura->ui.notes);
        sc_toggle_notes(zathura->ui.session, NULL, NULL, 0);
        girara_notify(zathura->ui.session, GIRARA_INFO, _("Note deleted"));
      }
      g_free(id_copy);
      return TRUE;
    }

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter: {
      // Jump to page
      page_set(zathura, note->page);
      return TRUE;
    }

    default:
      break;
  }

  return FALSE;
}

void cb_notes_search_changed(GtkEditable* UNUSED(editable), void* data) {
  GtkTreeModelFilter* filter = GTK_TREE_MODEL_FILTER(data);
  gtk_tree_model_filter_refilter(filter);
}

gboolean cb_notes_search_key_press(GtkWidget* widget, GdkEventKey* event, void* data) {
  GtkEntry* entry = GTK_ENTRY(widget);
  GtkTreeModelFilter* filter = GTK_TREE_MODEL_FILTER(data);
  zathura_t* zathura = g_object_get_data(G_OBJECT(widget), "zathura");

  // Escape to close notes panel
  if (event->keyval == GDK_KEY_Escape) {
    if (zathura != NULL && zathura->ui.notes != NULL) {
      gtk_widget_hide(zathura->ui.notes);
      if (zathura->ui.view != NULL) {
        gtk_widget_grab_focus(zathura->ui.view);
      }
    }
    return TRUE;
  }

  // Toggle fuzzy/substring mode on Tab key
  if (event->keyval == GDK_KEY_Tab) {
    // Get current mode
    gboolean fuzzy_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "fuzzy_mode"));

    // Toggle mode
    fuzzy_mode = !fuzzy_mode;
    g_object_set_data(G_OBJECT(entry), "fuzzy_mode", GINT_TO_POINTER(fuzzy_mode));

    // Update placeholder text
    if (fuzzy_mode) {
      gtk_entry_set_placeholder_text(entry, "Search [Tab: exact]...");
    } else {
      gtk_entry_set_placeholder_text(entry, "Search [Tab: fuzzy]...");
    }

    // Refilter to apply new mode
    gtk_tree_model_filter_refilter(filter);

    return TRUE;  // Consume the event
  }

  return FALSE;  // Let other handlers process the event
}

/* File picker callbacks */

/* Structure for deferred file open */
typedef struct {
  zathura_t* zathura;
  char* file_path;
  int page_num;  /* Page to navigate to (1-indexed from rga, 0 means no specific page) */
} file_picker_open_data_t;

/* Structure for content search */
typedef struct {
  zathura_t* zathura;
  char* query;
} file_picker_search_data_t;

/* Idle callback to open file after event handler returns */
static gboolean file_picker_deferred_open(gpointer user_data) {
  file_picker_open_data_t* data = user_data;
  zathura_t* zathura = data->zathura;
  char* file_path = data->file_path;

  g_message("FILE_PICKER: deferred_open called for '%s'", file_path ? file_path : "(null)");

  if (zathura == NULL || file_path == NULL) {
    g_free(file_path);
    g_free(data);
    return G_SOURCE_REMOVE;
  }

  /* Destroy file picker paned to restore original widget hierarchy */
  if (zathura->ui.file_picker_paned != NULL) {
    GtkWidget* paned = zathura->ui.file_picker_paned;
    /* main_widget is in child1 (pack1=main_widget, pack2=file_picker) */
    GtkWidget* main_widget = gtk_paned_get_child1(GTK_PANED(paned));
    GtkWidget* parent = gtk_widget_get_parent(paned);

    if (main_widget != NULL && parent != NULL) {
      g_object_ref(main_widget);
      gtk_container_remove(GTK_CONTAINER(paned), main_widget);
      gtk_container_remove(GTK_CONTAINER(parent), paned);
      gtk_container_add(GTK_CONTAINER(parent), main_widget);
      g_object_unref(main_widget);

      /* Immediately set visible child and ensure widget is shown */
      if (GTK_IS_STACK(parent)) {
        gtk_widget_set_visible(main_widget, TRUE);
        gtk_stack_set_visible_child(GTK_STACK(parent), main_widget);
        gtk_widget_grab_focus(main_widget);
        g_message("FILE_PICKER: Set visible child on stack after re-adding main_widget=%p", (void*)main_widget);
      }

      /* Destroy the paned and its children (file_picker) */
      gtk_widget_destroy(paned);
    }

    zathura->ui.file_picker_paned = NULL;
    zathura->ui.file_picker = NULL;
    zathura->ui.file_picker_search = NULL;
  }

  /* Close current document if any */
  if (zathura_has_document(zathura)) {
    document_close(zathura, false);
  }

  /* Open the selected file */
  g_message("FILE_PICKER: Calling document_open for '%s'", file_path);
  g_message("FILE_PICKER: Before document_open - mode=%u, view=%p, session=%p",
            girara_mode_get(zathura->ui.session),
            (void*)zathura->ui.view,
            (void*)zathura->ui.session);

  bool open_result = document_open(zathura, file_path, NULL, NULL, ZATHURA_PAGE_NUMBER_UNSPECIFIED, NULL);

  g_message("FILE_PICKER: After document_open - result=%d, mode=%u, view parent=%p, session->gtk.view=%p",
            open_result,
            girara_mode_get(zathura->ui.session),
            (void*)gtk_widget_get_parent(zathura->ui.view),
            (void*)zathura->ui.session->gtk.view);

  /* Ensure focus is properly set to the view after document open */
  if (open_result && zathura->ui.view != NULL) {
    g_message("FILE_PICKER: Explicitly grabbing focus on view");
    gtk_widget_grab_focus(zathura->ui.view);

    /* Navigate to specific page if set (from content search results) */
    if (data->page_num > 0 && zathura->document != NULL) {
      unsigned int num_pages = zathura_document_get_number_of_pages(zathura->document);
      unsigned int page_id = (unsigned int)(data->page_num - 1);  /* Convert 1-indexed to 0-indexed */
      if (page_id < num_pages) {
        g_message("FILE_PICKER: Navigating to page %d (0-indexed: %u)", data->page_num, page_id);
        page_set(zathura, page_id);
      }
    }

    /* Debug: Check what has focus after grabbing */
    GtkWidget* focused = gtk_window_get_focus(GTK_WINDOW(zathura->ui.session->gtk.window));
    g_message("FILE_PICKER: After grab_focus - focused widget=%p (view=%p, stack=%p)",
              (void*)focused, (void*)zathura->ui.view, (void*)zathura->ui.session->gtk.view);
    g_message("FILE_PICKER: view is in stack: %s",
              gtk_widget_get_parent(zathura->ui.view) == zathura->ui.session->gtk.view ? "YES" : "NO");
    g_message("FILE_PICKER: stack visible child=%p",
              (void*)gtk_stack_get_visible_child(GTK_STACK(zathura->ui.session->gtk.view)));
    g_message("FILE_PICKER: view_key_pressed signal handler ID=%lu",
              zathura->ui.session->signals.view_key_pressed);
    g_message("FILE_PICKER: window can_focus=%d, view can_focus=%d",
              gtk_widget_get_can_focus(zathura->ui.session->gtk.window),
              gtk_widget_get_can_focus(zathura->ui.view));
  }

  g_free(file_path);
  g_free(data);
  return G_SOURCE_REMOVE;
}

/* Helper to schedule deferred file open */
static void file_picker_schedule_open(zathura_t* zathura, const char* file_path) {
  g_message("FILE_PICKER: scheduling deferred open for '%s'", file_path ? file_path : "(null)");
  file_picker_open_data_t* data = g_new0(file_picker_open_data_t, 1);
  data->zathura = zathura;
  data->file_path = g_strdup(file_path);
  data->page_num = 0;
  g_idle_add(file_picker_deferred_open, data);
}

/* Helper to schedule deferred file open at specific page */
void file_picker_schedule_open_at_page(zathura_t* zathura, const char* file_path, int page_num) {
  g_message("FILE_PICKER: scheduling deferred open for '%s' at page %d", file_path ? file_path : "(null)", page_num);
  file_picker_open_data_t* data = g_new0(file_picker_open_data_t, 1);
  data->zathura = zathura;
  data->file_path = g_strdup(file_path);
  data->page_num = page_num;
  g_idle_add(file_picker_deferred_open, data);
}

/* Callback for rga subprocess completion */
static void file_picker_rga_callback(GObject* source_object, GAsyncResult* res, gpointer user_data) {
  GSubprocess* proc = G_SUBPROCESS(source_object);
  file_picker_search_data_t* search_data = user_data;
  zathura_t* zathura = search_data->zathura;
  GError* error = NULL;

  char* stdout_buf = NULL;
  char* stderr_buf = NULL;

  g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, &stderr_buf, &error);

  if (error != NULL) {
    g_message("FILE_PICKER: rga error: %s", error->message);
    g_error_free(error);
    g_free(search_data->query);
    g_free(search_data);
    g_free(stdout_buf);
    g_free(stderr_buf);
    return;
  }

  /* Check if file picker still exists and we're still in content mode */
  if (zathura->ui.file_picker == NULL) {
    g_free(search_data->query);
    g_free(search_data);
    g_free(stdout_buf);
    g_free(stderr_buf);
    return;
  }

  GtkEntry* entry = GTK_ENTRY(zathura->ui.file_picker_search);
  gboolean content_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "content_search_mode"));
  if (!content_mode) {
    g_free(search_data->query);
    g_free(search_data);
    g_free(stdout_buf);
    g_free(stderr_buf);
    return;
  }

  /* Clear and populate results */
  GtkListStore* store = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "store");
  gtk_list_store_clear(store);

  if (stdout_buf != NULL && stdout_buf[0] != '\0') {
    /* Parse rga output: filepath:line_num:Page N: content
     * The page number is embedded in the content as "Page N:" prefix */
    char** lines = g_strsplit(stdout_buf, "\n", -1);
    for (int i = 0; lines[i] != NULL && lines[i][0] != '\0'; i++) {
      char* line = lines[i];

      /* Find first colon (end of filepath) */
      char* first_colon = strchr(line, ':');
      if (first_colon == NULL) continue;

      /* Find second colon (end of line number) */
      char* second_colon = strchr(first_colon + 1, ':');
      if (second_colon == NULL) continue;

      /* Extract components */
      *first_colon = '\0';
      char* file_path = line;
      char* content = second_colon + 1;

      /* Extract page number from content "Page N: ..." */
      int page_num = 1;  /* Default to page 1 */
      if (strncmp(content, "Page ", 5) == 0) {
        page_num = atoi(content + 5);
        /* Skip past "Page N: " to get actual content */
        char* content_start = strchr(content + 5, ':');
        if (content_start != NULL) {
          content = content_start + 1;
          while (*content == ' ') content++;  /* Skip leading spaces */
        }
      }

      /* Get filename from path */
      char* filename = strrchr(file_path, '/');
      filename = filename ? filename + 1 : file_path;

      /* Create display text: filename:page - context */
      char display[512];
      snprintf(display, sizeof(display), "%s:%d - %.60s%s",
               filename, page_num, content,
               strlen(content) > 60 ? "..." : "");

      GtkTreeIter iter;
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
          0, file_path,    /* Full path */
          1, display,      /* Display text */
          2, page_num,     /* Page number */
          3, content,      /* Context snippet */
          -1);
    }
    g_strfreev(lines);
  }

  /* Select first row */
  GtkWidget* treeview = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "treeview");
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
  GtkTreeIter first_iter;
  if (gtk_tree_model_get_iter_first(model, &first_iter)) {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_select_iter(selection, &first_iter);
  }

  g_free(search_data->query);
  g_free(search_data);
  g_free(stdout_buf);
  g_free(stderr_buf);
}

/* Debounce timer callback for content search */
static gboolean file_picker_content_search_timer_cb(gpointer user_data) {
  file_picker_search_data_t* search_data = user_data;
  zathura_t* zathura = search_data->zathura;

  /* Check if file picker still exists */
  if (zathura->ui.file_picker == NULL || zathura->ui.file_picker_search == NULL) {
    g_free(search_data->query);
    g_free(search_data);
    return G_SOURCE_REMOVE;
  }

  /* Clear timer ID */
  g_object_set_data(G_OBJECT(zathura->ui.file_picker_search), "content_search_timer", NULL);

  const char* query = search_data->query;
  if (query == NULL || query[0] == '\0') {
    /* Empty query - clear results */
    GtkListStore* store = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "store");
    if (store != NULL) {
      gtk_list_store_clear(store);
    }
    g_free(search_data->query);
    g_free(search_data);
    return G_SOURCE_REMOVE;
  }

  g_message("FILE_PICKER: Running rga search for '%s'", query);

  /* Build search paths */
  const char* home = g_get_home_dir();
  char* downloads = g_build_filename(home, "Downloads", NULL);
  char* documents = g_build_filename(home, "Documents", NULL);
  char* projects = g_build_filename(home, "projects", NULL);

  /* Spawn rga subprocess */
  GError* error = NULL;
  GSubprocess* proc = g_subprocess_new(
      G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
      &error,
      "rga", "--color=never", "--no-heading", "--line-number",
      "--max-count=50", "--type=pdf", query,
      downloads, documents, projects,
      NULL);

  g_free(downloads);
  g_free(documents);
  g_free(projects);

  if (error != NULL) {
    g_message("FILE_PICKER: Failed to spawn rga: %s", error->message);
    g_error_free(error);
    g_free(search_data->query);
    g_free(search_data);
    return G_SOURCE_REMOVE;
  }

  /* Run async and handle results in callback */
  g_subprocess_communicate_utf8_async(proc, NULL, NULL, file_picker_rga_callback, search_data);
  g_object_unref(proc);

  return G_SOURCE_REMOVE;
}

/* Trigger content search with debouncing */
void file_picker_trigger_content_search(zathura_t* zathura, const char* query) {
  g_message("FILE_PICKER: trigger_content_search called with query='%s'", query ? query : "(null)");
  if (zathura->ui.file_picker_search == NULL) {
    g_message("FILE_PICKER: file_picker_search is NULL, aborting");
    return;
  }

  /* Cancel existing timer */
  guint timer_id = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(zathura->ui.file_picker_search), "content_search_timer"));
  if (timer_id > 0) {
    g_source_remove(timer_id);
  }

  /* Create search data */
  file_picker_search_data_t* search_data = g_new0(file_picker_search_data_t, 1);
  search_data->zathura = zathura;
  search_data->query = g_strdup(query);

  /* Start debounce timer (300ms) */
  timer_id = g_timeout_add(300, file_picker_content_search_timer_cb, search_data);
  g_object_set_data(G_OBJECT(zathura->ui.file_picker_search), "content_search_timer", GUINT_TO_POINTER(timer_id));
}

/* Refresh file list (when switching from content to filename mode) */
void file_picker_refresh_file_list(zathura_t* zathura) {
  if (zathura->ui.file_picker == NULL) {
    return;
  }

  GtkListStore* store = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "store");
  if (store == NULL) {
    return;
  }

  gtk_list_store_clear(store);

  /* Scan default directories */
  const char* home = g_get_home_dir();
  char* downloads = g_build_filename(home, "Downloads", NULL);
  char* documents = g_build_filename(home, "Documents", NULL);
  char* projects = g_build_filename(home, "projects", NULL);

  /* These functions are in shortcuts.c - we need forward declarations */
  extern void file_picker_scan_directory(const char* path, GtkListStore* store, int depth);
  file_picker_scan_directory(downloads, store, 0);
  file_picker_scan_directory(documents, store, 0);
  file_picker_scan_directory(projects, store, 0);
  file_picker_scan_directory("/tmp", store, 0);

  g_free(downloads);
  g_free(documents);
  g_free(projects);

  /* Select first row */
  GtkWidget* treeview = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "treeview");
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
  GtkTreeIter first_iter;
  if (gtk_tree_model_get_iter_first(model, &first_iter)) {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_select_iter(selection, &first_iter);
  }

  /* Refilter with current search text */
  GtkTreeModelFilter* filter = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "filter");
  if (filter != NULL) {
    gtk_tree_model_filter_refilter(filter);
  }
}

void cb_file_picker_row_activated(GtkTreeView* tree_view, GtkTreePath* path,
                                   GtkTreeViewColumn* UNUSED(column), void* data) {
  zathura_t* zathura = data;
  g_return_if_fail(zathura != NULL);

  GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter(model, &iter, path)) {
    char* file_path = NULL;
    gtk_tree_model_get(model, &iter, 0, &file_path, -1);

    if (file_path != NULL) {
      /* Defer file open to avoid widget destruction during event callback */
      file_picker_schedule_open(zathura, file_path);
      g_free(file_path);
    }
  }
}

gboolean cb_file_picker_key_press(GtkWidget* widget, GdkEventKey* event, void* data) {
  zathura_t* zathura = data;
  g_return_val_if_fail(zathura != NULL, FALSE);

  GtkTreeView* treeview = GTK_TREE_VIEW(widget);

  /* Escape to close picker */
  if (event->keyval == GDK_KEY_Escape) {
    gtk_widget_hide(zathura->ui.file_picker);
    if (zathura->ui.view != NULL) {
      gtk_widget_grab_focus(zathura->ui.view);
    }
    return TRUE;
  }

  /* Enter to open selected file */
  if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
    g_message("FILE_PICKER: Enter pressed on treeview");
    GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel* model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
      char* file_path = NULL;
      gtk_tree_model_get(model, &iter, 0, &file_path, -1);
      g_message("FILE_PICKER: Selected file_path='%s'", file_path ? file_path : "(null)");

      if (file_path != NULL) {
        /* Defer file open to avoid widget destruction during event callback */
        file_picker_schedule_open(zathura, file_path);
        g_free(file_path);
      }
    } else {
      g_message("FILE_PICKER: No selection found");
    }
    return TRUE;
  }

  /* Type to focus search entry */
  if (event->keyval >= GDK_KEY_space && event->keyval <= GDK_KEY_asciitilde) {
    gtk_widget_grab_focus(zathura->ui.file_picker_search);
    /* Forward the keypress to search entry */
    GdkEvent* new_event = gdk_event_copy((GdkEvent*)event);
    gtk_widget_event(zathura->ui.file_picker_search, new_event);
    gdk_event_free(new_event);
    return TRUE;
  }

  return FALSE;
}

void cb_file_picker_search_changed(GtkEditable* editable, void* data) {
  GtkEntry* entry = GTK_ENTRY(editable);
  zathura_t* zathura = g_object_get_data(G_OBJECT(entry), "zathura");

  gboolean content_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "content_search_mode"));

  if (content_mode) {
    /* In content mode, trigger debounced rga search */
    const char* text = gtk_entry_get_text(entry);
    if (zathura != NULL) {
      file_picker_trigger_content_search(zathura, text);
    }
  } else {
    /* In filename mode, refilter the existing list */
    GtkTreeModelFilter* filter = GTK_TREE_MODEL_FILTER(data);
    gtk_tree_model_filter_refilter(filter);
  }
}

gboolean cb_file_picker_search_key_press(GtkWidget* widget, GdkEventKey* event, void* data) {
  zathura_t* zathura = data;
  g_return_val_if_fail(zathura != NULL, FALSE);

  g_message("FILE_PICKER: search_key_press called, keyval=%u", event->keyval);

  GtkEntry* entry = GTK_ENTRY(widget);

  /* Escape to close picker */
  if (event->keyval == GDK_KEY_Escape) {
    gtk_widget_hide(zathura->ui.file_picker);
    if (zathura->ui.view != NULL) {
      gtk_widget_grab_focus(zathura->ui.view);
    }
    return TRUE;
  }

  /* Ctrl+T to toggle content search mode */
  g_message("FILE_PICKER: Checking Ctrl+T: keyval=%u (GDK_KEY_t=%u), state=0x%x, ctrl_mask=0x%x",
            event->keyval, GDK_KEY_t, event->state, GDK_CONTROL_MASK);
  if (event->keyval == GDK_KEY_t && (event->state & GDK_CONTROL_MASK)) {
    g_message("FILE_PICKER: Ctrl+T detected, toggling content search mode");
    gboolean content_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "content_search_mode"));
    content_mode = !content_mode;
    g_object_set_data(G_OBJECT(entry), "content_search_mode", GINT_TO_POINTER(content_mode));

    if (content_mode) {
      gtk_entry_set_placeholder_text(entry, "[Ctrl+T: filename] Search PDF content...");
      /* Clear file list - will be populated by rga results */
      GtkListStore* store = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "store");
      if (store != NULL) {
        gtk_list_store_clear(store);
      }
      /* Trigger search if there's existing text */
      const char* text = gtk_entry_get_text(entry);
      if (text != NULL && text[0] != '\0') {
        file_picker_trigger_content_search(zathura, text);
      }
    } else {
      gboolean fuzzy_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "fuzzy_mode"));
      if (fuzzy_mode) {
        gtk_entry_set_placeholder_text(entry, "Search files [Tab: exact] [Ctrl+T: content]...");
      } else {
        gtk_entry_set_placeholder_text(entry, "Search files [Tab: fuzzy] [Ctrl+T: content]...");
      }
      /* Refresh file list */
      file_picker_refresh_file_list(zathura);
    }
    return TRUE;
  }

  /* Tab to toggle fuzzy/exact mode (only in filename mode) */
  if (event->keyval == GDK_KEY_Tab) {
    gboolean content_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "content_search_mode"));
    if (content_mode) {
      return TRUE; /* Ignore Tab in content mode */
    }

    gboolean fuzzy_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "fuzzy_mode"));
    fuzzy_mode = !fuzzy_mode;
    g_object_set_data(G_OBJECT(entry), "fuzzy_mode", GINT_TO_POINTER(fuzzy_mode));

    if (fuzzy_mode) {
      gtk_entry_set_placeholder_text(entry, "Search files [Tab: exact] [Ctrl+T: content]...");
    } else {
      gtk_entry_set_placeholder_text(entry, "Search files [Tab: fuzzy] [Ctrl+T: content]...");
    }

    GtkTreeModelFilter* filter = g_object_get_data(
        G_OBJECT(zathura->ui.file_picker), "filter");
    if (filter != NULL) {
      gtk_tree_model_filter_refilter(filter);
    }
    return TRUE;
  }

  /* Enter to open first/selected file */
  if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
    g_message("FILE_PICKER: Enter pressed on search entry");
    GtkWidget* treeview = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "treeview");
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkTreeIter iter;

    /* Try to get selected, or get first visible row */
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
      g_message("FILE_PICKER: No selection, trying first row");
      if (!gtk_tree_model_get_iter_first(model, &iter)) {
        g_message("FILE_PICKER: No rows in model");
        return TRUE; /* No items */
      }
    }

    char* file_path = NULL;
    gint page_num = 0;
    gtk_tree_model_get(model, &iter, 0, &file_path, 2, &page_num, -1);
    g_message("FILE_PICKER: file_path='%s', page=%d", file_path ? file_path : "(null)", page_num);

    if (file_path != NULL) {
      /* Defer file open to avoid widget destruction during event callback */
      /* Pass page number for content search results (0 means no specific page) */
      file_picker_schedule_open_at_page(zathura, file_path, page_num);
      g_free(file_path);
    }
    return TRUE;
  }

  /* Arrow keys to navigate list */
  if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_Up) {
    GtkWidget* treeview = g_object_get_data(G_OBJECT(zathura->ui.file_picker), "treeview");
    gtk_widget_grab_focus(treeview);
    /* Forward the keypress */
    GdkEvent* new_event = gdk_event_copy((GdkEvent*)event);
    gtk_widget_event(treeview, new_event);
    gdk_event_free(new_event);
    return TRUE;
  }

  return FALSE;
}

typedef enum zathura_link_action_e {
  ZATHURA_LINK_ACTION_FOLLOW,
  ZATHURA_LINK_ACTION_COPY,
  ZATHURA_LINK_ACTION_DISPLAY
} zathura_link_action_t;

static gboolean handle_link(GtkEntry* entry, girara_session_t* session, zathura_link_action_t action) {
  g_return_val_if_fail(session != NULL, FALSE);
  g_return_val_if_fail(session->global.data != NULL, FALSE);

  zathura_t* zathura = session->global.data;
  gboolean eval      = TRUE;

  g_autofree char* input = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
  if (input == NULL || strlen(input) == 0) {
    eval = FALSE;
  }

  int index = 0;
  if (eval == TRUE) {
    index = atoi(input);
    if (index == 0 && g_strcmp0(input, "0") != 0) {
      girara_notify(session, GIRARA_WARNING, _("Invalid input '%s' given."), input);
      eval = FALSE;
    }
    index = index - 1;
  }

  zathura_document_t* document = zathura_get_document(zathura);
  /* find the link*/
  zathura_link_t* link         = NULL;
  unsigned int number_of_pages = zathura_document_get_number_of_pages(document);
  for (unsigned int page_id = 0; page_id < number_of_pages; page_id++) {
    zathura_page_t* page = zathura_document_get_page(document, page_id);
    if (page == NULL || zathura_page_get_visibility(page) == false || eval == false) {
      continue;
    }
    GtkWidget* page_widget = zathura_page_get_widget(zathura, page);
    link                   = zathura_page_widget_link_get(ZATHURA_PAGE(page_widget), index);
    if (link != NULL) {
      break;
    }
  }

  if (eval == TRUE && link == NULL) {
    girara_notify(session, GIRARA_WARNING, _("Invalid index '%s' given."), input);
  } else {
    switch (action) {
    case ZATHURA_LINK_ACTION_FOLLOW:
      zathura_link_evaluate(zathura, link);
      break;
    case ZATHURA_LINK_ACTION_DISPLAY:
      zathura_link_display(zathura, link);
      break;
    case ZATHURA_LINK_ACTION_COPY: {
      g_autofree GdkAtom* selection = get_selection(zathura);
      if (selection == NULL) {
        break;
      }

      zathura_link_copy(zathura, link, selection);
      break;
    }
    }
  }

  return eval;
}

gboolean cb_sc_follow(GtkEntry* entry, void* data) {
  girara_session_t* session = data;
  return handle_link(entry, session, ZATHURA_LINK_ACTION_FOLLOW);
}

gboolean cb_sc_display_link(GtkEntry* entry, void* data) {
  girara_session_t* session = data;
  return handle_link(entry, session, ZATHURA_LINK_ACTION_DISPLAY);
}

gboolean cb_sc_copy_link(GtkEntry* entry, void* data) {
  girara_session_t* session = data;
  return handle_link(entry, session, ZATHURA_LINK_ACTION_COPY);
}

static gboolean file_monitor_reload(void* data) {
  sc_reload((girara_session_t*)data, NULL, NULL, 0);
  return FALSE;
}

void cb_file_monitor(ZathuraFileMonitor* monitor, girara_session_t* session) {
  g_return_if_fail(monitor != NULL);
  g_return_if_fail(session != NULL);

  g_main_context_invoke(NULL, file_monitor_reload, session);
}

static gboolean password_dialog(gpointer data) {
  zathura_password_dialog_info_t* dialog = data;

  if (dialog != NULL) {
    girara_dialog(dialog->zathura->ui.session, "Incorrect password. Enter password:", true, NULL, cb_password_dialog,
                  dialog);
  }

  return FALSE;
}

gboolean cb_password_dialog(GtkEntry* entry, void* data) {
  if (entry == NULL || data == NULL) {
    goto error_ret;
  }

  zathura_password_dialog_info_t* dialog = data;
  if (dialog->path == NULL || dialog->zathura == NULL) {
    goto error_free;
  }

  char* input = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

  /* no or empty password: ask again */
  if (input == NULL || strlen(input) == 0) {
    g_free(input);
    gdk_threads_add_idle(password_dialog, dialog);
    return false;
  }

  /* try to open document again */
  if (document_open(dialog->zathura, dialog->path, dialog->uri, input, ZATHURA_PAGE_NUMBER_UNSPECIFIED, NULL) ==
      false) {
    gdk_threads_add_idle(password_dialog, dialog);
  } else {
    g_free(dialog->path);
    g_free(dialog->uri);
    g_free(dialog);
  }
  g_free(input);

  return true;

error_free:
  g_free(dialog->path);
  g_free(dialog);

error_ret:
  return false;
}

gboolean cb_view_resized(GtkWidget* UNUSED(widget), GtkAllocation* UNUSED(allocation), zathura_t* zathura) {
  if (zathura_has_document(zathura) == false) {
    return false;
  }

  /* adjust the scale according to settings. If nothing needs to be resized,
     it does not trigger the resize event.

     The right viewport size is already in the document object, due to a
     previous call to adjustment_changed. We don't want to use the allocation in
     here, because we would have to subtract scrollbars, etc. */
  adjust_view(zathura);

  return false;
}

void cb_setting_recolor_change(girara_session_t* session, const char* name, girara_setting_type_t UNUSED(type),
                               const void* value, void* UNUSED(data)) {
  g_return_if_fail(value != NULL);
  g_return_if_fail(session != NULL);
  g_return_if_fail(session->global.data != NULL);
  g_return_if_fail(name != NULL);
  zathura_t* zathura = session->global.data;

  const bool bool_value = *((const bool*)value);

  if (zathura->sync.render_thread != NULL &&
      zathura_renderer_recolor_enabled(zathura->sync.render_thread) != bool_value) {
    zathura_renderer_enable_recolor(zathura->sync.render_thread, bool_value);
    render_all(zathura);
  }
}

void cb_setting_recolor_keep_hue_change(girara_session_t* session, const char* name, girara_setting_type_t UNUSED(type),
                                        const void* value, void* UNUSED(data)) {
  g_return_if_fail(value != NULL);
  g_return_if_fail(session != NULL);
  g_return_if_fail(session->global.data != NULL);
  g_return_if_fail(name != NULL);
  zathura_t* zathura = session->global.data;

  const bool bool_value = *((const bool*)value);

  if (zathura->sync.render_thread != NULL &&
      zathura_renderer_recolor_hue_enabled(zathura->sync.render_thread) != bool_value) {
    zathura_renderer_enable_recolor_hue(zathura->sync.render_thread, bool_value);
    render_all(zathura);
  }
}

void cb_setting_recolor_keep_reverse_video_change(girara_session_t* session, const char* name,
                                                  girara_setting_type_t UNUSED(type), const void* value,
                                                  void* UNUSED(data)) {
  g_return_if_fail(value != NULL);
  g_return_if_fail(session != NULL);
  g_return_if_fail(session->global.data != NULL);
  g_return_if_fail(name != NULL);
  zathura_t* zathura = session->global.data;

  const bool bool_value = *((const bool*)value);

  if (zathura->sync.render_thread != NULL &&
      zathura_renderer_recolor_reverse_video_enabled(zathura->sync.render_thread) != bool_value) {
    zathura_renderer_enable_recolor_reverse_video(zathura->sync.render_thread, bool_value);
    render_all(zathura);
  }
}

bool cb_unknown_command(girara_session_t* session, const char* input) {
  g_return_val_if_fail(session != NULL, false);
  g_return_val_if_fail(session->global.data != NULL, false);
  g_return_val_if_fail(input != NULL, false);

  zathura_t* zathura = session->global.data;
  if (zathura_has_document(zathura) == false) {
    return false;
  }

  /* check for number */
  const size_t size = strlen(input);
  for (size_t i = 0; i < size; i++) {
    if (g_ascii_isdigit(input[i]) == FALSE) {
      return false;
    }
  }

  zathura_jumplist_add(zathura);
  page_set(zathura, atoi(input) - 1);
  zathura_jumplist_add(zathura);

  return true;
}

void cb_page_widget_text_selected(ZathuraPage* page, const char* text, void* data) {
  g_return_if_fail(page != NULL);
  g_return_if_fail(text != NULL);
  g_return_if_fail(data != NULL);

  zathura_t* zathura = data;
  girara_mode_t mode = girara_mode_get(zathura->ui.session);
  if (mode != zathura->modes.normal && mode != zathura->modes.fullscreen) {
    return;
  }

  g_autofree GdkAtom* selection = get_selection(zathura);
  if (selection == NULL) {
    return;
  }

  /* copy to clipboard */
  gtk_clipboard_set_text(gtk_clipboard_get(*selection), text, -1);

  bool notification = true;
  girara_setting_get(zathura->ui.session, "selection-notification", &notification);

  if (notification == true) {
    g_autofree char* target = NULL;
    girara_setting_get(zathura->ui.session, "selection-clipboard", &target);

    g_autofree char* stripped_text = g_strdelimit(g_strdup(text), "\n\t\r\n", ' ');
    g_autofree char* escaped_text =
        g_markup_printf_escaped(_("Copied selected text to selection %s: %s"), target, stripped_text);

    girara_notify(zathura->ui.session, GIRARA_INFO, "%s", escaped_text);
  }
}

void cb_page_widget_image_selected(ZathuraPage* page, GdkPixbuf* pixbuf, void* data) {
  g_return_if_fail(page != NULL);
  g_return_if_fail(pixbuf != NULL);
  g_return_if_fail(data != NULL);

  zathura_t* zathura            = data;
  g_autofree GdkAtom* selection = get_selection(zathura);

  if (selection != NULL) {
    gtk_clipboard_set_image(gtk_clipboard_get(*selection), pixbuf);

    bool notification = true;
    girara_setting_get(zathura->ui.session, "selection-notification", &notification);

    if (notification == true) {
      g_autofree char* target = NULL;
      girara_setting_get(zathura->ui.session, "selection-clipboard", &target);

      g_autofree char* escaped_text = g_markup_printf_escaped(_("Copied selected image to selection %s"), target);

      girara_notify(zathura->ui.session, GIRARA_INFO, "%s", escaped_text);
    }
  }
}

void cb_page_widget_link(ZathuraPage* page, void* data) {
  g_return_if_fail(page != NULL);

  bool enter = (bool)data;

  GdkWindow* window   = gtk_widget_get_parent_window(GTK_WIDGET(page));
  GdkDisplay* display = gtk_widget_get_display(GTK_WIDGET(page));
  GdkCursor* cursor   = gdk_cursor_new_for_display(display, enter == true ? GDK_HAND1 : GDK_LEFT_PTR);
  gdk_window_set_cursor(window, cursor);
  g_object_unref(cursor);
}

void cb_page_widget_scaled_button_release(ZathuraPage* page_widget, GdkEventButton* event, void* data) {
  zathura_t* zathura   = data;
  zathura_page_t* page = zathura_page_widget_get_page(page_widget);

  if (event->button != GDK_BUTTON_PRIMARY) {
    return;
  }

  /* set page number (but don't scroll there. it was clicked on, so it's visible) */
  zathura_document_set_current_page_number(zathura_get_document(zathura), zathura_page_get_index(page));
  refresh_view(zathura);

  if (event->state & zathura->global.synctex_edit_modmask) {
    bool synctex = false;
    girara_setting_get(zathura->ui.session, "synctex", &synctex);
    if (synctex == false) {
      return;
    }

    if (zathura->dbus != NULL) {
      zathura_dbus_edit(zathura, zathura_page_get_index(page), event->x, event->y);
    }

    g_autofree char* editor = NULL;
    girara_setting_get(zathura->ui.session, "synctex-editor-command", &editor);
    if (editor == NULL || *editor == '\0') {
      girara_debug("No SyncTeX editor specified.");
      return;
    }

    synctex_edit(zathura, editor, page, event->x, event->y);
  }
}

void cb_window_update_icon(ZathuraRenderRequest* GIRARA_UNUSED(request), cairo_surface_t* surface, void* data) {
  zathura_t* zathura = data;

  girara_debug("updating window icon");
  GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, cairo_image_surface_get_width(surface),
                                                  cairo_image_surface_get_height(surface));
  if (pixbuf == NULL) {
    girara_error("Unable to convert cairo surface to Gdk Pixbuf.");
    return;
  }

  gtk_window_set_icon(GTK_WINDOW(zathura->ui.session->gtk.window), pixbuf);
  g_object_unref(pixbuf);
}

void cb_gesture_zoom_begin(GtkGesture* UNUSED(self), GdkEventSequence* UNUSED(sequence), void* data) {
  zathura_t* zathura = data;
  if (zathura_has_document(zathura) == false) {
    return;
  }
  zathura->gesture.initial_zoom = zathura_document_get_zoom(zathura_get_document(zathura));
}

void cb_gesture_zoom_scale_changed(GtkGestureZoom* UNUSED(self), gdouble scale, void* data) {
  zathura_t* zathura = data;
  if (zathura_has_document(zathura) == false) {
    return;
  }

  const double next_zoom     = zathura->gesture.initial_zoom * scale;
  girara_argument_t argument = {.n = ZOOM_SPECIFIC};

  sc_zoom(zathura->ui.session, &argument, NULL, next_zoom * 100);
}

void cb_hide_links(GtkWidget* widget, gpointer data) {
  g_return_if_fail(widget != NULL);
  g_return_if_fail(data != NULL);

  /* disconnect from signal */
  gulong handler_id = GPOINTER_TO_UINT(g_object_steal_data(G_OBJECT(widget), "handler_id"));
  g_signal_handler_disconnect(G_OBJECT(widget), handler_id);

  zathura_t* zathura           = data;
  zathura_document_t* document = zathura_get_document(zathura);
  unsigned int number_of_pages = zathura_document_get_number_of_pages(document);
  for (unsigned int page_id = 0; page_id < number_of_pages; page_id++) {
    zathura_page_t* page = zathura_document_get_page(document, page_id);
    if (page == NULL) {
      continue;
    }

    GtkWidget* page_widget = zathura_page_get_widget(zathura, page);
    g_object_set(G_OBJECT(page_widget), "draw-links", FALSE, NULL);
  }
}
