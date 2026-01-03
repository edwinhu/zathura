#!/bin/bash
# Test: File monitor race condition fix
# Verifies that the file monitor is stopped before saving embedded notes
# and restarted after, to prevent reload race condition

set -e

ZATHURA_DIR="/home/edwinhu/projects/zathura"
SHORTCUTS_C="$ZATHURA_DIR/zathura/shortcuts.c"

echo "=== Testing File Monitor Race Condition Fix ==="

# Test 1: File monitor is stopped before save_as in note_save_callback
echo -n "Test 1: File monitor stopped before save_as call... "
if grep -A5 "Suppress file monitor during internal save" "$SHORTCUTS_C" | grep -q "zathura_filemonitor_stop"; then
    echo "PASS"
else
    echo "FAIL: zathura_filemonitor_stop not found before save_as"
    exit 1
fi

# Test 2: File monitor is restarted after save completes
echo -n "Test 2: File monitor restarted after save completes... "
if grep -A20 "zathura_document_save_as" "$SHORTCUTS_C" | grep -q "zathura_filemonitor_start"; then
    echo "PASS"
else
    echo "FAIL: zathura_filemonitor_start not found after save_as"
    exit 1
fi

# Test 3: Proper null check for monitor before stopping
echo -n "Test 3: Null check before stopping file monitor... "
if grep -B1 "zathura_filemonitor_stop" "$SHORTCUTS_C" | grep -q "file_monitor.monitor != NULL"; then
    echo "PASS"
else
    echo "FAIL: Missing null check before stopping file monitor"
    exit 1
fi

# Test 4: Proper null check for monitor before restarting
echo -n "Test 4: Null check before restarting file monitor... "
if grep -B1 "zathura_filemonitor_start" "$SHORTCUTS_C" | grep -q "file_monitor.monitor != NULL"; then
    echo "PASS"
else
    echo "FAIL: Missing null check before restarting file monitor"
    exit 1
fi

# Test 5: Build succeeds
echo -n "Test 5: Build succeeds... "
if ninja -C "$ZATHURA_DIR/build" > /dev/null 2>&1; then
    echo "PASS"
else
    echo "FAIL: Build failed"
    exit 1
fi

echo ""
echo "=== All tests passed ==="
