#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
arch=${ARCH:-$(uname -m)}
case "$arch" in
  arm64) hostarch=arm64; target= ;;
  x86_64) hostarch=amd64; target="-target x86_64-apple-macosx13.0" ;;
  *) echo "unsupported macOS architecture: $arch" >&2; exit 2 ;;
esac
output="$root/internal/hostbin/darwin_$hostarch/webview-host"

mkdir -p "$(dirname -- "$output")"
swiftc $target -O -framework AppKit -framework WebKit "$root/host/macos/main.swift" -o "$output"
codesign --force --sign - "$output"
