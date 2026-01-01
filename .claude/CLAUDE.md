# Zathura Development

## Building

```bash
cd ~/projects/zathura-highlight-list
meson setup build  # first time only
ninja -C build
```

## Running Dev Build

The dev build doesn't know where to find PDF plugins. Use:

```bash
ZATHURA_PLUGINS_PATH=/usr/lib/zathura ./build/zathura <file.pdf>
```

Or create an alias:
```bash
alias zathura-dev='ZATHURA_PLUGINS_PATH=/usr/lib/zathura ~/projects/zathura-highlight-list/build/zathura'
```

## Testing Highlights

```bash
# Open a PDF
ZATHURA_PLUGINS_PATH=/usr/lib/zathura ./build/zathura ~/Downloads/bitcoin.pdf

# Create highlights: select text, press h (yellow), j (green), k (blue), or l (red)
# List highlights: :highlights
# In highlight panel:
#   - Type to fuzzy search
#   - Enter to jump to highlight
#   - d/x to delete
#   - c to cycle color
#   - Escape to close
```

## Database Location

Highlights are stored in SQLite at:
```
~/.local/share/zathura/bookmarks.sqlite
```

Table: `highlights` with columns: file, id, page, rects_json, color, text, created_at
