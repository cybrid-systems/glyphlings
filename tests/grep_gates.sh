#!/bin/sh
set -e
cd "$(dirname "$0")/.."
if grep -nE '\b(cat|dog|bee|sun|fox|pig|hen|ant|zoo|scene|score)\b' display/*.c; then
  echo "GLYPHLINGS_FAIL display word"
  exit 1
fi
if grep -nE '\(eval |\(load |\(shell |http-get|std/net|pheromone:init|security:grant-capability!' aura/*.aura; then
  echo "GLYPHLINGS_FAIL banned call"
  exit 1
fi
if grep -nE 'http-post|std/llm' aura/*.aura | grep -v 'aura/coach.aura'; then
  echo "GLYPHLINGS_FAIL llm outside coach"
  exit 1
fi
if ! grep -q 'std/llm' aura/coach.aura; then
  echo "GLYPHLINGS_FAIL coach missing llm"
  exit 1
fi
if ! grep -q 'http-post' aura/coach.aura; then
  echo "GLYPHLINGS_FAIL coach missing post"
  exit 1
fi
rep=$(grep -n 'mutate:replace-value' aura/*.aura || true)
case "$rep" in
  *aura/driver.aura*) ;;
  *) echo "GLYPHLINGS_FAIL replace site"; exit 1 ;;
esac
echo "$rep" | grep -v 'aura/driver.aura' | grep -q 'mutate:replace-value' && {
  echo "GLYPHLINGS_FAIL replace outside driver"
  exit 1
}
if ! grep -q 'mutate:replace-value' aura/driver.aura; then
  echo "GLYPHLINGS_FAIL missing replace"
  exit 1
fi
if grep -n 'mutate:rebind' aura/*.aura; then
  echo "GLYPHLINGS_FAIL rebind in aura"
  exit 1
fi
sw=$(grep -n 'hot-strategy:swap!' aura/*.aura || true)
case "$sw" in
  *aura/spawn.aura*) ;;
  *) echo "GLYPHLINGS_FAIL swap site"; exit 1 ;;
esac
echo "$sw" | grep -v 'aura/spawn.aura' | grep -q 'hot-strategy:swap!' && {
  echo "GLYPHLINGS_FAIL swap outside spawn"
  exit 1
}
echo "GLYPHLINGS_OK"
