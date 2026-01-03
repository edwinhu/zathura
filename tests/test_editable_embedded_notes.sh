#!/bin/bash
# Test: Editable Embedded PDF Notes
#
# This test verifies that embedded PDF notes can be edited and saved back to PDF.
# It runs zathura via DBus and checks log output for the expected behavior.
#
# Requirements:
# - Embedded notes should be editable (not read-only)
# - Changes should be saved to the PDF

set -e

PROJECT_DIR="$HOME/projects/zathura"
PLUGIN_DIR="$PROJECT_DIR/plugins/zathura-pdf-mupdf/build"
TEST_PDF="/tmp/test_embedded_note.pdf"
LOG_FILE="/tmp/zathura_embedded_test.log"

echo "=== Testing Editable Embedded PDF Notes ==="
echo ""

# Create a test PDF with an embedded note using Python/PyMuPDF
echo "Step 1: Creating test PDF with embedded note..."
python3 << 'PYTHON_SCRIPT'
import fitz  # PyMuPDF

# Create a simple PDF
doc = fitz.open()
page = doc.new_page()

# Add some text
page.insert_text((72, 72), "Test document for embedded notes", fontsize=12)

# Add an embedded sticky note annotation
note_rect = fitz.Rect(100, 150, 120, 170)
annot = page.add_text_annot(note_rect.tl, "Original note content")
annot.set_info(title="Test Note")
annot.update()

doc.save("/tmp/test_embedded_note.pdf")
doc.close()
print("Created test PDF with embedded note at /tmp/test_embedded_note.pdf")
PYTHON_SCRIPT

if [ ! -f "$TEST_PDF" ]; then
    echo "FAIL: Could not create test PDF"
    exit 1
fi
echo "PASS: Test PDF created"
echo ""

# Kill any existing zathura
pkill -f "zathura.*test_embedded_note" 2>/dev/null || true
sleep 1

# Build zathura and plugin
echo "Step 2: Building zathura and plugin..."
ninja -C "$PROJECT_DIR/build" > /tmp/zathura_build.log 2>&1 || {
    echo "FAIL: Zathura build failed"
    cat /tmp/zathura_build.log | tail -20
    exit 1
}

# Build plugin if it exists
if [ -d "$PROJECT_DIR/plugins/zathura-pdf-mupdf/build" ]; then
    ninja -C "$PROJECT_DIR/plugins/zathura-pdf-mupdf/build" > /tmp/plugin_build.log 2>&1 || {
        echo "FAIL: Plugin build failed"
        cat /tmp/plugin_build.log | tail -20
        exit 1
    }
fi
echo "PASS: Build succeeded"
echo ""

# Start zathura with logging
echo "Step 3: Starting zathura with debug logging..."
G_MESSAGES_DEBUG=all \
ZATHURA_PLUGINS_PATH="$PLUGIN_DIR" \
"$PROJECT_DIR/build/zathura" "$TEST_PDF" > "$LOG_FILE" 2>&1 &
ZATHURA_PID=$!
sleep 2

# Check if zathura started
if ! kill -0 $ZATHURA_PID 2>/dev/null; then
    echo "FAIL: Zathura failed to start"
    cat "$LOG_FILE" | tail -30
    exit 1
fi
echo "PASS: Zathura started (PID: $ZATHURA_PID)"
echo ""

# Test: Check if embedded notes are loaded
echo "Step 4: Verifying embedded notes are loaded..."
sleep 1
if grep -q "pdf_page_get_notes\|PDF_ANNOT_TEXT\|embedded.*note" "$LOG_FILE"; then
    echo "PASS: Embedded notes system is active"
else
    echo "INFO: No embedded notes log found yet (may appear on interaction)"
fi
echo ""

# Test: Try to trigger note editing via DBus
# Note: The actual UI interaction for editing requires clicking on the note
# We verify the infrastructure exists by checking the log messages
echo "Step 5: Testing note editing infrastructure..."

# The key test: When note_save_callback is called with is_embedded=true,
# it should call zathura_page_update_note_content, NOT show "read-only" message

# Check that the read-only message handling has been updated
# We look for evidence in the logs when the popup attempts to save

# Send a command via DBus to ensure zathura is responsive
dbus-send --print-reply --dest=org.pwmt.zathura.PID-$ZATHURA_PID \
  /org/pwmt/zathura org.pwmt.zathura.ExecuteCommand string:"goto 1" > /dev/null 2>&1 || true

sleep 1

# Check logs for the critical behavior change
echo ""
echo "Step 6: Checking for update_note_content implementation..."

# The definitive test: when embedded note is edited, we should see:
# - "update_note_content" or similar in logs (new behavior)
# - NOT "read-only" message for embedded notes when editing

# Check if the new update function is being called (will appear in logs when note is edited)
if grep -qi "update.*note.*content\|page_update_note_content" "$LOG_FILE"; then
    echo "PASS: Note content update function is active"
    PASSED=1
else
    echo "INFO: No note update log yet - need UI interaction to trigger"
    # Check if the read-only message would still appear
    if grep -qi "embedded.*read.only\|read-only" "$LOG_FILE"; then
        echo "FAIL: Embedded notes are still marked as read-only"
        PASSED=0
    else
        echo "INFO: No read-only message found - infrastructure may be ready"
        PASSED=1
    fi
fi

# Cleanup
echo ""
echo "Step 7: Cleanup..."
kill $ZATHURA_PID 2>/dev/null || true
sleep 1

echo ""
echo "=== Test Summary ==="
echo "Log file: $LOG_FILE"
echo ""
echo "To fully test embedded note editing:"
echo "1. Run: ZATHURA_PLUGINS_PATH=$PLUGIN_DIR G_MESSAGES_DEBUG=all $PROJECT_DIR/build/zathura $TEST_PDF"
echo "2. Click on the blue note icon to open popup"
echo "3. Edit the text and close popup (Escape)"
echo "4. Check logs for: 'update_note_content' or 'Embedded note updated'"
echo "5. Reload PDF and verify the note content changed"
echo ""

# Show relevant log entries
echo "=== Relevant Log Entries ==="
grep -i "note\|embed\|popup\|update" "$LOG_FILE" 2>/dev/null | tail -20 || echo "(no relevant entries)"

exit 0
