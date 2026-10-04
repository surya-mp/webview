# webview

`webview` gives an existing web application a native desktop window. Your app
continues to own its routes, HTML, CSS, JavaScript, assets, and HTTP handler;
this package supplies the window that displays it.

On macOS, the window uses WebKit through `WKWebView`. The project also contains
native hosts for Windows/WebView2 and Linux/WebKitGTK. The window is an app
shell, not a full browser: it has no address bar, tabs, or browser toolbar.

## Quick start

Use an existing `http.Handler`. `Run` starts it on a private loopback address,
opens the host window, and returns when the window closes.

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
		_, _ = w.Write([]byte(`<!doctype html><h1>Hello desktop</h1>`))
	})

	err := webview.Run(ctx, webview.Options{
		Title:   "Hello",
		Width:   1100,
		Height:  750,
		Handler: mux,
	})
	if err != nil && !errors.Is(err, context.Canceled) {
		log.Fatal(err)
	}
}
```

On macOS, `go run .` works directly: the matching host is packaged with the
module and is extracted for the process automatically. See
[examples/local](examples/local) for a runnable application.

## Hosted application

Use `StartURL` when the web app already runs elsewhere. Do not set `Handler`
in this mode.

```go
err := webview.Run(ctx, webview.Options{
	Title:    "Account",
	Width:    1200,
	Height:   800,
	StartURL: "https://app.example.com/",
})
```

See [examples/remote](examples/remote).

## What `Run` does

1. Validates the options.
2. In `Handler` mode, binds the handler to `127.0.0.1` on an ephemeral port.
3. Starts the native host with the selected URL, title, and dimensions.
4. Waits for the host window to close or for the context to be cancelled.
5. Stops the loopback server before returning.

The host window is a single window. `Run` returns `nil` after an ordinary
window close, returns `ctx.Err()` after cancellation, and returns an error for
invalid options or host startup/exit failures.

## Options

| Field | Meaning |
| --- | --- |
| `Title` | Window title. Defaults to the executable name. |
| `Width`, `Height` | Initial window size. Each defaults to `1024` and `768`. |
| `Handler` | The app's HTTP handler. Mutually exclusive with `StartURL`. |
| `StartURL` | Absolute `http` or `https` URL to open. Mutually exclusive with `Handler`. |
| `StartPath` | Initial absolute route in `Handler` mode; defaults to `/`. |
| `HostPath` | Optional path to a specific native host executable for development or a custom distribution. |

## Navigation, popups, and browser features

The host treats scheme, host, and effective port as an origin:

- A same-origin navigation remains in the app window.
- A same-origin `window.open()` or `target="_blank"` navigation is loaded in
  that same window.
- A different origin is opened by the system's default browser.

The host relies on the operating system's browser engine for web-platform
behavior. On macOS, file inputs open native file panels and JavaScript
`alert`, `confirm`, and `prompt` display native dialogs. Printing, downloads,
permissions, and other browser features are governed by the platform engine
and should be verified for each release target.

There is no Go/JavaScript bridge, Go-defined menu API, tab API, or runtime
window mutation API. Keep application behavior in the web app itself.

## Host packaging

The host is a small executable started by the Go application. `HostPath` is an
override. Without it, the package searches for a host next to the application
executable, then on `PATH`, before using a packaged host when available.

macOS Apple-silicon and Intel hosts are packaged for normal `go run` and
application builds. Build and package the matching Windows and Linux host on
its target platform before distributing an application there. The operational
steps are in [docs/operations.md](docs/operations.md).

## Platform support

| Platform | Host engine | Distribution status |
| --- | --- | --- |
| macOS arm64 / amd64 | WKWebView | Packaged host; source and host compilation verified. |
| Windows | WebView2 | Host source available; build and runtime verification required on Windows. |
| Linux | WebKitGTK | Host source available; build and runtime verification required on the target distribution. |

## Development and release

```sh
go test ./...
go vet ./...
```

Use the [operations runbook](docs/operations.md) for platform checks and the
[release checklist](docs/release.md) when publishing a version.

## License

[MIT](LICENSE).
