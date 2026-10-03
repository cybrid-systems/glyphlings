#!/bin/sh
set -e
cd "$(dirname "$0")/.."
if grep -nE '\b(cat|dog|bee|sun|fox|pig|hen|ant|zoo|scene|score)\b' display/*.c; then
  echo "GLYPHLINGS_FAIL display word"
  exit 1
fi
if grep -nE '\(eval |\(load |\(shell |http-get|http-post|std/llm|std/net|pheromone:init|security:grant-capability!' aura/*.aura; then
  echo "GLYPHLINGS_FAIL banned call"
  exit 1
fi
reb=$(grep -n 'mutate:rebind' aura/*.aura || true)
case "$reb" in
  *aura/driver.aura*) ;;
  *) echo "GLYPHLINGS_FAIL rebind site"; exit 1 ;;
esac
echo "$reb" | grep -v 'aura/driver.aura' | grep -q 'mutate:rebind' && {
  echo "GLYPHLINGS_FAIL rebind outside driver"
  exit 1
}
if ! grep -q 'mutate:rebind' aura/driver.aura; then
  echo "GLYPHLINGS_FAIL missing rebind"
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
