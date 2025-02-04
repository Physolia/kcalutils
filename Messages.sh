#! /bin/sh
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
$EXTRACT_GRANTLEE_TEMPLATE_STRINGS `find src/templates -name \*.html` >> rc.cpp
$EXTRACTRC *.kcfg *.ui >> rc.cpp
$XGETTEXT rc.cpp src/*.cpp src/*.h -o $podir/libkcalutils5.pot
rm -f rc.cpp

