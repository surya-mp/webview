# webview development plan

## Current design

The package starts a separate native host process rather than placing platform
window code in every application executable. The Go side owns startup, the
loopback server, option validation, and host lifecycle. The host side owns the
desktop window and browser-engine callbacks.

## Implemented

1. Handler-backed and remote-URL startup modes.
2. Private loopback serving with route preservation.
3. Window title and size configuration.
4. Host discovery, packaged-host extraction, process startup, and cleanup.
5. macOS WKWebView host with navigation routing, popup handling, native file
   selection, and JavaScript dialogs.
6. Windows/WebView2 and Linux/WebKitGTK host source plus target build scripts.

## Release completion work

1. Build and verify the Windows host on supported Windows versions.
2. Build and verify the Linux host on each supported distribution.
3. Package the matching target host with every release artifact.
4. Validate file upload, downloads, print, media permissions, popup routing,
   external-link handling, and close behavior on each target.
5. Sign and checksum each distributed host artifact.

## Deferred features

Menus driven from Go, a Go/JavaScript bridge, multiple windows, custom
permission policy, runtime host updates, and automatic update delivery are
deferred until an application needs them.
