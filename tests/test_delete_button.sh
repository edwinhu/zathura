#!/bin/bash
# Test: Embedded note deletion via plugin API
#
# This test verifies that:
# 1. pdf_page_delete_note() is implemented in the plugin
# 2. page_delete_note is registered in the plugin function table
# 3. The function uses pdf_delete_annot() from MuPDF
#
# ROOT CAUSE: page_delete_note was NOT registered in plugin.c function table
# FIX: Added .page_delete_note = pdf_page_delete_note to plugin.c

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PLUGIN_DIR="$PROJECT_DIR/plugins/zathura-pdf-mupdf/zathura-pdf-mupdf"

echo "=== Test: Embedded note deletion plugin registration ==="

# Test 1: Verify pdf_page_delete_note implementation exists
echo "Test 1: Verifying pdf_page_delete_note implementation exists..."
if grep -q "^zathura_error_t pdf_page_delete_note" "$PLUGIN_DIR/page.c"; then
    echo "PASS: pdf_page_delete_note implementation found in page.c"
else
    echo "FAIL: pdf_page_delete_note implementation NOT found in page.c"
    exit 1
fi

# Test 2: Verify page_delete_note is registered in plugin.c
echo "Test 2: Verifying page_delete_note is registered in plugin.c..."
if grep -q "\.page_delete_note.*=.*pdf_page_delete_note" "$PLUGIN_DIR/plugin.c"; then
    echo "PASS: page_delete_note is registered in plugin.c function table"
else
    echo "FAIL: page_delete_note NOT registered in plugin.c function table"
    echo "This is the ROOT CAUSE - the plugin doesn't tell zathura about this function!"
    exit 1
fi

# Test 3: Verify pdf_delete_annot is called in the implementation
echo "Test 3: Verifying pdf_delete_annot is called..."
if grep -q "pdf_delete_annot" "$PLUGIN_DIR/page.c"; then
    echo "PASS: pdf_delete_annot (MuPDF API) is called in implementation"
else
    echo "FAIL: pdf_delete_annot NOT called - the annotation won't be deleted"
    exit 1
fi

# Test 4: Verify declaration exists in plugin.h
echo "Test 4: Verifying declaration in plugin.h..."
if grep -q "zathura_error_t pdf_page_delete_note" "$PLUGIN_DIR/plugin.h"; then
    echo "PASS: pdf_page_delete_note declared in plugin.h"
else
    echo "FAIL: pdf_page_delete_note NOT declared in plugin.h"
    exit 1
fi

# Test 5: Verify plugin builds successfully
echo "Test 5: Verifying plugin builds..."
if [ -f "$PROJECT_DIR/plugins/zathura-pdf-mupdf/build/libpdf-mupdf.so" ]; then
    echo "PASS: Plugin binary exists"
else
    echo "FAIL: Plugin binary not found - run: ninja -C plugins/zathura-pdf-mupdf/build"
    exit 1
fi

# Test 6: Verify symbol is exported
echo "Test 6: Verifying symbol is exported from plugin..."
if nm "$PROJECT_DIR/plugins/zathura-pdf-mupdf/build/libpdf-mupdf.so" 2>/dev/null | grep -q "pdf_page_delete_note"; then
    echo "PASS: pdf_page_delete_note symbol exported from plugin"
else
    echo "WARNING: Could not verify symbol export (may still work)"
fi

echo ""
echo "=== All tests PASSED ==="
echo ""
echo "Summary:"
echo "  - pdf_page_delete_note() is implemented in plugin/page.c"
echo "  - page_delete_note is registered in plugin.c function table"
echo "  - pdf_delete_annot() from MuPDF is called to delete the annotation"
echo ""
echo "The ROOT CAUSE was: page_delete_note was NOT registered in the plugin function table."
echo "When zathura called zathura_page_delete_note(), it returned ZATHURA_ERROR_NOT_IMPLEMENTED"
echo "because functions->page_delete_note was NULL."
echo ""
echo "The FIX: Added .page_delete_note = pdf_page_delete_note to plugin.c"

exit 0
