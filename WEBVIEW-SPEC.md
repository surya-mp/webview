# webview specification

## 1. Purpose

`webview` is a reusable Go package that starts a desktop application inside
one native browser-engine window on macOS, Windows, and Linux.  It gives a Go
application a single `Run` entry point: the package starts the application's
local HTTP handler, opens the window to it, and blocks until the window closes.

The package is a thin desktop shell, not a web framework.  The application
owns its HTTP handler, assets, routes, state, and UI.

## 2. Supported runtime model

`Run(ctx, Options)` MUST:

1. Validate `Options` before creating a listener or native window.
2. When `Handler` is supplied, start an HTTP server bound only to the loopback
   interface on an ephemeral port and serve that handler. When `StartURL` is
   supplied, start no server.
3. Create one visible native webview window and navigate it to the selected
   local URL (including `StartPath`, if supplied) or `StartURL`.
4. Run the native event loop on the platform-required UI thread.
5. Stop the server and release native resources when the user closes the
   window, the context is cancelled, or setup fails.

The initial page is deliberately empty from the shell's perspective: there is
no address bar, navigation chrome, or embedded developer tools.  The
application handler or start URL supplies all visible web content. The package
MUST use the operating system's installed webview engine: WKWebView on macOS,
WebView2 on Windows, and WebKitGTK on Linux. It MUST NOT bundle a browser
runtime.

## 3. Public API (v0)

The initial public surface is intentionally small:

```go
package webview

type Options struct {
	Title     string
	Width     int
	Height    int
	Handler   http.Handler
	StartURL  string
	StartPath string
	Menu      []MenuItem
}

type MenuItem struct {
	ID       string
	Label    string
	Shortcut string
	Disabled bool
	Children []MenuItem
	Action   func(context.Context) error
}

func Run(ctx context.Context, options Options) error
```

Exactly one of `Handler` and `StartURL` is required. `Handler` serves the
application from a private loopback server. `StartURL` loads an existing
absolute HTTP(S) application URL. `Title` defaults to the executable name.
`Width` and `Height` default to 1024 and 768 CSS-independent window units and
MUST be positive when provided. `StartPath` defaults to `/`, MUST begin with
`/`, and is available only with `Handler`. `ID` and `Label` are required
for every menu item; IDs are unique across the complete menu tree.  A menu
item with `Children` is a submenu and MUST NOT also have `Action`.

`Run` is synchronous.  It returns `nil` after ordinary user-window closure;
it returns an error for invalid options, listener/window creation failure,
navigation failure, or an action failure.  It returns `ctx.Err()` if context
cancellation initiates shutdown before ordinary closure.  Calling `Run` more
than once concurrently is unsupported in v0 and MUST return a documented
error rather than create competing UI event loops.

The package MUST NOT expose the underlying webview implementation in its
public API.  A backend-neutral package API keeps an implementation replacement
possible without breaking applications.

## 4. Menus

`Options.Menu` is an ordered top-level menu model.  Separators are deferred
from v0; applications can use submenus and command items only.

On macOS, the model MUST be installed as a native application menu in the
system menu bar (the top panel), in the declared order.  The OS-supplied
application menu remains available; the package MUST NOT replace standard
Quit behavior.  Native keyboard-equivalent syntax is mapped to macOS menu
shortcuts where representable.

On Windows and Linux, the same model MUST appear as a menu bar inside the
application window.  A platform that has no usable native in-window menu API
MAY render an accessible HTML menu overlay instead, but activation and
ordering must remain equivalent.

Selecting an enabled command schedules its `Action` without blocking the UI
event loop.  A second activation of the same command while it is running is
ignored.  Menu mutation after `Run` starts is out of scope for v0.  `Disabled`
items are visibly disabled and never invoke `Action`.  An action error causes
orderly application shutdown and is returned by `Run`.

## 5. Lifecycle and safety

- The loopback server MUST listen on `127.0.0.1` and/or `::1`, never an
  externally reachable interface.
- A handler-backed application server preserves application routes unchanged,
  so absolute and relative links work identically in browser and desktop modes.
- `Run` MUST stop accepting requests before returning and wait for in-flight
  handler requests to finish or for their request contexts to end.
- The package MUST prevent external navigation from silently replacing the
  application. Same-origin navigations, including `target="_blank"` and
  `window.open`, load in the original shell window. A different origin opens
  in the operating system's default browser. Origin means scheme, host, and
  effective port.
- All native calls occur on the UI thread. Handler calls and menu actions run
  away from that thread.
- Startup and shutdown must be idempotent internally so an action error,
  context cancellation, and a close event racing together release each
  resource once and return one deterministic terminal result.

## 6. Platform support and dependencies

The first implementation targets the latest two supported releases of macOS,
Windows 10/11 with WebView2 Runtime, and mainstream Linux distributions with
WebKitGTK 4.1 installed.  A missing platform runtime is a clear startup error
that names the required runtime; it is not silently replaced by a different
renderer.

The package MUST own its platform backends; it MUST NOT wrap another Go
webview package. Platform-specific window and menu code is isolated behind
internal build-tagged files. The exported package is Go-only, but native
backends may call the operating system frameworks directly (and use cgo where
that is the platform-native mechanism). Cross-compilation follows each
platform backend's documented toolchain requirements.

## 7. Non-goals for v0

- A bundled browser engine, tabs, address bar, or browser-like navigation UI.
- A frontend framework, asset compiler, hot reload, or JavaScript/Go RPC API.
- Multiple windows, tray integration, modal dialogs, automatic updates, or
  runtime menu changes.
- Sandboxing arbitrary untrusted web content.

## 8. Acceptance criteria

1. A Go program with only a handler and `webview.Run` launches one application
   window and serves its handler on macOS, Windows, and Linux.
2. The window shows the handler's `/` response and has no browser chrome.
3. On macOS, configured top-level menus appear in the system menu bar; on
   Windows/Linux they appear in the app window.
4. Clicking an enabled command invokes its Go action once; disabled commands
   do not invoke theirs.
5. Closing the window ends `Run` and frees the loopback server port.
6. Invalid options, a cancelled context, an unavailable webview runtime, and
   a command action error return actionable errors without resource leaks.
