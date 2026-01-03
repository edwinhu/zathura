#!/bin/bash
# Test: Unified Note Popup UX
# Verifies that both native and embedded notes use the same popup for editing

cd "$(dirname "$0")/.."

PASSED=0
FAILED=0

pass() {
  echo "PASS: $1"
  ((PASSED++))
}

fail() {
  echo "FAIL: $1"
  ((FAILED++))
}

echo "=== Testing Unified Note Popup UX ==="

# Test 1: Note popup has delete button
echo ""
echo "Test 1: Note popup has delete button"
if grep -q "delete.*btn\|Delete.*button" zathura/note-popup.c 2>/dev/null; then
  pass "Delete button exists in note-popup.c"
else
  fail "Delete button not found in note-popup.c"
fi

# Test 2: Delete button callback in note-popup.c
echo ""
echo "Test 2: Delete button callback exists"
if grep -q "cb_note_popup_delete\|delete.*callback\|delete.*clicked" zathura/note-popup.c 2>/dev/null; then
  pass "Delete callback exists in note-popup.c"
else
  fail "Delete callback not found in note-popup.c"
fi

# Test 3: Embedded note click opens popup (not just selects)
echo ""
echo "Test 3: Embedded note click opens popup"
# The embedded note click handler should call sc_note_handle_embedded_edit_click (called on button release)
if grep -q "sc_note_handle_embedded_edit_click" zathura/page-widget.c 2>/dev/null; then
  pass "Embedded note click opens popup"
else
  fail "Embedded note click does not open popup"
fi

# Test 4: Standalone delete button drawing removed
echo ""
echo "Test 4: Standalone delete button drawing removed"
# Check that DELETE_BUTTON drawing code is removed
if ! grep -q "DELETE_BUTTON: drawing at" zathura/page-widget.c 2>/dev/null; then
  pass "Standalone delete button drawing removed"
else
  fail "Standalone delete button drawing still exists"
fi

# Test 5: Standalone delete button click detection removed
echo ""
echo "Test 5: Standalone delete button click detection removed"
# Check that DELETE_BTN click code is removed (not related to popup)
if ! grep -q "DELETE_BTN: CLICKED!" zathura/page-widget.c 2>/dev/null; then
  pass "Standalone delete button click detection removed"
else
  fail "Standalone delete button click detection still exists"
fi

# Test 6: Note popup tracks note type (native vs embedded)
echo ""
echo "Test 6: Note popup can track note type"
if grep -q "is_embedded\|note_type\|NOTE_TYPE" zathura/note-popup.c 2>/dev/null || \
   grep -q "embedded.*note.*popup\|NOTE_POPUP_KEY_IS_EMBEDDED" zathura/note-popup.c 2>/dev/null; then
  pass "Note popup tracks note type"
else
  fail "Note popup does not track note type - needed for proper deletion"
fi

# Test 7: Build succeeds
echo ""
echo "Test 7: Build succeeds"
if ninja -C build > /tmp/build.log 2>&1; then
  pass "Build succeeds"
else
  fail "Build failed - check /tmp/build.log"
fi

echo ""
echo "=== Results: $PASSED passed, $FAILED failed ==="
exit $FAILED
