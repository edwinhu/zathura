/* SPDX-License-Identifier: Zlib */

#ifndef PAGE_WIDGET_H
#define PAGE_WIDGET_H

#include <gtk/gtk.h>
#include "types.h"
#include "document.h"

/**
 * The page view widget. The widget handles all the rendering on its own. It
 * only has to be resized. The widget also manages and handles all the
 * rectangles for highlighting.
 *
 * Before the properties contain the correct values, 'draw-links' has to be set
 * to TRUE at least one time.
 * */
struct zathura_page_widget_s {
  GtkDrawingArea parent;
};

struct zathura_page_widget_class_s {
  GtkDrawingAreaClass parent_class;
};

#define ZATHURA_TYPE_PAGE (zathura_page_widget_get_type())
#define ZATHURA_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), ZATHURA_TYPE_PAGE, ZathuraPage))
#define ZATHURA_PAGE_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), ZATHURA_TYPE_PAGE, ZathuraPageClass))
#define ZATHURA_IS_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZATHURA_TYPE_PAGE))
#define ZATHURA_IS_PAGE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj), ZATHURA_TYPE_PAGE))
#define ZATHURA_PAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), ZATHURA_TYPE_PAGE, ZathuraPageClass))

/**
 * Returns the type of the page view widget.
 * @return the type
 */
GType zathura_page_widget_get_type(void) G_GNUC_CONST;
/**
 * Create a page view widget.
 * @param zathura the zathura instance
 * @param page the page to be displayed
 * @return a page view widget
 */
GtkWidget* zathura_page_widget_new(zathura_t* zathura, zathura_page_t* page);
/**
 * Update the widget's surface. This should only be called from the render
 * thread.
 * @param widget the widget
 * @param surface the new surface
 * @param keep_thumbnail don't destroy when surface is NULL
 */
void zathura_page_widget_update_surface(ZathuraPage* widget, cairo_surface_t* surface, bool keep_thumbnail);
/**
 * Clear highlight of the selection/highlighter.
 * @param widget the widget
 */
void zathura_page_widget_clear_selection(ZathuraPage* widget);
/**
 * Draw a rectangle to mark links or search results
 * @param widget the widget
 * @param rectangle the rectangle
 * @param linkid the link id if it's a link, -1 otherwise
 */
zathura_link_t* zathura_page_widget_link_get(ZathuraPage* widget, unsigned int index);
/**
 * Update the last view time of the page.
 *
 * @param widget the widget
 */
void zathura_page_widget_update_view_time(ZathuraPage* widget);
/**
 * Check if we have a surface.
 *
 * @param widget the widget
 * @returns true if the widget has a surface, false otherwise
 */
bool zathura_page_widget_have_surface(ZathuraPage* widget);
/**
 * Abort outstanding render requests
 *
 * @param widget the widget
 */
void zathura_page_widget_abort_render_request(ZathuraPage* widget);
/**
 * Get underlying page
 *
 * @param widget the widget
 * @return underlying zathura_page_t instance
 */
zathura_page_t* zathura_page_widget_get_page(ZathuraPage* widget);

/**
 * Get selection rectangles
 *
 * @param widget the widget
 * @return list of selection rectangles or NULL
 */
girara_list_t* zathura_page_widget_get_selection(ZathuraPage* widget);

/**
 * Get selection bounds (mouse selection rectangle)
 *
 * @param widget the widget
 * @param rect output rectangle
 * @return true if there's a valid selection
 */
bool zathura_page_widget_get_selection_bounds(ZathuraPage* widget, zathura_rectangle_t* rect);

/**
 * Set persistent highlights for this page
 *
 * @param widget the widget
 * @param highlights list of zathura_highlight_t* (ownership NOT transferred)
 */
void zathura_page_widget_set_highlights(ZathuraPage* widget, girara_list_t* highlights);

/**
 * Add a single highlight to this page (takes ownership)
 *
 * @param widget the widget
 * @param highlight the highlight to add (ownership transferred)
 */
void zathura_page_widget_add_highlight(ZathuraPage* widget, zathura_highlight_t* highlight);

/**
 * Remove a highlight from this page by ID
 *
 * @param widget the widget
 * @param highlight_id the unique identifier of the highlight to remove
 * @return true if highlight was found and removed, false otherwise
 */
bool zathura_page_widget_remove_highlight(ZathuraPage* widget, const char* highlight_id);

/**
 * Get highlights for this page
 *
 * @param widget the widget
 * @return list of zathura_highlight_t* or NULL if no highlights
 */
girara_list_t* zathura_page_widget_get_highlights(ZathuraPage* widget);

/**
 * Get last click position (persists after button release)
 *
 * @param widget the widget
 * @param x output x position
 * @param y output y position
 * @return true if there's a valid last click position
 */
bool zathura_page_widget_get_last_click(ZathuraPage* widget, double* x, double* y);

/**
 * Get the ID of the currently selected highlight
 *
 * @param widget the widget
 * @return highlight ID or NULL if none selected
 */
const char* zathura_page_widget_get_selected_highlight_id(ZathuraPage* widget);

/**
 * Clear the selected highlight
 *
 * @param widget the widget
 */
void zathura_page_widget_clear_selected_highlight(ZathuraPage* widget);

/**
 * Get the rectangles of the currently selected embedded annotation
 *
 * @param widget the widget
 * @return list of rectangles or NULL if no embedded annotation selected
 */
girara_list_t* zathura_page_widget_get_embedded_selected_rects(ZathuraPage* widget);

/**
 * Note icon size in pixels
 */
#define NOTE_ICON_SIZE 16.0

/**
 * Set notes for this page (takes ownership)
 *
 * @param widget the widget
 * @param notes list of zathura_note_t* (ownership transferred)
 */
void zathura_page_widget_set_notes(ZathuraPage* widget, girara_list_t* notes);

/**
 * Add a single note to this page (takes ownership)
 *
 * @param widget the widget
 * @param note the note to add (ownership transferred)
 */
void zathura_page_widget_add_note(ZathuraPage* widget, zathura_note_t* note);

/**
 * Remove a note from this page by ID
 *
 * @param widget the widget
 * @param note_id the unique identifier of the note to remove
 * @return true if note was found and removed, false otherwise
 */
bool zathura_page_widget_remove_note(ZathuraPage* widget, const char* note_id);

/**
 * Get notes for this page
 *
 * @param widget the widget
 * @return list of zathura_note_t* or NULL if no notes
 */
girara_list_t* zathura_page_widget_get_notes(ZathuraPage* widget);

#endif
