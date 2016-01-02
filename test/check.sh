#!/bin/bash

set -e
RED=$'\033[31m'
GREEN=$'\033[32m'
NORM=$'\033[m'

function reset(){
	rm -f tmp*
}

function green(){ echo "$GREEN$*$NORM"; }
function red(){ echo "$RED$*$NORM"; }
function pass(){ green pass; reset; }
function xfail(){ green xfail; reset; }
function fail(){ red fail; exit 1; }
function xpass(){ red xpass; exit 1; }

trap "reset" EXIT

# ensure it fails when no additional data is detected
echo -n "[simple] "
if ../jpegsplit simple.jpg 2> /dev/null; then
	xpass
fi
xfail

# ensure it succeeds in detecting two concaternated jpegs
echo -n "[jpg_jpg detect] "
if ! ../jpegsplit jpg_jpg.jpg 2> /dev/null; then
	fail
fi
pass

# ensure it succeeds in extracting all parts
echo -n "[jpg_png_txt extract] "
if ! ../jpegsplit jpg_png_txt.jpg tmp.jpg tmp.png tmp.txt 2> /dev/null; then
	fail
fi
if ! md5sum --quiet -c jpg_png_txt.md5; then
	fail
fi
pass

# validate detected types
echo -n "[jpg_jpg_txt datatypes] "
if ! diff -u <(../jpegsplit jpg_png_txt.jpg 2>&1 | sed -n "s/.*\`\(.*\)'.*/\1/p") <(echo -e "image/jpeg\nimage/png\ntext/plain"); then
	fail
fi
pass
