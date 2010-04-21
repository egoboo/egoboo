#!/bin/sh

EGOBOO_PREFIX=${HOME}/.local

# exit on any error
set -e

exec ${EGOBOO_PREFIX}/bin/egoboo-2.x "$@"
