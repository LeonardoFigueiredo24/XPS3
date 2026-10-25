#!/bin/sh
# Fetch the external source trees used by the reproducible XPS3 UWP build.
# Usage: xps3-fetch-sources.sh [destination]
set -eu

destination=${1:-/build/sources}
mkdir -p "$destination"

fetch_locked() {
    name=$1
    url=$2
    ref=$3
    expected=$4
    checkout="$destination/$name"
    created=0

    if [ ! -d "$checkout/.git" ]; then
        if [ -e "$checkout" ]; then
            echo "$checkout exists but is not a Git checkout; refusing to overwrite it" >&2
            exit 1
        fi
        git clone --filter=blob:none --no-checkout "$url" "$checkout"
        git -C "$checkout" config xps3.fetchIncomplete true
        created=1
    fi

    actual_url=$(git -C "$checkout" remote get-url origin)
    if [ "$actual_url" != "$url" ]; then
        echo "$name origin mismatch: $actual_url" >&2
        exit 1
    fi

    git -C "$checkout" fetch --depth 1 origin "$ref"
    actual=$(git -C "$checkout" rev-parse FETCH_HEAD^{commit})
    if [ "$actual" != "$expected" ]; then
        echo "$name revision mismatch: expected $expected, got $actual" >&2
        exit 1
    fi
    incomplete=$(git -C "$checkout" config --bool --get xps3.fetchIncomplete || echo false)
    if [ "$created" -eq 1 ] || [ "$incomplete" = true ]; then
        git -C "$checkout" checkout --detach --force "$expected"
        git -C "$checkout" config --unset xps3.fetchIncomplete
    else
        head=$(git -C "$checkout" rev-parse HEAD^{commit})
        if [ "$head" != "$expected" ]; then
            echo "$name checkout is at $head, expected $expected; refusing to overwrite it" >&2
            exit 1
        fi
        if [ -n "$(git -C "$checkout" status --porcelain)" ]; then
            echo "$name checkout has local changes; refusing to overwrite them" >&2
            exit 1
        fi
    fi
    printf '%s %s %s\n' "$name" "$ref" "$actual"
}

fetch_locked \
    ffmpeg \
    https://github.com/FFmpeg/FFmpeg.git \
    refs/tags/n8.1 \
    9047fa1b084f76b1b4d065af2d743df1b40dfb56

fetch_locked \
    mesa \
    https://gitlab.freedesktop.org/mesa/mesa.git \
    refs/tags/mesa-26.2.5 \
    ddaf85e353442d9d7f192fdcf1823e652e3f2ee3

fetch_locked \
    Vulkan-Headers \
    https://github.com/KhronosGroup/Vulkan-Headers.git \
    refs/tags/v1.4.335 \
    2fa203425eb4af9dfc6b03f97ef72b0b5bcb8350

fetch_locked \
    msix-packaging \
    https://github.com/microsoft/msix-packaging.git \
    refs/heads/master \
    25a65f5c1690930813bcc10cdf1d59fa865f2bb1

echo "XPS3_SOURCE_FETCH=PASS"
