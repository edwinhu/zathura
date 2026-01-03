/* SPDX-License-Identifier: Zlib */

#ifndef PAGE_H
#define PAGE_H

#include <girara/datastructures.h>
#include <cairo.h>

#include "types.h"

/**
 * Get the page object
 *
 * @param document The document
 * @param index Page number
 * @param error Optional error
 * @return Page object or NULL if an error occurred
 */
ZATHURA_PLUGIN_API zathura_page_t* zathura_page_new(zathura_document_t* document, unsigned int index,
                                                    zathura_error_t* error);

/**
 * Frees the page object
 *
 * @param page The page object
 * @return ZATHURA_ERROR_OK when no error occurred, otherwise see
 *    zathura_error_t
 */
ZATHURA_PLUGIN_API zathura_error_t zathura_page_free(zathura_page_t* page);

/**
 * Returns the associated document
 *
 * @param page The page object
 * @return The associated document
 * @return NULL if an error occurred
 */
ZATHURA_PLUGIN_API zathura_document_t* zathura_page_get_document(zathura_page_t* page);

/**
 * Returns the set id of the page
 *
 * @param page The page object
 * @return The id of the page
 */
ZATHURA_PLUGIN_API unsigned int zathura_page_get_index(zathura_page_t* page);

/**
 * Returns the width of the page
 *
 * @param page The page object
 * @return Width of the page
 * @return -1 If an error occurred
 */
ZATHURA_PLUGIN_API double zathura_page_get_width(zathura_page_t* page);

/**
 * Sets the new width of the page
 *
 * @param page The page object
 * @param width The new width of the page
 */
ZATHURA_PLUGIN_API void zathura_page_set_width(zathura_page_t* page, double width);

/**
 * Returns the height of the page
 *
 * @param page The page object
 * @return Height of the page
 * @return -1 If an error occurred
 */
ZATHURA_PLUGIN_API double zathura_page_get_height(zathura_page_t* page);

/**
 * Sets the new height of the page
 *
 * @param page The page object
 * @param height The new height of the page
 */
ZATHURA_PLUGIN_API void zathura_page_set_height(zathura_page_t* page, double height);

/**
 * Returns the visibility of the page
 *
 * @param page The page object
 * @return true if the page is visible
 * @return false if the page is hidden
 */
ZATHURA_PLUGIN_API bool zathura_page_get_visibility(zathura_page_t* page);

/**
 * Sets the visibility of the page
 *
 * @param page The page object
 * @param visibility The new visibility value
 */
ZATHURA_PLUGIN_API void zathura_page_set_visibility(zathura_page_t* page, bool visibility);

/**
 * Returns the custom data
 *
 * @param page The page object
 * @return The custom data or NULL
 */
ZATHURA_PLUGIN_API void* zathura_page_get_data(zathura_page_t* page);

/**
 * Sets the custom data
 *
 * @param page The page object
 * @param data The custom data
 */
ZATHURA_PLUGIN_API void zathura_page_set_data(zathura_page_t* page, void* data);

/**
 * Search page
 *
 * @param page The page object
 * @param text Search item
 * @param error Set to an error value (see \ref zathura_error_t) if an
 *   error occurred
 * @return List of results
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_search_text(zathura_page_t* page, const char* text,
                                                           zathura_error_t* error);

/**
 * Get page links
 *
 * @param page The page object
 * @param error Set to an error value (see \ref zathura_error_t) if an
 *   error occurred
 * @return List of links
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_links_get(zathura_page_t* page, zathura_error_t* error);

/**
 * Free page links
 *
 * @param list List of links
 * @return ZATHURA_ERROR_OK when no error occurred, otherwise see
 *    zathura_error_t
 */
ZATHURA_PLUGIN_API zathura_error_t zathura_page_links_free(girara_list_t* list);

/**
 * Get list of form fields
 *
 * @param page The page object
 * @param error Set to an error value (see \ref zathura_error_t) if an
 *   error occurred
 * @return List of form fields
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_form_fields_get(zathura_page_t* page, zathura_error_t* error);

/**
 * Free list of form fields
 *
 * @param list List of form fields
 * @return ZATHURA_ERROR_OK when no error occurred, otherwise see
 *    zathura_error_t
 */
ZATHURA_PLUGIN_API zathura_error_t zathura_page_form_fields_free(girara_list_t* list);

/**
 * Get list of images
 *
 * @param page Page
 * @param error Set to an error value (see \ref zathura_error_t) if an
 *   error occurred
 * @return List of images or NULL if an error occurred
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_images_get(zathura_page_t* page, zathura_error_t* error);

/**
 * Get image
 *
 * @param page Page
 * @param image Image identifier
 * @param error Set to an error value (see \ref zathura_error_t) if an
 *   error occurred
 * @return The cairo image surface or NULL if an error occurred
 */
