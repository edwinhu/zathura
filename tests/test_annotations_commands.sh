#!/bin/bash
# Test unified annotations_import/annotations_export commands
# This test verifies at the source level:
# 1. :annotations_import imports both highlights AND notes from PDF
# 2. :annotations_export exports both highlights AND notes to PDF
# 3. :hlimport and :hlexport NO LONGER exist (removed)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== Test: Unified annotations_import/annotations_export commands ==="
echo ""

PASS_COUNT=0
FAIL_COUNT=0

# Test 1: Check that hlimport command is REMOVED from config.c
echo "Test 1: Verify :hlimport command is removed from config.c"
if grep -q '"hlimport"' "$PROJECT_DIR/zathura/config.c"; then
    echo -e "${RED}FAIL: :hlimport command still exists in config.c${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
else
    echo -e "${GREEN}PASS: :hlimport command removed from config.c${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
fi

# Test 2: Check that hlexport command is REMOVED from config.c
echo ""
echo "Test 2: Verify :hlexport command is removed from config.c"
if grep -q '"hlexport"' "$PROJECT_DIR/zathura/config.c"; then
    echo -e "${RED}FAIL: :hlexport command still exists in config.c${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
else
    echo -e "${GREEN}PASS: :hlexport command removed from config.c${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
fi

# Test 3: annotations_import command is registered in config.c
echo ""
echo "Test 3: Verify :annotations_import is registered in config.c"
if grep -q '"annotations_import"' "$PROJECT_DIR/zathura/config.c"; then
    echo -e "${GREEN}PASS: :annotations_import command registered${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: :annotations_import not found in config.c${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 4: annotations_export command is registered in config.c
echo ""
echo "Test 4: Verify :annotations_export is registered in config.c"
if grep -q '"annotations_export"' "$PROJECT_DIR/zathura/config.c"; then
    echo -e "${GREEN}PASS: :annotations_export command registered${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: :annotations_export not found in config.c${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 5: cmd_annot_import message mentions BOTH highlights AND notes
echo ""
echo "Test 5: Verify cmd_annot_import mentions both highlights and notes"
if grep -q 'Imported %u highlights and %u notes' "$PROJECT_DIR/zathura/commands.c"; then
    echo -e "${GREEN}PASS: cmd_annot_import message mentions highlights AND notes${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: cmd_annot_import message doesn't mention both types${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 6: cmd_annot_export message mentions BOTH highlights AND notes
echo ""
echo "Test 6: Verify cmd_annot_export mentions both highlights and notes"
if grep -q 'Exported %u highlights and %u notes' "$PROJECT_DIR/zathura/commands.c"; then
    echo -e "${GREEN}PASS: cmd_annot_export message mentions highlights AND notes${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: cmd_annot_export message doesn't mention both types${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 7: cmd_annot_import imports notes via zathura_page_get_notes
echo ""
echo "Test 7: Verify cmd_annot_import calls zathura_page_get_notes"
if grep -q 'zathura_page_get_notes' "$PROJECT_DIR/zathura/commands.c"; then
    echo -e "${GREEN}PASS: cmd_annot_import uses zathura_page_get_notes${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: cmd_annot_import doesn't call zathura_page_get_notes${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 8: cmd_annot_export exports notes via zathura_page_export_notes
echo ""
echo "Test 8: Verify cmd_annot_export calls zathura_page_export_notes"
if grep -q 'zathura_page_export_notes' "$PROJECT_DIR/zathura/commands.c"; then
    echo -e "${GREEN}PASS: cmd_annot_export uses zathura_page_export_notes${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: cmd_annot_export doesn't call zathura_page_export_notes${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 9: Build succeeds
echo ""
echo "Test 9: Verify build succeeds"
if ninja -C "$PROJECT_DIR/build" > /dev/null 2>&1; then
    echo -e "${GREEN}PASS: Build succeeds${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: Build failed${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 10: Unit tests pass
echo ""
echo "Test 10: Verify unit tests pass"
if ninja -C "$PROJECT_DIR/build" test > /dev/null 2>&1; then
    echo -e "${GREEN}PASS: Unit tests pass${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "${RED}FAIL: Unit tests failed${NC}"
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
