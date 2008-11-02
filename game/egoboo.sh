#!/bin/sh

EGOBOO_PREFIX=${HOME}/.local
EGOBOO_SHARED="${EGOBOO_PREFIX}/share/egoboo"

# exit on any error
set -e

if [ ! -d ~/.egoboo ]; then
  mkdir ~/.egoboo
fi

if [ ! -f ~/.egoboo/setup.txt ]; then
    cp -a "${EGOBOO_SHARED}/setup.txt" ~/.egoboo
fi

if [ ! -f ~/.egoboo/controls.txt ]; then
    cp -a "${EGOBOO_SHARED}/controls.txt" ~/.egoboo
fi

if [ ! -d ~/.egoboo/players ]; then
    cp -a "${EGOBOO_SHARED}/players" ~/.egoboo
fi

if [ ! -d ~/.egoboo/basicdat ]; then
    ln -s "${EGOBOO_SHARED}/basicdat" ~/.egoboo
fi

if [ ! -d ~/.egoboo/modules ]; then
    ln -s "${EGOBOO_SHARED}/modules" ~/.egoboo
fi

cd ~/.egoboo

exec "${EGOBOO_PREFIX}/libexec/egoboo" "$@"
