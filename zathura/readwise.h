/* SPDX-License-Identifier: Zlib */

#ifndef READWISE_H
#define READWISE_H

#include <girara/types.h>

/**
 * Result codes for Readwise API operations
 */
typedef enum readwise_result_e {
  READWISE_OK,                  /** Success */
  READWISE_ERROR_NO_TOKEN,      /** READWISE_TOKEN environment variable not set */
  READWISE_ERROR_NETWORK,       /** Network or connection error */
  READWISE_ERROR_AUTH,          /** Authentication failed (401) */
  READWISE_ERROR_SERVER,        /** Server error (4xx/5xx) */
  READWISE_ERROR_JSON           /** JSON encoding error */
} readwise_result_t;

/**
 * Sync highlights to Readwise API
 *
 * @param highlights List of zathura_highlight_t* to sync
 * @param title Document title
 * @param author Document author (can be NULL)
 * @param token Readwise API token (if NULL, reads from READWISE_TOKEN env var)
 * @return Result code indicating success or failure type
 */
readwise_result_t readwise_sync_highlights(girara_list_t* highlights,
                                           const char* title,
                                           const char* author,
                                           const char* token);

/**
 * Get human-readable message for result code
 *
 * @param result Result code
 * @return Error message string
 */
const char* readwise_result_message(readwise_result_t result);

#endif // READWISE_H
