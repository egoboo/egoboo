#!/bin/sh

EGOBOO_PREFIX=${HOME}/.local
EGOBOO_SHARED="${EGOBOO_PREFIX}/share/egoboo"

# exit on any error
set -e

if [ ! -d ~/.egoboo-2.x ]; then
  mkdir ~/.egoboo-2.x
fi

if [ ! -f ~/.egoboo-2.x/setup.txt ]; then
    cp -a "${EGOBOO_SHARED}/setup.txt" ~/.egoboo-2.x
fi

if [ ! -f ~/.egoboo-2.x/controls.txt ]; then
    cp -a "${EGOBOO_SHARED}/controls.txt" ~/.egoboo-2.x
fi

if [ ! -d ~/.egoboo-2.x/players ]; then
    cp -a "${EGOBOO_SHARED}/players" ~/.egoboo-2.x
fi

if [ ! -d ~/.egoboo-2.x/basicdat ]; then
    ln -s "${EGOBOO_SHARED}/basicdat" ~/.egoboo-2.x
fi

if [ ! -d ~/.egoboo-2.x/modules ]; then
    ln -s "${EGOBOO_SHARED}/modules" ~/.egoboo-2.x
fi

exec /usr/games/egoboo-2.x "$@"
