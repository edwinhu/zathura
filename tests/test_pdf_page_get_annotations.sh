#!/bin/bash
# Test: pdf_page_get_annotations() implementation in MuPDF plugin
# TDD RED/GREEN test

cd /home/edwinhu/projects/zathura

PLUGIN_PAGE="/home/edwinhu/projects/zathura/plugins/zathura-pdf-mupdf/zathura-pdf-mupdf/page.c"
PLUGIN_H="/home/edwinhu/projects/zathura/plugins/zathura-pdf-mupdf/zathura-pdf-mupdf/plugin.h"
PLUGIN_C="/home/edwinhu/projects/zathura/plugins/zathura-pdf-mupdf/zathura-pdf-mupdf/plugin.c"

PASSED=0
FAILED=0

echo "=== Test: pdf_page_get_annotations() implementation ==="
echo

# Test 1: Verify function is declared in plugin.h
echo "Test 1: Verify pdf_page_get_annotations is declared in plugin.h"
if grep -q 'pdf_page_get_annotations' "$PLUGIN_H"; then
    echo "PASS: pdf_page_get_annotations declared in plugin.h"
    PASSED=$((PASSED+1))
else
    echo "FAIL: pdf_page_get_annotations not declared in plugin.h"
    FAILED=$((FAILED+1))
fi

# Test 2: Verify function is registered in plugin.c
echo
echo "Test 2: Verify pdf_page_get_annotations is registered in plugin.c"
if grep -q '\.page_get_annotations.*=.*pdf_page_get_annotations' "$PLUGIN_C"; then
    echo "PASS: pdf_page_get_annotations registered in plugin.c"
    PASSED=$((PASSED+1))
else
    echo "FAIL: pdf_page_get_annotations not registered in plugin.c"
    FAILED=$((FAILED+1))
fi

# Test 3: Verify function is implemented in page.c
echo
echo "Test 3: Verify pdf_page_get_annotations is implemented in page.c"
if grep -q 'girara_list_t\* pdf_page_get_annotations' "$PLUGIN_PAGE"; then
    echo "PASS: pdf_page_get_annotations implemented in page.c"
    PASSED=$((PASSED+1))
else
    echo "FAIL: pdf_page_get_annotations not implemented in page.c"
    FAILED=$((FAILED+1))
fi

# Test 4: Verify implementation iterates PDF annotations
echo
echo "Test 4: Verify implementation uses pdf_first_annot/pdf_next_annot"
if grep -A 50 'girara_list_t\* pdf_page_get_annotations' "$PLUGIN_PAGE" | grep -q 'pdf_first_annot\|pdf_next_annot'; then
    echo "PASS: Implementation iterates PDF annotations"
    PASSED=$((PASSED+1))
else
    echo "FAIL: pdf_first_annot/pdf_next_annot not found in implementation"
    FAILED=$((FAILED+1))
fi

# Test 5: Verify implementation checks annotation types (HIGHLIGHT, UNDERLINE, STRIKE_OUT)
echo
echo "Test 5: Verify implementation filters by annotation type"
if grep -A 100 'girara_list_t\* pdf_page_get_annotations' "$PLUGIN_PAGE" | grep -qE 'PDF_ANNOT_HIGHLIGHT|PDF_ANNOT_UNDERLINE|PDF_ANNOT_STRIKE_OUT'; then
    echo "PASS: Implementation filters annotation types"
    PASSED=$((PASSED+1))
else
    echo "FAIL: PDF_ANNOT_HIGHLIGHT/UNDERLINE/STRIKE_OUT not found in implementation"
    FAILED=$((FAILED+1))
fi

# Test 6: Verify implementation extracts quad points
echo
echo "Test 6: Verify implementation extracts quad points"
if grep -A 100 'girara_list_t\* pdf_page_get_annotations' "$PLUGIN_PAGE" | grep -qE 'pdf_annot_quad_point|pdf_annot_quad_point_count'; then
    echo "PASS: Implementation extracts quad points"
    PASSED=$((PASSED+1))
else
    echo "FAIL: pdf_annot_quad_point not found in implementation"
    FAILED=$((FAILED+1))
fi

# Test 7: Verify implementation maps colors
echo
echo "Test 7: Verify implementation maps PDF colors to zathura_highlight_color_t"
if grep -A 100 'girara_list_t\* pdf_page_get_annotations' "$PLUGIN_PAGE" | grep -qE 'pdf_annot_color|ZATHURA_HIGHLIGHT'; then
    echo "PASS: Implementation maps colors"
    PASSED=$((PASSED+1))
else
    echo "FAIL: Color mapping not found in implementation"
    FAILED=$((FAILED+1))
fi

# Test 8: Verify implementation creates zathura_highlight_t
echo
echo "Test 8: Verify implementation creates zathura_highlight_t"
if grep -A 150 'girara_list_t\* pdf_page_get_annotations' "$PLUGIN_PAGE" | grep -qE 'zathura_highlight_new'; then
    echo "PASS: Implementation creates zathura_highlight_t"
    PASSED=$((PASSED+1))
else
    echo "FAIL: zathura_highlight_t creation not found"
    FAILED=$((FAILED+1))
fi

# Test 9: Build MuPDF plugin
echo
echo "Test 9: Verify MuPDF plugin build succeeds"
if ninja -C plugins/zathura-pdf-mupdf/build >/dev/null 2>&1; then
    echo "PASS: Plugin build succeeds"
    PASSED=$((PASSED+1))
else
    echo "FAIL: Plugin build failed"
    FAILED=$((FAILED+1))
fi

# Test 10: Build main zathura
echo
echo "Test 10: Verify main zathura build succeeds"
if ninja -C build >/dev/null 2>&1; then
    echo "PASS: Main build succeeds"
    PASSED=$((PASSED+1))
else
    echo "FAIL: Main build failed"
    FAILED=$((FAILED+1))
fi

echo
echo "=== Test Summary ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo "ALL TESTS PASSED"
    exit 0
else
    echo "SOME TESTS FAILED"
    exit 1
fi
