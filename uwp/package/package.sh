#!/bin/sh
# Build the signed .appx from a finished build.
# Usage: package.sh <build dir> <output .appx> <cert.pfx> <pfx password>
# Needs makemsix (github.com/microsoft/msix-packaging, MAKEMSIX env var) and osslsigncode.
set -e
build=$1; out=$2; pfx=$3; pass=$4
here=$(dirname "$(realpath "$0")")
src=$(realpath "$here/../..")
layout=$build/appx-layout
python3 "$here/make_layout.py" "$build/bin/rpcs3_uwp.exe" "$src" "$layout"
rm -f "$build/rpcs3_uwp_unsigned.appx"
"${MAKEMSIX:-makemsix}" pack -d "$layout" -p "$build/rpcs3_uwp_unsigned.appx"
osslsigncode sign -pkcs12 "$pfx" -pass "$pass" -h sha256 -in "$build/rpcs3_uwp_unsigned.appx" -out "$out"
