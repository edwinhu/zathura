# Annotation Layer Roadmap

## Current State (2026-01)

Zathura has a **sioyek-style highlight system** that stores highlights in SQLite:

| Feature | Status |
|---------|--------|
| Create highlights (h/j/k/l keys) | Done |
| Persist to SQLite database | Done |
| Load on document open | Done |
| Visual selection (click to select) | Done |
| Delete selected highlight (x key) | Done |

**Limitation**: This system is completely separate from PDF annotations. Embedded PDF annotations (created by other apps) cannot be imported, and zathura highlights cannot be exported to PDF.

## Problem

PDFs with embedded annotations (e.g., highlights from sioyek, Acrobat, Preview) display correctly (mupdf renders them), but zathura cannot:
- List them
- Select them
- Delete them
- Modify them

Users expect to manage all visible highlights, regardless of origin.

## Proposed Architecture

### Phase 1: Import (Read-Only Sync)

On document open, read embedded PDF annotations via mupdf and convert to zathura highlights:

```
PDF opens → mupdf pdf_first_annot() → iterate annotations
         → filter Highlight/Underline/StrikeOut types
         → convert QuadPoints to zathura_rectangle_t
         → add to page widget (but NOT to SQLite)
         → mark as "embedded" (read-only)
```

**Requires:**
- New plugin API function: `zathura_page_get_annotations()`
- Implementation in zathura-pdf-mupdf
- Annotation type enum in zathura types

**User value:** Can see and list all highlights uniformly

### Phase 2: Delete Embedded Annotations

Allow deleting imported annotations:

```
User selects embedded annotation → press x
→ call plugin: zathura_page_delete_annotation(page, annot_id)
→ mupdf: pdf_delete_annot()
→ mark document as modified
→ prompt to save on close
```

**Requires:**
- Plugin API: `zathura_page_delete_annotation()`
- Document modified state tracking
- Save-on-close prompt
- zathura-pdf-mupdf implementation using `pdf_delete_annot()`

**User value:** Can delete any highlight regardless of origin

### Phase 3: Export Highlights to PDF

Convert zathura SQLite highlights to embedded PDF annotations:

```
:export-annotations or on save
→ iterate SQLite highlights for document
→ convert to PDF annotation format
→ call plugin: zathura_page_add_annotation()
→ mupdf: pdf_create_annot() + pdf_set_annot_quad_points()
→ save PDF
```

**Requires:**
- Plugin API: `zathura_page_add_annotation()`
- Export command
- Save modified PDF functionality

**User value:** Highlights visible in other PDF readers

### Phase 4: Full Sync

Bidirectional sync between SQLite and PDF:
- Import embedded → SQLite (with origin tracking)
- Export SQLite → embedded
- Conflict resolution strategy
- Option to prefer embedded vs SQLite as source of truth

## Plugin API Additions

```c
// types.h
typedef enum {
  ZATHURA_ANNOTATION_HIGHLIGHT,
  ZATHURA_ANNOTATION_UNDERLINE,
  ZATHURA_ANNOTATION_STRIKEOUT,
  ZATHURA_ANNOTATION_SQUIGGLY,
  // future: ZATHURA_ANNOTATION_TEXT, ZATHURA_ANNOTATION_INK, etc.
} zathura_annotation_type_t;

typedef struct {
  char* id;                        // unique identifier (e.g., /NM field)
  zathura_annotation_type_t type;
  girara_list_t* quad_points;      // list of zathura_quad_t
  float color[3];                  // RGB
  float opacity;
  char* contents;                  // optional text content
  bool embedded;                   // true if from PDF, false if from SQLite
} zathura_annotation_t;

// plugin-api.h
typedef girara_list_t* (*zathura_plugin_page_get_annotations_t)(
    zathura_page_t* page, void* data, zathura_error_t* error);

typedef zathura_error_t (*zathura_plugin_page_delete_annotation_t)(
    zathura_page_t* page, void* data, const char* annotation_id);

typedef zathura_error_t (*zathura_plugin_page_add_annotation_t)(
    zathura_page_t* page, void* data, zathura_annotation_t* annotation);

typedef zathura_error_t (*zathura_plugin_document_save_t)(
    zathura_document_t* document, void* data, const char* path);
```

## MuPDF Implementation Notes

Key mupdf functions:
- `pdf_first_annot()` / `pdf_next_annot()` - iterate annotations
- `pdf_annot_type()` - get annotation type
- `pdf_annot_quad_point_count()` / `pdf_annot_quad_point()` - get highlight rects
- `pdf_annot_color()` - get color
- `pdf_delete_annot()` - delete annotation
- `pdf_create_annot()` - create new annotation
- `pdf_set_annot_quad_points()` - set highlight rectangles
- `pdf_save_document()` - save modified PDF

## Dependencies

- zathura-pdf-mupdf plugin must be updated alongside zathura
- Poppler backend (zathura-pdf-poppler) would need equivalent implementation
- Consider API versioning for backwards compatibility

## Related Features

Once annotation layer exists, these become possible:
- **Highlight list panel**: Show all highlights, jump to location
- **Search highlights**: Find highlight by text content
- **Highlight notes**: Add text notes to highlights
- **Highlight export**: Export to markdown, org-mode, etc.
