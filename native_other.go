//go:build (!darwin && !linux && !windows) || (linux && !cgo) || (windows && !cgo)

package webview

func newNativeWindow(options options, url, allowedOrigin string, onMenu func(uintptr), onNavigationError func(NavigationError), permission func(PermissionRequest) PermissionDecision) (nativeWindow, error) {
	return nil, ErrUnsupported
}
