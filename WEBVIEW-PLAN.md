# webview development plan

## Implemented

1. Keep the public package pure Go and buildable with `CGO_ENABLED=0`.
2. Start handler applications on a private loopback server.
3. Locate and run a versioned native sidecar host, passing only startup URL and
   window metadata through command-line arguments.
4. End `Run` when the host exits or its context is cancelled.
5. Provide native-host sources and release build scripts for macOS, Windows,
   and Linux without making them dependencies of consuming Go builds.

## Release work

1. Build each host on its target OS/architecture.
2. Code-sign macOS and Windows artifacts; publish checksums.
3. Package the correct artifact next to each consuming application executable.
4. Complete native UI checks for navigation, popups, uploads, downloads,
   printing, permissions, and close behavior.

## Boundaries

The sidecar is deliberate: it is the smallest way to use system browser
engines while keeping application builds CGo-free. Do not replace it with a
runtime download, a bundled Chromium distribution, or a Go/JavaScript bridge
without a concrete product need.
