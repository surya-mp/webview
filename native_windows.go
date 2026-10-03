//go:build windows && cgo

package webview

/*
#cgo CXXFLAGS: -std=c++17
#cgo LDFLAGS: -lole32 -lshell32 -luser32
#include <stdlib.h>
typedef void* windows_webview;
windows_webview windows_webview_new(const char*, int, int, const char*, const char*);
int windows_webview_run(windows_webview);
void windows_webview_terminate(windows_webview);
void windows_webview_destroy(windows_webview);
void windows_webview_print(windows_webview);
*/
import "C"

import (
	"fmt"
	"unsafe"
)

type windowsWindow struct{ handle C.windows_webview }

func newNativeWindow(o options, url, allowedOrigin string, onMenu func(uintptr), onNavigationError func(NavigationError), permission func(PermissionRequest) PermissionDecision) (nativeWindow, error) {
	title, startURL, origin := C.CString(o.title), C.CString(url), C.CString(allowedOrigin)
	defer C.free(unsafe.Pointer(title))
	defer C.free(unsafe.Pointer(startURL))
	defer C.free(unsafe.Pointer(origin))
	handle := C.windows_webview_new(title, C.int(o.width), C.int(o.height), startURL, origin)
	if handle == nil {
		return nil, ErrUnsupported
	}
	return &windowsWindow{handle: handle}, nil
}

func (w *windowsWindow) Run() error {
	if C.windows_webview_run(w.handle) != 0 {
		return fmt.Errorf("webview: WebView2 startup failed")
	}
	return nil
}
func (w *windowsWindow) Terminate()   { C.windows_webview_terminate(w.handle) }
func (w *windowsWindow) Destroy()     { C.windows_webview_destroy(w.handle) }
func (w *windowsWindow) Print() error { C.windows_webview_print(w.handle); return nil }
