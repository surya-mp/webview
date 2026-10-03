//go:build linux && cgo

package webview

/*
#cgo pkg-config: webkit2gtk-4.1
#include <stdlib.h>
typedef void* linux_webview;
linux_webview linux_webview_new(const char*, int, int, const char*, const char*);
void linux_webview_run(linux_webview);
void linux_webview_terminate(linux_webview);
void linux_webview_destroy(linux_webview);
void linux_webview_print(linux_webview);
*/
import "C"

import "unsafe"

type linuxWindow struct{ handle C.linux_webview }

func newNativeWindow(o options, url, allowedOrigin string, onMenu func(uintptr), onNavigationError func(NavigationError), permission func(PermissionRequest) PermissionDecision) (nativeWindow, error) {
	title, startURL, origin := C.CString(o.title), C.CString(url), C.CString(allowedOrigin)
	defer C.free(unsafe.Pointer(title))
	defer C.free(unsafe.Pointer(startURL))
	defer C.free(unsafe.Pointer(origin))
	handle := C.linux_webview_new(title, C.int(o.width), C.int(o.height), startURL, origin)
	if handle == nil {
		return nil, ErrUnsupported
	}
	return &linuxWindow{handle: handle}, nil
}

func (w *linuxWindow) Run() error   { C.linux_webview_run(w.handle); return nil }
func (w *linuxWindow) Terminate()   { C.linux_webview_terminate(w.handle) }
func (w *linuxWindow) Destroy()     { C.linux_webview_destroy(w.handle) }
func (w *linuxWindow) Print() error { C.linux_webview_print(w.handle); return nil }
