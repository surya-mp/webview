# webview

`webview` runs an existing web application inside one native desktop window.
The page, routes, assets, and UI remain the same; the package adds no browser
chrome and no frontend framework.

It owns platform backends directly—WKWebView on macOS, WebView2 on Windows,
and WebKitGTK on Linux. There are no third-party Go module dependencies.

## Install

```sh
go get github.com/surya-mp/webview
```

## Quick start: existing Go handler

```go
package main

import (
	"context"
	"log"
	"net/http"

	"github.com/surya-mp/webview"
)

func main() {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte("<h1>Same web app, native window</h1>"))
	})

	err := webview.Run(context.Background(), webview.Options{
		Title:   "Example",
		Handler: mux,
		Menu: []webview.MenuItem{{
			ID: "file", Label: "File", Children: []webview.MenuItem{{
				ID: "quit", Label: "Quit", Shortcut: "cmd+q",
				Action: func(context.Context) error { return nil },
			}},
		}},
	})
	if err != nil {
		log.Fatal(err)
	}
}
```

`Handler` starts a loopback-only server. Existing absolute and relative routes
continue to work. See [examples/local](examples/local) for a runnable version.

## Hosted application

Use `StartURL` when the application already runs elsewhere. It is mutually
exclusive with `Handler`.

```go
err := webview.Run(context.Background(), webview.Options{
	Title:    "Example",
	StartURL: "https://app.example.com/",
})
```

See [examples/remote](examples/remote).

## Navigation and native UI

- Same-origin navigations, `target="_blank"`, and `window.open()` remain in
  the one shell window.
- A different scheme, host, or port opens in the default system browser.
- macOS provides native menus, upload/save dialogs, JavaScript dialogs,
  downloads, print dialogs, permission prompts, and navigation errors.
- `App.Print` opens the platform print dialog. Receive the `App` in `OnReady`.
- `PermissionPolicy` can return `PermissionPrompt`, `PermissionAllow`, or
  `PermissionDeny` for camera, microphone, location, and motion requests.

## Platform prerequisites

| Platform | Build/runtime requirement | Status |
| --- | --- | --- |
| macOS | Xcode command-line tools; system Cocoa/WebKit | Builds and unit-tests in this repository; complete the UI runbook before release. |
| Linux | `CGO_ENABLED=1`, GTK3 and WebKitGTK 4.1 development/runtime libraries | Native backend requires target-system validation. |
| Windows | `CGO_ENABLED=1`, a C++ toolchain, WebView2 SDK header, `WebView2Loader.dll`, and Evergreen WebView2 Runtime | Native backend requires target-system validation. |

For Linux packages, see [operations](docs/operations.md). For macOS camera or
microphone access, the app bundle must include the relevant `Info.plist` usage
descriptions.

## Development

```sh
go test ./...
go vet ./...
```

Before release, follow the [platform verification runbook](docs/operations.md)
and [release checklist](docs/release.md).

## Compatibility and security

The shell intentionally supports one window and static startup menus. It does
not sandbox untrusted content. In `StartURL` mode, treat the loaded origin as
trusted application content. The full contract is in [WEBVIEW-SPEC.md](WEBVIEW-SPEC.md).

## License

[MIT](LICENSE).
