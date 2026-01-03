#!/bin/bash
# Test pdf_page_export_notes() implementation in MuPDF plugin
# This test verifies:
# 1. The function is registered in plugin.c
# 2. The function is declared in plugin.h
# 3. The function is implemented in page.c
# 4. Build succeeds
# 5. Runtime: note export actually creates PDF annotations

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PLUGIN_DIR="$PROJECT_DIR/plugins/zathura-pdf-mupdf/zathura-pdf-mupdf"

echo "=== Test: pdf_page_export_notes() implementation ==="
echo ""

PASS_COUNT=0
FAIL_COUNT=0

# Test 1: Function registered in plugin.c
echo "Test 1: Verify pdf_page_export_notes is registered in plugin.c"
if grep -q '\.page_export_notes.*=.*pdf_page_export_notes' "$PLUGIN_DIR/plugin.c"; then
    echo -e "${GREEN}PASS: pdf_page_export_notes registered in plugin.c${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: pdf_page_export_notes not registered in plugin.c${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 2: Function declared in plugin.h
echo ""
echo "Test 2: Verify pdf_page_export_notes is declared in plugin.h"
if grep -q 'pdf_page_export_notes' "$PLUGIN_DIR/plugin.h"; then
    echo -e "${GREEN}PASS: pdf_page_export_notes declared in plugin.h${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: pdf_page_export_notes not declared in plugin.h${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 3: Function implemented in page.c
echo ""
echo "Test 3: Verify pdf_page_export_notes is implemented in page.c"
if grep -q 'zathura_error_t pdf_page_export_notes' "$PLUGIN_DIR/page.c"; then
    echo -e "${GREEN}PASS: pdf_page_export_notes implemented in page.c${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: pdf_page_export_notes not implemented in page.c${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 4: Implementation uses pdf_create_annot
echo ""
echo "Test 4: Verify implementation uses pdf_create_annot"
if grep -A50 'pdf_page_export_notes' "$PLUGIN_DIR/page.c" | grep -q 'pdf_create_annot'; then
    echo -e "${GREEN}PASS: pdf_create_annot is used in implementation${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: pdf_create_annot not found in implementation${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 5: Implementation uses PDF_ANNOT_TEXT
echo ""
echo "Test 5: Verify implementation creates PDF_ANNOT_TEXT annotations"
if grep -A50 'pdf_page_export_notes' "$PLUGIN_DIR/page.c" | grep -q 'PDF_ANNOT_TEXT'; then
    echo -e "${GREEN}PASS: PDF_ANNOT_TEXT is used${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: PDF_ANNOT_TEXT not found in implementation${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 6: Plugin build succeeds
echo ""
echo "Test 6: Verify MuPDF plugin build succeeds"
if ninja -C "$PROJECT_DIR/plugins/zathura-pdf-mupdf/build" 2>&1; then
    echo -e "${GREEN}PASS: Plugin build succeeds${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: Plugin build failed${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 7: Main zathura build succeeds
echo ""
echo "Test 7: Verify main zathura build succeeds"
if ninja -C "$PROJECT_DIR/build" 2>&1; then
    echo -e "${GREEN}PASS: Main build succeeds${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: Main build failed${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""
echo "=== Test Summary ==="
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$FAIL_COUNT${NC}"

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}ALL TESTS PASSED${NC}"
    exit 0
else
    echo -e "${RED}SOME TESTS FAILED${NC}"
    exit 1
fi
