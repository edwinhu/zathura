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
# string:"5"            - go to page 5 (just the number, no "goto")
# string:"quit"         - close zathura
# string:"version"      - show version (good for testing DBus works)
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

## File Picker (feature/file-picker branch)

The file picker provides fzf-like file selection within zathura. Located in worktree `~/projects/zathura-file-picker`.

### Features
- **Ctrl+o** or `:files` - Toggle file picker panel
- Scans ~/Downloads, ~/Documents, ~/projects for supported files
- Supports: pdf, epub, djvu, ps, cbz, cbr, xps, oxps
- Fuzzy matching by default, Tab to toggle exact match
- Type to filter, Enter to open, Escape to close

### Testing the File Picker

**Important:** The dev build may crash with system plugins due to ABI mismatch. Use locally built mupdf plugin:

```bash
# Build the mupdf plugin first (if not done)
cd ~/projects/zathura/plugins/zathura-pdf-mupdf
meson setup build && ninja -C build

# Start zathura in scratchpad (required by Hyprland hook)
ZATHURA_PLUGINS_PATH=~/projects/zathura/plugins/zathura-pdf-mupdf/build \
  hyprctl dispatch exec "[workspace special:scratchpad silent]" \
  ~/projects/zathura-file-picker/build/zathura

# Get PID and open file picker via DBus
sleep 2
ZPID=$(pgrep -f "zathura-file-picker/build" | head -1)
dbus-send --print-reply --dest=org.pwmt.zathura.PID-$ZPID \
  /org/pwmt/zathura org.pwmt.zathura.ExecuteCommand string:"files"
```

### Taking Screenshots

```bash
# Toggle scratchpad to make window visible
hyprctl dispatch togglespecialworkspace scratchpad
sleep 0.5

# Get window geometry
hyprctl clients -j | jq -r '.[] | select(.class | test("org.pwmt")) | "\(.at[0]),\(.at[1]) \(.size[0])x\(.size[1])"'

# Screenshot (adjust geometry as needed)
grim -g "12,38 1256x750" /tmp/filepicker-screenshot.png
```

### Known Issues (Phase 1)

1. ~~**No default selection**~~: FIXED - First row is now selected by default.

2. **wtype keyboard input**: Wayland security model prevents wtype from sending keyboard input to GTK widgets. This only affects automated testing - actual user interaction works because the window has focus when user types `:files`.

### Automated Test Script

```bash
/tmp/test-file-picker.sh
```

Tests:
- Opens file picker via DBus ✓
- Takes screenshots for visual verification ✓
- Verifies first row selection (blue highlight) ✓
- Note: wtype Enter simulation fails due to Wayland - use manual test

### Manual Test Checklist

1. [x] `:files` opens file picker panel
2. [x] Ctrl+o toggles file picker
3. [x] Files from ~/Downloads, ~/Documents, ~/projects appear
4. [x] First row auto-selected (blue highlight)
5. [ ] Typing filters the list (fuzzy match)
6. [ ] Tab toggles between fuzzy/exact mode
7. [ ] Arrow keys navigate the list
8. [ ] Enter opens selected file
9. [ ] Escape closes picker
10. [ ] Double-click opens file
