# Platform verification runbook

Run `go test ./...` and `go vet ./...` before every platform check.

## macOS

Build with Xcode command-line tools. Open the local example and verify the
window, global menu bar, internal/external links, `window.open`, file input,
download destination, JavaScript dialogs, print, and close behavior. Bundle
`NSCameraUsageDescription` and `NSMicrophoneUsageDescription` before testing
camera or microphone access.

## Linux

Install GTK3 and WebKitGTK 4.1 development/runtime packages, then run:

```sh
CGO_ENABLED=1 go run ./examples/local
```

Verify internal/external links, popup routing, print, file selection, download,
and window close. Record the distro, package versions, and desktop session in
the release issue.

## Windows

Install a supported C++ toolchain, WebView2 SDK headers, `WebView2Loader.dll`,
and the Evergreen WebView2 Runtime. Then run:

```powershell
$env:CGO_ENABLED = "1"
go run ./examples/local
```

Verify window creation, internal/external links, popup routing, print, file
selection, download, permission prompts, menus, and close behavior. Record
the Windows and WebView2 Runtime versions.

## Failure triage

- Missing Linux libraries: confirm `pkg-config --modversion webkit2gtk-4.1`.
- Missing WebView2 loader/runtime: confirm `WebView2Loader.dll` is beside the
  executable or on `PATH`, then repair/install the Evergreen Runtime.
- Blank page: capture the start URL and navigation-error callback details;
  first verify the handler or remote URL outside the shell.
