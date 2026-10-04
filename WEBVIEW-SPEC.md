# webview specification

## Purpose

`webview` runs an existing web application in one native desktop window. It is
a window host, not a web framework: callers keep their existing HTTP handler
or deployed URL, frontend, routes, and application state.

## API

```go
type Options struct {
	Title     string
	Width     int
	Height    int
	Handler   http.Handler
	StartURL  string
	StartPath string
	HostPath  string
}

func Run(context.Context, Options) error
```

Exactly one of `Handler` and `StartURL` is required. `StartURL` must be an
absolute HTTP(S) URL. `StartPath` applies only to `Handler`, must be a clean
absolute path, and defaults to `/`. Width and height must not be negative and
default independently to 1024 and 768. Title defaults to the executable name.

`HostPath` overrides normal host discovery. It must point to a compatible host
executable that accepts the documented startup arguments.

## Lifecycle

`Run` validates before starting any resources. In handler mode it starts a
loopback-only HTTP server, then starts the host with:

```text
--url <url> --title <title> --width <width> --height <height>
```

It waits for the host process. An ordinary host exit returns `nil`. Cancelling
the supplied context terminates the host and returns `ctx.Err()`. A
handler-backed server is shut down before `Run` returns.

## Navigation policy

The intended policy across hosts is one window and one trusted origin. A URL
with the initial scheme, host, and effective port stays in the host. A URL with
another origin is handed to the operating system's default browser. New-window
requests follow the same rule.

## Native host behavior

Hosts are responsible for native window creation and web-engine integration.
The macOS host creates a resizable, titled WKWebView window with a minimal app
menu. It routes JavaScript dialogs and file selection to native panels.

The Windows and Linux host sources provide the same one-window launch and
navigation model using WebView2 and WebKitGTK respectively. They require
target-system build and UI verification before release.

## Deliberate limits

- One window per `Run` invocation; no tabs or address bar.
- No Go/JavaScript bridge or page scripting API.
- No Go-defined application menus or dynamic window controls.
- No built-in updater, host downloader, or application sandbox.
