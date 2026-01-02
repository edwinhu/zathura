# SPEC: Embedded PDF Annotation Import (Phase 1)

**Status: COMPLETE** (2026-01-02)

## Feature

`:hlimport` command imports embedded PDF highlight/underline/strikeout annotations via mupdf and **persists them to SQLite** as regular highlights.

## User Flow

```
1. User opens PDF with embedded highlights from another app
2. User runs :highlights import
3. Embedded annotations converted to zathura highlights
4. Saved to SQLite database (permanent)
5. Appear in :highlights panel like any other highlight
6. Can be deleted/modified normally
```

## Scope

| Item | In Scope | Out of Scope |
|------|----------|--------------|
| `:highlights import` command | Yes | Auto-import on open |
| Convert to SQLite highlights | Yes | Keep as "embedded" type |
| Full CRUD after import | Yes | Sync back to PDF |
| Filter by type (highlight/underline/strikeout) | Yes | Ink/text/other annotations |
| Color mapping | Yes | Full RGB support |

## Architecture

```
:highlights import → iterate pages
                   → plugin: page_get_annotations()
                   → mupdf: pdf_first_annot() / pdf_next_annot()
                   → filter Highlight/Underline/StrikeOut types
                   → convert QuadPoints → zathura_rectangle_t
                   → map RGB → nearest zathura_highlight_color_t
                   → save to SQLite via zathura_db_add_highlight()
                   → add to page widget
                   → notify user: "Imported N highlights"
```

## Files to Modify

### Zathura Core

| File | Changes |
|------|---------|
| `zathura/plugin-api.h` | Add `zathura_plugin_page_get_annotations_t` typedef + struct field |
| `zathura/page.h` | Declare `zathura_page_get_annotations()` |
| `zathura/page.c` | Implement `zathura_page_get_annotations()` dispatch wrapper |
| `zathura/commands.c` | Add `cmd_highlights_import()` function |
| `zathura/commands.h` | Declare `cmd_highlights_import()` |
| `zathura/config.c` | Register `:highlights import` command |

### MuPDF Plugin (clone into repo)

| File | Changes |
|------|---------|
| `zathura-pdf-mupdf/plugin.c` | Register `page_get_annotations` in function table |
| `zathura-pdf-mupdf/annotations.c` | **New file**: Implement `pdf_page_get_annotations()` |
| `zathura-pdf-mupdf/meson.build` | Add `annotations.c` to sources |

## Implementation Steps

### Step 1: Clone mupdf plugin

```bash
git clone https://github.com/pwmt/zathura-pdf-mupdf plugins/zathura-pdf-mupdf
```

### Step 2: Modify `zathura/plugin-api.h`

Add typedef after line 120:

```c
typedef girara_list_t* (*zathura_plugin_page_get_annotations_t)(
    zathura_page_t* page, void* data, zathura_error_t* error);
```

Add to struct after line 216 (after `page_get_signatures`):

```c
  zathura_plugin_page_get_annotations_t page_get_annotations;
```

### Step 3: Add `zathura/page.h` declaration

After `zathura_page_get_signatures`:

```c
girara_list_t* zathura_page_get_annotations(zathura_page_t* page, zathura_error_t* error);
```

### Step 4: Implement in `zathura/page.c`

Pattern: follow `zathura_page_get_signatures()` at lines 365-393.

```c
girara_list_t* zathura_page_get_annotations(zathura_page_t* page, zathura_error_t* error) {
  if (page == NULL || page->document == NULL) {
    if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    return NULL;
  }

  const zathura_plugin_t* plugin = zathura_document_get_plugin(page->document);
  const zathura_plugin_functions_t* functions = zathura_plugin_get_functions(plugin);

  if (functions->page_get_annotations == NULL) {
    if (error) *error = ZATHURA_ERROR_NOT_IMPLEMENTED;
    return NULL;
  }

  return functions->page_get_annotations(page, page->data, error);
}
```

### Step 5: Add `cmd_highlights_import()` in `zathura/commands.c`

