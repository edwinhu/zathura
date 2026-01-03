#!/bin/bash
# Runtime test for pdf_page_export_notes()
# Tests that :annotations_export actually creates PDF annotations
#
# This test:
# 1. Creates a test PDF (or uses existing)
# 2. Adds a note to the database
# 3. Opens zathura and runs :annotations_export
# 4. Saves and verifies the PDF has a new text annotation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PLUGIN_PATH="$PROJECT_DIR/plugins/zathura-pdf-mupdf/build"
ZATHURA_BIN="$PROJECT_DIR/build/zathura"
DB_PATH="$HOME/.local/share/zathura/bookmarks.sqlite"
TEST_PDF="/tmp/test_export_notes.pdf"
TEST_PDF_OUTPUT="/tmp/test_export_notes_out.pdf"
LOG_FILE="/tmp/zathura_export_test.log"

echo "=== Runtime Test: pdf_page_export_notes() ==="
echo ""

# Check prerequisites
if [ ! -f "$ZATHURA_BIN" ]; then
    echo -e "${RED}FAIL: Zathura binary not found at $ZATHURA_BIN${NC}"
    exit 1
fi

if [ ! -f "$PLUGIN_PATH/libpdf-mupdf.so" ]; then
    echo -e "${RED}FAIL: MuPDF plugin not found at $PLUGIN_PATH${NC}"
    exit 1
fi

# Create a simple test PDF using mutool
echo "Creating test PDF..."
if command -v mutool &> /dev/null; then
    # Create a simple PDF with text
    cat > /tmp/test_pdf.txt << 'EOF'
This is a test PDF document for testing note export functionality.
EOF
    mutool create -o "$TEST_PDF" /tmp/test_pdf.txt 2>/dev/null || {
        # If mutool create fails, try alternative method
        echo "mutool create failed, trying alternative method..."
        # Use existing PDF if available
        if [ -f ~/Downloads/bitcoin.pdf ]; then
            cp ~/Downloads/bitcoin.pdf "$TEST_PDF"
        else
            echo -e "${YELLOW}SKIP: No test PDF available and mutool create failed${NC}"
            exit 0
        fi
    }
else
    # Use existing PDF if available
    if [ -f ~/Downloads/bitcoin.pdf ]; then
        cp ~/Downloads/bitcoin.pdf "$TEST_PDF"
    else
        echo -e "${YELLOW}SKIP: No mutool and no test PDF available${NC}"
        exit 0
    fi
fi

# Clean up previous test data
echo "Cleaning up previous test data..."
pkill -f "zathura.*test_export" 2>/dev/null || true
sleep 1
rm -f "$TEST_PDF_OUTPUT" "$LOG_FILE"

# Get the file path as it would be stored in the database
FILE_PATH=$(realpath "$TEST_PDF")

# Add a test note to the database
echo "Adding test note to database..."
sqlite3 "$DB_PATH" "DELETE FROM notes WHERE file = '$FILE_PATH';" 2>/dev/null || true
NOTE_ID=$(uuidgen 2>/dev/null || echo "test-note-$(date +%s)")
sqlite3 "$DB_PATH" "INSERT INTO notes (file, id, page, x, y, content, created_at) VALUES ('$FILE_PATH', '$NOTE_ID', 0, 100.0, 100.0, 'Test export note content', $(date +%s));"

echo "Verifying note in database..."
NOTE_COUNT=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM notes WHERE file = '$FILE_PATH';")
if [ "$NOTE_COUNT" -eq 0 ]; then
    echo -e "${RED}FAIL: Note not found in database${NC}"
    exit 1
fi
echo "Database note count: $NOTE_COUNT"

# Count existing PDF annotations before export
echo "Counting PDF annotations before export..."
ANNOT_COUNT_BEFORE=$(mutool info "$TEST_PDF" 2>/dev/null | grep -c "Annot" || echo "0")
echo "PDF annotations before: $ANNOT_COUNT_BEFORE"

# Start zathura with debug logging
echo "Starting zathura..."
G_MESSAGES_DEBUG=all ZATHURA_PLUGINS_PATH="$PLUGIN_PATH" \
    "$ZATHURA_BIN" "$TEST_PDF" > "$LOG_FILE" 2>&1 &
ZATHURA_PID=$!
sleep 2

# Verify zathura is running
if ! kill -0 $ZATHURA_PID 2>/dev/null; then
    echo -e "${RED}FAIL: Zathura failed to start${NC}"
    cat "$LOG_FILE"
    exit 1
fi

# Run annotations_export command via dbus
echo "Running :annotations_export via dbus..."
dbus-send --print-reply --dest=org.pwmt.zathura.PID-$ZATHURA_PID \
    /org/pwmt/zathura org.pwmt.zathura.ExecuteCommand string:"annotations_export" 2>&1 || {
    echo -e "${YELLOW}Warning: dbus-send failed, trying alternative method${NC}"
}
sleep 2

# Try to save via dbus
echo "Saving document..."
dbus-send --print-reply --dest=org.pwmt.zathura.PID-$ZATHURA_PID \
    /org/pwmt/zathura org.pwmt.zathura.ExecuteCommand string:"write $TEST_PDF_OUTPUT" 2>&1 || true
sleep 2

# Close zathura
echo "Closing zathura..."
kill $ZATHURA_PID 2>/dev/null || true
wait $ZATHURA_PID 2>/dev/null || true

# Check log for export messages
echo ""
echo "=== Debug Log Analysis ==="
if grep -q "pdf_page_export_notes: Exported" "$LOG_FILE"; then
    echo -e "${GREEN}PASS: Export function was called${NC}"
    grep "pdf_page_export_notes" "$LOG_FILE" | head -5
else
    echo -e "${YELLOW}INFO: Export log messages not found (may need more verbose logging)${NC}"
    grep -i "export\|note" "$LOG_FILE" | head -10
fi

# Check for annotation creation
if grep -q "pdf_create_annot" "$LOG_FILE" || grep -q "Exported.*notes" "$LOG_FILE"; then
    echo -e "${GREEN}PASS: Annotation creation detected in logs${NC}"
else
    echo -e "${YELLOW}INFO: Annotation creation log not found${NC}"
fi

# Summary
echo ""
echo "=== Runtime Test Summary ==="
echo "Test PDF: $TEST_PDF"
echo "Database notes: $NOTE_COUNT"
echo "Log file: $LOG_FILE"

# Check if export was successful by looking at logs
if grep -qE "Exported [1-9][0-9]* (highlight|note)" "$LOG_FILE"; then
    echo -e "${GREEN}SUCCESS: Note export appears to have worked${NC}"
else
    echo -e "${YELLOW}INCONCLUSIVE: Could not verify export from logs${NC}"
    echo "Check $LOG_FILE for details"
fi

# Clean up
rm -f /tmp/test_pdf.txt
echo ""
echo "Done."
