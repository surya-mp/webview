# Platform verification runbook

Run the package checks before a target-platform test:

```sh
go test ./...
go vet ./...
```

Every host accepts the following launch contract. It is useful for isolated
smoke testing when an application executable is not yet available:

```text
webview-host --url <http-or-https-url> --title <title> --width <pixels> --height <pixels>
```

An invalid URL must exit with status `2` before attempting to create a window.

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

The script writes `webview-host` to `dist/linux/amd64` or `dist/linux/arm64`.
Verify window creation, routing (including case and explicit default ports),
same-origin popup reuse, uploads, downloads, printing, permissions, and
shutdown. Record the distribution, desktop session, and package versions used
for the release artifact.

For distribution, place the host beside the application and ensure it remains
executable:

```text
my-app
webview-host
```

Use `HostPath` if the application and host cannot be placed together. The
target system must provide GTK3 and WebKitGTK 4.1 runtime libraries compatible
with the build environment.

## Windows

Use a Visual Studio x64 developer shell with the WebView2 Runtime installed.
Download the `Microsoft.Web.WebView2` NuGet package, set `WEBVIEW2_SDK` to its
package directory, then build the host:

```powershell
.\scripts\build-host-windows.ps1
```

The script writes `webview-host.exe` and `WebView2Loader.dll` to
`dist\windows\amd64`. Place both beside the application executable for testing.
Verify window creation, routing (including case and explicit default ports),
popups, uploads, downloads, printing, permissions, and shutdown. Record
Windows, WebView2 Runtime, and SDK versions.

For distribution, keep the host and loader DLL together with the application:

```text
my-app.exe
webview-host.exe
WebView2Loader.dll
```

`WebView2Loader.dll` is required even when the Evergreen WebView2 Runtime is
installed; the loader locates that runtime for the host process.

## Failure triage

- `ErrHostNotFound`: package a target host beside the executable or provide
  `Options.HostPath`.
- Host exits immediately: run the host with the startup arguments shown in the
  specification to identify a missing runtime or loader.
- Blank page: open the target URL outside the host and verify the application
  response before investigating host behavior.
- External link remains in the host: confirm the target differs by scheme,
  host, or effective port. A same-origin popup is intentionally reused in the
  existing window.
- Windows host cannot start: keep `WebView2Loader.dll` next to the host and
  install the Evergreen WebView2 Runtime.
- Linux host cannot start: verify the target has compatible GTK3 and
  WebKitGTK 4.1 runtime libraries, then run the host from a terminal to see
  loader errors.
