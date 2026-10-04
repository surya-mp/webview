# webview specification

## Purpose

`webview` is a pure-Go package that starts a web application in a prebuilt
native sidecar host. The application code compiles with `CGO_ENABLED=0`; native
browser bindings are compiled once into separate release artifacts.

## Public API

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

Exactly one of `Handler` and `StartURL` is required. A handler is served only
on an ephemeral loopback listener. `StartURL` must be an absolute HTTP(S) URL.
`StartPath` is handler-only, defaults to `/`, and must be a clean absolute path.
`Title` defaults to the executable name; dimensions default to 1024×768.

`Run` validates options, starts the loopback server when needed, starts the
host with `--url`, `--title`, `--width`, and `--height`, then waits for the host
to exit. Context cancellation terminates the host process and returns
`ctx.Err()`.

## Host discovery and distribution

The matching host is embedded in the Go binary and extracted automatically at
runtime. `HostPath`, a host beside the application executable, and `PATH` are
optional overrides/fallbacks. Failure returns `ErrHostNotFound`.

Releases must distribute one code-signed/checksummed host executable for every
supported OS and architecture. The package must never compile a host or
download one at a consuming application's build or runtime.

## Platform behavior

| Platform | Host engine | Status |
| --- | --- | --- |
| macOS | WKWebView | Source builds on macOS; target UI verification required. |
| Windows | WebView2 | Source requires target build and WebView2 Runtime verification. |
| Linux | WebKitGTK | Source requires target build and GTK/WebKitGTK verification. |

The host has no browser chrome. Same-origin main-frame navigation stays in the
host; another origin opens in the default browser. Browser-engine dialogs and
permissions remain native-host concerns, not Go APIs.

## Non-goals

- A browser engine implemented in Go, a Safari process launcher, or CGo in the
  public Go package.
- Runtime host downloads, browser bundling, or automatic updates.
- Go/JavaScript bridges, Go-defined menus, dynamic host control, or multiple
  windows in v0.
