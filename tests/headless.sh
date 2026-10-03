#!/bin/sh
cd "$(dirname "$0")/.."
AURA_BIN="${AURA_BIN:-/home/dev/code/grok-dev/aura-grok/build/aura}"
export AURA_BIN
export AURA_PATH="${AURA_PATH:-/home/dev/code/grok-dev/aura-grok/lib}"
export AURA_SANDBOX=off
export AURA_PIPELINE_STRICT="${AURA_PIPELINE_STRICT:-force-soa}"
export GLYPHLINGS_FAST=1
if [ ! -x "$AURA_BIN" ]; then
  echo "找不到 Aura 二进制：$AURA_BIN"
  exit 127
fi
fail=0
run() {
  echo "== $1 =="
  out=$("$AURA_BIN" "$1" 2>&1) || true
  printf '%s\n' "$out"
  printf '%s\n' "$out" | grep -q GLYPHLINGS_OK || { echo "MISSING OK $1"; fail=1; }
  printf '%s\n' "$out" | grep -q GLYPHLINGS_FAIL && { echo "HAD FAIL $1"; fail=1; }
}
run tests/roundtrip.aura
run tests/accept.aura
run tests/refuse.aura
run tests/backspace.aura
run tests/refuse_pheromone.aura
run tests/hatch_heal.aura
echo "== grep =="
if sh tests/grep_gates.sh; then
  true
else
  fail=1
fi
echo "== pipe =="
if cc -std=c11 -Wall -Wextra -Werror -o tests/pipe_lf tests/pipe_lf.c && ./tests/pipe_lf; then
  true
else
  echo "GLYPHLINGS_FAIL pipe"
  fail=1
fi
snap="${TMPDIR:-/tmp}/glyphlings-session-snap"
rm -f "$snap" "$snap.writing"
mkdir -p "$(dirname "$snap")"
echo "== session save =="
out=$(GLYPHLINGS_SNAP="$snap" "$AURA_BIN" tests/session_save.aura 2>&1) || true
printf '%s\n' "$out"
printf '%s\n' "$out" | grep -q GLYPHLINGS_OK || { echo "MISSING OK save"; fail=1; }
printf '%s\n' "$out" | grep -q GLYPHLINGS_FAIL && { echo "HAD FAIL save"; fail=1; }
echo "== session load =="
out=$(GLYPHLINGS_SNAP="$snap" "$AURA_BIN" tests/session_load.aura 2>&1) || true
printf '%s\n' "$out"
printf '%s\n' "$out" | grep -q GLYPHLINGS_OK || { echo "MISSING OK load"; fail=1; }
printf '%s\n' "$out" | grep -q GLYPHLINGS_FAIL && { echo "HAD FAIL load"; fail=1; }
rm -f "$snap" "$snap.writing"
exit "$fail"
