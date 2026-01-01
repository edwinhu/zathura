# SPEC: Highlight List Panel

## Feature

`:highlights` command opens a modal panel showing all highlights with fuzzy search, navigation, and actions.

## Requirements

| Feature | Spec |
|---------|------|
| Access | `:highlights` command |
| Display | Text preview + page number + color indicator |
| Search | fzf-style fuzzy matching (chars in order) |
| Actions | Enter=jump, d/x=delete, c=cycle color |
| Behavior | Panel stays open after jump |

## UI Layout

```
+------------------------------------------+
| [Search: _________________________]       |
+------------------------------------------+
| Color | Page | Text                       |
|   Y   |   5  | "Important concept..."    |
|   G   |  12  | "Key definition here..."  |
+------------------------------------------+
```

## Files to Modify

| File | Changes |
|------|---------|
| `zathura/zathura.h` | Add `ui.highlights`, `ui.highlights_search`, `modes.highlights` |
| `zathura/config.c` | Register mode, command, keybindings for HIGHLIGHTS mode |
| `zathura/commands.c` | Add `cmd_highlights()` |
| `zathura/commands.h` | Declare `cmd_highlights()` |
| `zathura/shortcuts.c` | Add `sc_toggle_highlights()` |
| `zathura/shortcuts.h` | Declare `sc_toggle_highlights()` |
| `zathura/callbacks.c` | Add row/key/search callbacks |
| `zathura/callbacks.h` | Declare callbacks |

## Key Patterns

### Fuzzy Match (fzf-style)
```c
// "hlst" matches "highlight" - chars must appear in order
static gboolean fuzzy_match(const char* pattern, const char* text) {
  const char* p = pattern;
  const char* t = text;
  while (*p && *t) {
    if (g_ascii_tolower(*p) == g_ascii_tolower(*t)) p++;
    t++;
  }
  return (*p == '\0');
}
```

### Panel Toggle (follow sc_toggle_index pattern)
- Create GtkBox with GtkEntry + GtkScrolledWindow + GtkTreeView
- Use GtkListStore (4 cols: text, page_str, color_str, highlight_ptr)
- Wrap with GtkTreeModelFilter for search
- Toggle: normal mode ↔ highlights mode

### Stay Open After Jump
- Unlike index mode, do NOT call sc_toggle_highlights() after jump
- Just call page_set() and leave panel open

## Acceptance Criteria

- [ ] `:highlights` opens panel with all highlights listed
- [ ] Typing in search filters list with fuzzy matching
- [ ] Enter on row jumps to highlight page (panel stays open)
- [ ] `d` or `x` deletes highlight, refreshes list and view
- [ ] `c` cycles color (Y→G→B→R→Y), updates DB and view
- [ ] Escape closes panel, returns to normal mode
- [ ] Empty list shows "No highlights" message
