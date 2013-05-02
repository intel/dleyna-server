#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# Derived from https://git.gnome.org/browse/glib/tree/autogen.sh

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

olddir=`pwd`
cd "$srcdir"

AUTORECONF=`which autoreconf`
if test -z $AUTORECONF; then
        echo "*** No autoreconf found, please install it ***"
        exit 1
fi

autoreconf --force --install --verbose || exit $?

cd "$olddir"
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
