#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
arch=$(uname -m)
output="$root/dist/linux/$arch/webview-host"

mkdir -p "$(dirname -- "$output")"
cc -O2 "$root/host/linux/main.c" $(pkg-config --cflags --libs webkit2gtk-4.1) -o "$output"
