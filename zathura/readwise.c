/* SPDX-License-Identifier: Zlib */

#include "readwise.h"
#include "types.h"

#include <curl/curl.h>
#include <json-glib/json-glib.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define READWISE_API_URL "https://readwise.io/api/v2/highlights/"

/**
 * Callback to discard response body (we don't need it)
 */
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  (void)contents;
  (void)userp;
  return size * nmemb;
}

/**
 * Build JSON payload for Readwise API
 */
static char* build_json_payload(girara_list_t* highlights, const char* title, const char* author) {
  JsonBuilder* builder = json_builder_new();

  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "highlights");
  json_builder_begin_array(builder);

  for (size_t i = 0; i < girara_list_size(highlights); i++) {
    zathura_highlight_t* hl = girara_list_nth(highlights, i);
    if (hl == NULL || hl->text == NULL) {
      continue;
    }

    json_builder_begin_object(builder);

    // text (required)
    json_builder_set_member_name(builder, "text");
    json_builder_add_string_value(builder, hl->text);

    // title (required)
    json_builder_set_member_name(builder, "title");
    json_builder_add_string_value(builder, title);

    // author (optional)
    if (author != NULL) {
      json_builder_set_member_name(builder, "author");
      json_builder_add_string_value(builder, author);
    }

    // source_type (required)
    json_builder_set_member_name(builder, "source_type");
    json_builder_add_string_value(builder, "pdf");

    // category (required)
    json_builder_set_member_name(builder, "category");
    json_builder_add_string_value(builder, "books");

    // location_type
    json_builder_set_member_name(builder, "location_type");
    json_builder_add_string_value(builder, "page");

    // location (1-indexed for Readwise)
    json_builder_set_member_name(builder, "location");
    json_builder_add_int_value(builder, hl->page + 1);

    // highlighted_at (ISO 8601 timestamp)
    GDateTime* dt = g_date_time_new_from_unix_utc(hl->created_at);
    if (dt != NULL) {
      char* timestamp = g_date_time_format_iso8601(dt);
      json_builder_set_member_name(builder, "highlighted_at");
      json_builder_add_string_value(builder, timestamp);
      g_free(timestamp);
      g_date_time_unref(dt);
    }

    json_builder_end_object(builder);
  }

  json_builder_end_array(builder);
  json_builder_end_object(builder);

  JsonGenerator* gen = json_generator_new();
  JsonNode* root = json_builder_get_root(builder);
  json_generator_set_root(gen, root);
  char* json_str = json_generator_to_data(gen, NULL);

  json_node_free(root);
  g_object_unref(gen);
  g_object_unref(builder);

  return json_str;
}

/**
 * Sync highlights to Readwise API
 */
readwise_result_t readwise_sync_highlights(girara_list_t* highlights,
                                           const char* title,
                                           const char* author) {
  if (highlights == NULL || title == NULL) {
    return READWISE_ERROR_JSON;
  }

  // Check for READWISE_TOKEN environment variable
  const char* token = g_getenv("READWISE_TOKEN");
  if (token == NULL || strlen(token) == 0) {
    return READWISE_ERROR_NO_TOKEN;
  }

  // Build JSON payload
  char* json_payload = build_json_payload(highlights, title, author);
  if (json_payload == NULL) {
    return READWISE_ERROR_JSON;
  }

  // Initialize curl
  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    g_free(json_payload);
    return READWISE_ERROR_NETWORK;
  }

  // Set up headers
  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  char auth_header[512];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", token);
  headers = curl_slist_append(headers, auth_header);

  // Configure curl
  curl_easy_setopt(curl, CURLOPT_URL, READWISE_API_URL);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  // Perform request
  CURLcode res = curl_easy_perform(curl);

  readwise_result_t result = READWISE_OK;

  if (res != CURLE_OK) {
    result = READWISE_ERROR_NETWORK;
  } else {
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 401) {
      result = READWISE_ERROR_AUTH;
    } else if (http_code >= 400) {
      result = READWISE_ERROR_SERVER;
    }
  }

  // Cleanup
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  g_free(json_payload);

  return result;
}

/**
 * Get human-readable message for result code
 */
const char* readwise_result_message(readwise_result_t result) {
  switch (result) {
    case READWISE_OK:
      return "Success";
    case READWISE_ERROR_NO_TOKEN:
      return "READWISE_TOKEN not set. Get token from readwise.io/access_token";
    case READWISE_ERROR_NETWORK:
      return "Network error";
    case READWISE_ERROR_AUTH:
      return "Invalid token";
    case READWISE_ERROR_SERVER:
      return "Server error";
    case READWISE_ERROR_JSON:
      return "JSON encoding error";
    default:
      return "Unknown error";
  }
}
