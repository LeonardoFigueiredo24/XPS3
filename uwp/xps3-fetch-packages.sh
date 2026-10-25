#!/bin/sh
# Download and verify the Microsoft redistributable packages used by XPS3 UWP.
# Usage: xps3-fetch-packages.sh [destination]
set -eu

destination=${1:-/build/packages}
mkdir -p "$destination"

fetch_locked() {
    filename=$1
    url=$2
    expected=$3
    output="$destination/$filename"

    if [ ! -f "$output" ]; then
        partial="$output.partial"
        curl --fail --location --retry 3 --output "$partial" "$url"
        actual=$(sha256sum "$partial" | awk '{print $1}')
        if [ "$actual" != "$expected" ]; then
            echo "$filename SHA-256 mismatch: expected $expected, got $actual" >&2
            exit 1
        fi
        mv "$partial" "$output"
    fi

    actual=$(sha256sum "$output" | awk '{print $1}')
    if [ "$actual" != "$expected" ]; then
        echo "$filename SHA-256 mismatch: expected $expected, got $actual" >&2
        exit 1
    fi
    printf '%s %s\n' "$actual" "$filename"
}

dxc=microsoft.direct3d.dxc.1.9.2609.5.nupkg
xaml=microsoft.ui.xaml.2.8.7.nupkg

fetch_locked \
    "$dxc" \
    https://api.nuget.org/v3-flatcontainer/microsoft.direct3d.dxc/1.9.2609.5/microsoft.direct3d.dxc.1.9.2609.5.nupkg \
    73cfa082245918c2cc1ec6740250eb7377e83a757b3b3a3e16e0d71535bfbfc3

fetch_locked \
    "$xaml" \
    https://api.nuget.org/v3-flatcontainer/microsoft.ui.xaml/2.8.7/microsoft.ui.xaml.2.8.7.nupkg \
    79207b10fe243eb1a8dcdc29beceba2f472f145ed31f147f7ae3f43b0659c9f7

mkdir -p "$destination/dxc-1.9.2609.5" "$destination/xaml-2.8.7"
unzip -oq "$destination/$dxc" \
    build/native/bin/x64/dxil.dll \
    build/native/bin/x64/dxcompiler.dll \
    LICENSE-LLVM.txt \
    LICENSE-MS.txt \
    -d "$destination/dxc-1.9.2609.5"
unzip -oq "$destination/$xaml" \
    tools/AppX/x64/Release/Microsoft.UI.Xaml.2.8.appx \
    license.txt \
    -d "$destination/xaml-2.8.7"

echo "XPS3_PACKAGE_FETCH=PASS"
