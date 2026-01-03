# SPEC: Unified Annotations (Phase 5)

**Status:** IMPLEMENTED

## Overview

Unify highlights and sticky notes under a single "annotations" umbrella with consistent commands and workflows.

## Goals

1. **Unified commands**: Single `:annot_import`/`:annot_export` for both highlights and notes
2. **Unified panel**: `:annotations` shows all, with `:highlights`/`:notes` as filter aliases
3. **Consistent workflow**: Same import/export/delete patterns for both types
4. **Backward simplicity**: Remove old `:hlimport`/`:hlexport` commands (no aliases)

## Non-Goals

- Syncing notes to Readwise (only highlights sync)
- Auto-import on document open
- Merging highlights and notes into a single data type

## Commands

### New Commands

| Command | Function |
|---------|----------|
| `:annot_import` | Import all embedded PDF annotations (highlights + notes) to database |
| `:annot_export [path]` | Export all database annotations (highlights + notes) to PDF |
| `:annotations` | Open panel showing all annotations (highlights + notes) |

### Modified Commands

| Command | Change |
|---------|--------|
| `:highlights` | Becomes alias for `:annotations` with type filter |
| `:notes` | Becomes alias for `:annotations` with type filter |
| `:readwise_sync` | No change (highlights only, skip notes) |

### Removed Commands

| Command | Replacement |
|---------|-------------|
| `:hlimport` | `:annot_import` |
| `:hlexport` | `:annot_export` |

## User Flows

### Import Flow

```
1. User opens PDF with embedded annotations (highlights + notes)
2. User runs :annot_import
3. System imports:
   - PDF_ANNOT_HIGHLIGHT → database highlights (yellow)
   - PDF_ANNOT_UNDERLINE → database highlights
   - PDF_ANNOT_STRIKE_OUT → database highlights
   - PDF_ANNOT_TEXT → database notes (yellow)
4. Notification: "Imported N highlights, M notes"
5. Icons change from blue (embedded) to yellow (database)
```

### Export Flow

```
1. User has highlights and notes in database
2. User runs :annot_export [optional path]
3. System exports:
   - Database highlights → PDF_ANNOT_HIGHLIGHT
   - Database notes → PDF_ANNOT_TEXT
4. Deduplication: Skip if matching annotation already exists in PDF
5. Notification: "Exported N highlights, M notes"
```

### Note Creation Flow

```
1. User presses 'n' key
2. Cursor changes to placement mode indicator
3. User clicks desired location on page
4. Note popup opens at click location
5. User types note content
6. Ctrl+Enter or click away saves to database
7. Yellow note icon appears at location
```

### Annotations Panel

```
1. User runs :annotations (or :highlights or :notes)
2. Panel opens showing:
   - Type icon (highlight stripe or note icon)
   - Page number
   - Preview text (highlight text or note content)
   - Color indicator
3. Fuzzy search filters list
4. Enter jumps to annotation
5. 'd' or 'x' deletes annotation
6. 'c' cycles color
```

## Architecture

### Command Implementation

```
:annot_import
  → for each page:
      → zathura_page_get_annotations() [existing]
      → zathura_page_get_notes() [existing]
      → deduplicate by geometry
      → save to database
  → notify count

:annot_export
  → load highlights from database
  → load notes from database
  → for each page:
      → get existing PDF annotations (for dedup)
      → zathura_page_export_annotations() [existing]
      → zathura_page_export_notes() [new or existing?]
  → save document
  → notify count
```

### Data Model

**Highlights** (existing):
```sql
CREATE TABLE highlights (
  file TEXT, id TEXT, page INT, rects_json TEXT,
  color INT, text TEXT, created_at INT
);
```

**Notes** (existing):
```sql
CREATE TABLE notes (
  file TEXT, id TEXT, page INT, x REAL, y REAL,
  content TEXT, created_at INT
);
```

No schema changes needed.

### Visual Distinction

| Annotation Type | Source | Icon/Color |
|-----------------|--------|------------|
| Highlight | Database | Yellow/Green/Blue/Red highlight overlay |
| Highlight | Embedded PDF | Same (after import becomes database) |
| Note | Database | Yellow sticky note icon |
| Note | Embedded PDF | Blue sticky note icon |

After import, embedded annotations become database annotations (blue → yellow).

## Files to Modify

### Zathura Core

| File | Changes |
|------|---------|
| `commands.c` | Add `cmd_annot_import()`, `cmd_annot_export()`, `cmd_annotations()` |
| `commands.h` | Declare new functions |
| `config.c` | Register new commands, remove old commands |
| `shortcuts.c` | Ensure 'n' key placement mode works correctly |

### Optional Cleanup

| File | Changes |
|------|---------|
| `commands.c` | Remove `cmd_highlights_import()`, `cmd_highlights_export()` |

## Implementation Steps

1. **Create `:annot_import`**: Combine `cmd_highlights_import` + note import logic
2. **Create `:annot_export`**: Combine `cmd_highlights_export` + note export logic
3. **Create `:annotations` panel**: Unified list with type filter
4. **Update `:highlights`/`:notes`**: Make them filter aliases
5. **Remove old commands**: Delete `:hlimport`/`:hlexport` registrations
6. **Test**: Full import/export/panel workflow

## Acceptance Criteria

- [ ] `:annot_import` imports both highlights and notes from PDF
- [ ] `:annot_export` exports both highlights and notes to PDF
- [ ] `:annotations` shows combined list of highlights and notes
- [ ] `:highlights` filters to highlights only
- [ ] `:notes` filters to notes only
- [ ] `:hlimport`/`:hlexport` no longer exist
- [ ] `:readwise_sync` still works (highlights only)
- [ ] Note placement with 'n' + click works
- [ ] Delete works for both types ('x' or 'd')
- [ ] No duplicates on re-import/re-export

## Test Plan

1. Open PDF with embedded highlights and notes
2. Run `:annot_import` - verify both types imported
3. Run `:annotations` - verify both types listed
4. Run `:highlights` - verify only highlights shown
5. Run `:notes` - verify only notes shown
6. Create new note with 'n' + click
7. Run `:annot_export` - verify both exported
8. Open in another viewer - verify annotations visible
9. Run `:readwise_sync` - verify only highlights synced
10. Verify `:hlimport`/`:hlexport` return "unknown command"