ZATHURA_PLUGIN_API cairo_surface_t* zathura_page_image_get_cairo(zathura_page_t* page, zathura_image_t* image,
                                                                 zathura_error_t* error);

/**
 * Get text for selection
 * @param page Page
 * @param rectangle Selection
 * @param error Set to an error value (see \ref zathura_error_t) if an error
 * occurred
 * @return The selected text (needs to be deallocated with g_free)
 */
ZATHURA_PLUGIN_API char* zathura_page_get_text(zathura_page_t* page, zathura_rectangle_t rectangle,
                                               zathura_error_t* error);

/**
 * Get rectangles from selection
 * @param page Page
 * @param rectangle Selection
 * @param error Set to an error value (see \ref zathura_error_t) if an error
 * occurred
 * @return List of rectangles or NULL if an error occurred
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_get_selection(zathura_page_t* page, zathura_rectangle_t rectangle,
                                                             zathura_error_t* error);

/**
 * Render page
 *
 * @param page The page object
 * @param cairo Cairo object
 * @param printing render for printing
 * @return ZATHURA_ERROR_OK when no error occurred, otherwise see
 *    zathura_error_t
 */
ZATHURA_PLUGIN_API zathura_error_t zathura_page_render(zathura_page_t* page, cairo_t* cairo, bool printing);

/**
 * Get page label. Note that the page label might not exist, in this case NULL
 * is returned.
 *
 * @param page Page
 * @param error Set to an error value (see \ref zathura_error_t) if an error
 *    occurred.
 * @return Page label
 */
ZATHURA_PLUGIN_API const char* zathura_page_get_label(zathura_page_t* page, zathura_error_t* error);

/**
 * Get whether the page label equals the page number
 *
 * @param page Page
 * @return Boolean indicating whether the page label equals the page number
 */
ZATHURA_PLUGIN_API bool zathura_page_label_is_number(zathura_page_t* page);

/**
 * Get signatures of a page
 *
 * @param page Page
 * @param error Set to an error value (see \ref zathura_error_t) if an error
 *    occurred.
 * @return List of signatures
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_get_signatures(zathura_page_t* page, zathura_error_t* error);

/**
 * Get text markup annotations from a page
 *
 * @param page Page
 * @param error Set to an error value (see \ref zathura_error_t) if an error
 *    occurred.
 * @return List of annotations
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_get_annotations(zathura_page_t* page, zathura_error_t* error);

/**
 * Export highlights to page as PDF annotations
 *
 * @param page The page
 * @param highlights List of zathura_highlight_t* to export
 * @return ZATHURA_ERROR_OK or error code
 */
zathura_error_t zathura_page_export_annotations(zathura_page_t* page, girara_list_t* highlights);

/**
 * Delete an embedded annotation matching the given geometry
 *
 * @param page The page
 * @param rects List of zathura_rectangle_t* defining the annotation geometry
 * @return ZATHURA_ERROR_OK or error code
 */
zathura_error_t zathura_page_delete_annotation(zathura_page_t* page, girara_list_t* rects);

/**
 * Get embedded sticky note annotations from a page
 *
 * @param page Page
 * @param error Set to an error value (see \ref zathura_error_t) if an error
 *    occurred.
 * @return List of zathura_note_t* or NULL if none
 */
ZATHURA_PLUGIN_API girara_list_t* zathura_page_get_notes(zathura_page_t* page, zathura_error_t* error);

/**
 * Delete an embedded note annotation by position
 *
 * @param page The page
 * @param x X coordinate of the note
 * @param y Y coordinate of the note
 * @return ZATHURA_ERROR_OK or error code
 */
zathura_error_t zathura_page_delete_note(zathura_page_t* page, double x, double y);

/**
 * Update the content of an embedded note annotation by position
 *
 * @param page The page
 * @param x X coordinate of the note
 * @param y Y coordinate of the note
 * @param content The new content for the note
 * @return ZATHURA_ERROR_OK or error code
 */
zathura_error_t zathura_page_update_note_content(zathura_page_t* page, double x, double y, const char* content);

/**
 * Export notes to page as PDF text annotations
 *
 * @param page The page
 * @param notes List of zathura_note_t* to export
 * @return ZATHURA_ERROR_OK or error code
 */
zathura_error_t zathura_page_export_notes(zathura_page_t* page, girara_list_t* notes);

#endif // PAGE_H