```c
bool cmd_highlights_import(girara_session_t* session, girara_list_t* argument_list) {
  g_return_val_if_fail(session != NULL, false);
  zathura_t* zathura = session->global.data;

  if (zathura->document == NULL) {
    girara_notify(session, GIRARA_ERROR, "No document opened");
    return false;
  }

  unsigned int imported = 0;
  unsigned int num_pages = zathura_document_get_number_of_pages(zathura->document);
  const char* file_path = zathura_document_get_path(zathura->document);

  for (unsigned int page_id = 0; page_id < num_pages; page_id++) {
    zathura_page_t* page = zathura_document_get_page(zathura->document, page_id);
    girara_list_t* annotations = zathura_page_get_annotations(page, NULL);

    if (annotations != NULL) {
      for (size_t i = 0; i < girara_list_size(annotations); i++) {
        zathura_highlight_t* hl = girara_list_nth(annotations, i);
        if (hl != NULL) {
          // Save to database (permanent)
          zathura_db_add_highlight(zathura->database, file_path, hl);
          // Add to page widget for immediate display
          zathura_page_widget_add_highlight(ZATHURA_PAGE(zathura->pages[page_id]), hl);
          imported++;
        }
      }
      girara_list_free(annotations);
    }
  }

  girara_notify(session, GIRARA_INFO, "Imported %u highlights", imported);
  return true;
}
```

### Step 6: Register command in `zathura/config.c`

In `config_load_default()`, add after other highlight commands:

```c
girara_inputbar_command_add(gsession, "highlights import", NULL, cmd_highlights_import,
                            NULL, "Import embedded PDF annotations as highlights");
```

### Step 7: Create `plugins/zathura-pdf-mupdf/annotations.c`

```c
#include "plugin.h"
#include <mupdf/pdf.h>

static zathura_highlight_color_t map_color(int n, float c[4]) {
  if (n < 3) return ZATHURA_HIGHLIGHT_YELLOW;
  float r = c[0], g = c[1], b = c[2];
  if (r > 0.7f && g > 0.7f && b < 0.5f) return ZATHURA_HIGHLIGHT_YELLOW;
  if (g > 0.6f && g > r && g > b) return ZATHURA_HIGHLIGHT_GREEN;
  if (b > 0.5f && b > r) return ZATHURA_HIGHLIGHT_BLUE;
  if (r > 0.6f && r > g && r > b) return ZATHURA_HIGHLIGHT_RED;
  return ZATHURA_HIGHLIGHT_YELLOW;
}

girara_list_t* pdf_page_get_annotations(zathura_page_t* page, void* data,
                                         zathura_error_t* error) {
  mupdf_page_t* mupdf_page = data;
  // ... iterate pdf_first_annot/pdf_next_annot
  // ... filter PDF_ANNOT_HIGHLIGHT, PDF_ANNOT_UNDERLINE, PDF_ANNOT_STRIKE_OUT
  // ... convert quad_points to rectangles
  // ... map colors
  // ... return list of zathura_highlight_t*
}
```

### Step 8: Register in plugin

In `plugins/zathura-pdf-mupdf/plugin.c`, add to function table:

```c
.page_get_annotations = pdf_page_get_annotations,
```

## Key Technical Details

### Coordinate Transform

PDF: origin bottom-left, Y up. Zathura: origin top-left, Y down.

```c
zathura_y = page_height - pdf_y;
```

### QuadPoints to Rectangle

```c
fz_rect r = fz_rect_from_quad(quad);
rect.x1 = r.x0;
rect.y1 = page_height - r.y1;
rect.x2 = r.x1;
rect.y2 = page_height - r.y0;
```

### Memory Ownership

- `page_get_annotations()` returns `girara_list_t*` **owned by caller**
- Each `zathura_highlight_t*` in list **owned by list**
- Caller transfers highlights to page widget, then frees empty list

## Acceptance Criteria

- [x] `:hlimport` command exists (renamed from `:highlights import` - girara doesn't support spaces)
- [x] Command imports embedded PDF annotations to SQLite
- [x] Imported highlights appear on page immediately
- [x] Imported highlights show in `:highlights` panel
- [x] Imported highlights have correct color mapping (Y/G/B/R)
- [x] Imported highlights persist after document close/reopen
- [x] Can delete/modify imported highlights like any other
- [x] Shows count: "Imported N highlights"
- [x] No crash on PDFs without annotations (shows "Imported 0")
- [x] No duplicates on re-import (geometry-based deduplication)

## Test Plan

1. Open PDF with embedded highlights from another app (Preview, Acrobat, sioyek)
2. Run `:hlimport`
3. Verify notification shows import count
4. Verify highlights appear on pages with correct colors
5. Run `:highlights` - verify imported highlights listed
6. Delete an imported highlight - verify it's removed
7. Close and reopen document - verify highlights persisted
8. Run `:hlimport` again - verify no duplicates created
9. Test on PDF with no annotations - verify "Imported 0"
