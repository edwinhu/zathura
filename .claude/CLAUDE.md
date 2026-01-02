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

## Automated Testing via DBus

Zathura exposes a DBus interface for sending commands programmatically. This is more reliable than keyboard simulation (wtype/xdotool) on Wayland.

### Start zathura with logging
```bash
G_MESSAGES_DEBUG=all ZATHURA_PLUGINS_PATH=/path/to/plugin/build ./build/zathura ~/Downloads/test.pdf > /tmp/zathura.log 2>&1 &
ZATHURA_PID=$!
sleep 2
```

### Send commands via DBus
```bash
# Execute a command (e.g., hlimport)
dbus-send --print-reply --dest=org.pwmt.zathura.PID-$ZATHURA_PID \
  /org/pwmt/zathura org.pwmt.zathura.ExecuteCommand string:"hlimport"

# Other useful commands:
# string:"highlights"   - open highlights panel
# string:"goto 5"       - go to page 5
# string:"quit"         - close zathura
```

### Check results
```bash
# View debug output
grep -E "(Annotation|Exception|highlight)" /tmp/zathura.log

# Check database
sqlite3 ~/.local/share/zathura/bookmarks.sqlite \
  "SELECT id, page, substr(text,1,50) FROM highlights WHERE file LIKE '%test%';"
```

### Full test script example
```bash
#!/bin/bash
pkill zathura 2>/dev/null; sleep 1
sqlite3 ~/.local/share/zathura/bookmarks.sqlite "DELETE FROM highlights WHERE file LIKE '%bitcoin%';"

G_MESSAGES_DEBUG=all \
ZATHURA_PLUGINS_PATH=~/projects/zathura-highlights-import/plugins/zathura-pdf-mupdf/build \
~/projects/zathura-highlights-import/build/zathura ~/Downloads/bitcoin.pdf > /tmp/z.log 2>&1 &
PID=$!; sleep 2

dbus-send --print-reply --dest=org.pwmt.zathura.PID-$PID \
  /org/pwmt/zathura org.pwmt.zathura.ExecuteCommand string:"hlimport"
sleep 1

grep -E "(Annotation|Exception|Total)" /tmp/z.log
sqlite3 ~/.local/share/zathura/bookmarks.sqlite "SELECT count(*) FROM highlights WHERE file LIKE '%bitcoin%';"
kill $PID
```
