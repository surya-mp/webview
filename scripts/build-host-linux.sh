#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
case $(uname -m) in
  x86_64|amd64) arch=amd64 ;;
  aarch64|arm64) arch=arm64 ;;
  *) echo "unsupported Linux architecture: $(uname -m)" >&2; exit 2 ;;
esac

if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists webkit2gtk-4.1; then
  echo "WebKitGTK 4.1 development files are required (pkg-config package: webkit2gtk-4.1)." >&2
  exit 2
fi

output="$root/dist/linux/$arch/webview-host"
mkdir -p "$(dirname -- "$output")"
cc -std=c11 -O2 -Wall -Wextra -Werror ${CFLAGS:-} \
  "$root/host/linux/main.c" \
  $(pkg-config --cflags --libs webkit2gtk-4.1) ${LDFLAGS:-} \
  -o "$output"
