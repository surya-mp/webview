# webview

`webview` runs an existing Go web application in a chrome-free native window.
The public package is pure Go: it uses no CGo, native headers, or Go module
dependencies. It launches a prebuilt platform host beside the application
instead of compiling native UI code into every consuming app.

The host uses WKWebView on macOS, WebView2 on Windows, and WebKitGTK on Linux.
It is not Safari, Edge, or Firefox as an application; it is a small native
window around each operating system's browser engine.

## Install

```sh
go get github.com/surya-mp/webview
```

On macOS (Apple silicon and Intel), the matching host is embedded and extracted
automatically, so `go run` needs no flags, host path, or setup command. A release embeds the
matching host artifact for each supported target. External hosts remain an
optional override and are found in this order: `Options.HostPath`, beside the
application executable, then on `PATH`.

## Quick start

```go
package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"

	"github.com/surya-mp/webview"
)

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte("<h1>Same web app, native shell</h1>"))
	})

	err := webview.Run(ctx, webview.Options{Title: "Example", Handler: mux})
	if err != nil && !errors.Is(err, context.Canceled) {
		log.Fatal(err)
	}
}
```

`Handler` starts a loopback-only server. Existing absolute and relative routes
continue to work. See [examples/local](examples/local) for a runnable version.

For a deployed app, use `StartURL` instead of `Handler`:

```go
err := webview.Run(ctx, webview.Options{
	Title:    "Example",
	StartURL: "https://app.example.com/",
})
```

## Host distribution

Build hosts only in this repository's release pipeline; consuming applications
do not need native compilers, headers, or host-launch commands. Both macOS
artifacts are embedded in the module for local `go run` development. Build the
other platform artifacts on their target systems before their release.

```sh
# macOS arm64 or x86_64, on the matching Mac
scripts/build-host-macos.sh

# Linux, on the target distribution
scripts/build-host-linux.sh
```

On Windows, run `scripts/build-host-windows.ps1` from a Visual Studio developer
shell with `WEBVIEW2_INCLUDE` pointing to the WebView2 SDK include directory.
The build scripts produce the artifact that is embedded for its matching
platform release. `HostPath` is only for development overrides. Do not download
a host at application runtime; ship a versioned, checksummed artifact with the
application instead.

## Behavior and boundaries

- The host window has no address bar, browser tabs, or browser toolbar.
- Same-origin navigation and popups remain in the host. Another origin opens in
  the operating system's default browser.
- The macOS host provides native JavaScript dialogs and file-selection panels.
- Closing the host window ends `Run` and shuts down a handler-backed server.
- Static Go-defined menus, Go/JavaScript bridges, and app-level permission
  policy are not implemented by the sidecar protocol.

The platform browser runtime remains an operating-system dependency: macOS
supplies WebKit, Windows requires the WebView2 Runtime, and Linux requires
WebKitGTK. The full contract is in [WEBVIEW-SPEC.md](WEBVIEW-SPEC.md).

## Development

```sh
CGO_ENABLED=0 go test ./...
CGO_ENABLED=0 go vet ./...
```

Before release, follow the [platform verification runbook](docs/operations.md)
and [release checklist](docs/release.md).

## License

[MIT](LICENSE).
