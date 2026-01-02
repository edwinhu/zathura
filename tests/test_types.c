/* SPDX-License-Identifier: Zlib */

#include <limits.h>

#include "types.h"

static void test_image_buffer_fail(void) {
  g_assert_null(zathura_image_buffer_create(UINT_MAX, UINT_MAX));
}

static void test_image_buffer(void) {
  zathura_image_buffer_t* buffer = zathura_image_buffer_create(1, 1);
  g_assert_nonnull(buffer);
  zathura_image_buffer_free(buffer);
}

static void test_note_new(void) {
  zathura_note_t* note = zathura_note_new(5, 100.0, 200.0, "Test note content");
  g_assert_nonnull(note);
  g_assert_nonnull(note->id);
  g_assert_cmpuint(note->page, ==, 5);
  g_assert_cmpfloat(note->x, ==, 100.0);
  g_assert_cmpfloat(note->y, ==, 200.0);
  g_assert_nonnull(note->content);
  g_assert_cmpstr(note->content, ==, "Test note content");
  g_assert_cmpint(note->created_at, >, 0);
  zathura_note_free(note);
}

static void test_note_new_null_content(void) {
  zathura_note_t* note = zathura_note_new(0, 0.0, 0.0, NULL);
  g_assert_nonnull(note);
  g_assert_nonnull(note->id);
  g_assert_null(note->content);
  zathura_note_free(note);
}

static void test_note_free_null(void) {
  /* Should not crash when passed NULL */
  zathura_note_free(NULL);
}

int main(int argc, char* argv[]) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/types/image_buffer_fail", test_image_buffer_fail);
  g_test_add_func("/types/image_buffer", test_image_buffer);
  g_test_add_func("/types/note_new", test_note_new);
  g_test_add_func("/types/note_new_null_content", test_note_new_null_content);
  g_test_add_func("/types/note_free_null", test_note_free_null);
  return g_test_run();
}
