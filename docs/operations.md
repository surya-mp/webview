# Platform verification runbook

Run the package checks before a target-platform test:

```sh
go test ./...
go vet ./...
```

## macOS

The module packages arm64 and Intel hosts. Run the local example directly:

```sh
go run ./examples/local
```

Verify the initial dimensions and title, internal route navigation, external
link handoff, `window.open`, file input, JavaScript dialogs, `window.print`,
and window close. Build a release host from `host/macos/main.swift` with
`scripts/build-host-macos.sh`, then sign the shipped artifact with the
application's Developer ID identity.

## Linux

On the target distribution, install GTK3 and WebKitGTK 4.1 development/runtime
packages. Build the host and place the resulting `webview-host` beside the
application executable or configure `HostPath` while testing.

```sh
scripts/build-host-linux.sh
```

Verify window creation, routing, popup handling, uploads, downloads, printing,
permissions, and shutdown. Record the distribution, desktop session, and
package versions used for the release artifact.

## Windows

Use a Visual Studio developer shell with the WebView2 SDK and Runtime installed.
Set `WEBVIEW2_INCLUDE` to the SDK include directory, then build the host:

```powershell
.\scripts\build-host-windows.ps1
```

Place `webview-host.exe` and `WebView2Loader.dll` beside the application
executable for testing. Verify window creation, routing, popups, uploads,
downloads, printing, permissions, and shutdown. Record Windows, WebView2
Runtime, and SDK versions.

## Failure triage

- `ErrHostNotFound`: package a target host beside the executable or provide
  `Options.HostPath`.
- Host exits immediately: run the host with the startup arguments shown in the
  specification to identify a missing runtime or loader.
- Blank page: open the target URL outside the host and verify the application
  response before investigating host behavior.
