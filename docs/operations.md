# Platform verification runbook

Run `CGO_ENABLED=0 go test ./...` and `CGO_ENABLED=0 go vet ./...` before
testing any target platform.

## macOS

Build the host on each target architecture:

```sh
scripts/build-host-macos.sh
go run ./examples/local
```

Verify the chrome-free WKWebView window, internal navigation, external-link
handoff, popup routing, file input, JavaScript dialogs, print behavior, and
window close. Sign the final host with the application's Developer ID identity
rather than the script's ad-hoc signature.

## Linux

On the target distribution, install GTK3 and WebKitGTK 4.1 development/runtime
packages, then build and test:

```sh
scripts/build-host-linux.sh
go run ./examples/local
```

Record the distribution, desktop session, and package versions.

## Windows

In a Visual Studio developer shell, install the WebView2 SDK and Runtime, set
`WEBVIEW2_INCLUDE` to the SDK include directory, then run:

```powershell
.\scripts\build-host-windows.ps1
go run .\examples\local
```

Ship `WebView2Loader.dll` beside `webview-host.exe` when using the dynamic
loader implementation. Verify window creation, external-link handoff, popup
routing, uploads, downloads, printing, permissions, and close behavior.

## Failure triage

- `ErrHostNotFound`: put the matching host beside the application or set
  `Options.HostPath`.
- Host exits immediately: run it from a terminal with the same arguments to
  identify a missing platform runtime or loader.
- Blank page: open the supplied URL outside the host and verify the handler or
  remote deployment first.
