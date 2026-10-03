//go:build darwin

package webview

/*
#cgo LDFLAGS: -framework Cocoa -framework WebKit
#include <stdint.h>
#include <stdlib.h>

typedef void* webview_handle;
webview_handle webview_new(const char *title, int width, int height, const char *url, const char *allowed_origin);
void webview_run(webview_handle handle);
void webview_terminate(webview_handle handle);
void webview_destroy(webview_handle handle);
void webview_print(webview_handle handle);
void webview_menu_prepare(webview_handle handle);
void* webview_menu_add_submenu(void *parent, const char *title);
void webview_menu_add_command(void *parent, const char *title, const char *shortcut, int enabled, uintptr_t action);
*/
import "C"

import (
	"sync"
	"unsafe"
)

type darwinWindow struct{ handle C.webview_handle }

var darwinActions struct {
	sync.RWMutex
	callback func(uintptr)
}

var darwinCallbacks struct {
	sync.RWMutex
	navigationError func(NavigationError)
	permission      func(PermissionRequest) PermissionDecision
}

func newNativeWindow(o options, url, allowedOrigin string, onMenu func(uintptr), onNavigationError func(NavigationError), permission func(PermissionRequest) PermissionDecision) (nativeWindow, error) {
	title := C.CString(o.title)
	defer C.free(unsafe.Pointer(title))
	startURL := C.CString(url)
	defer C.free(unsafe.Pointer(startURL))
	origin := C.CString(allowedOrigin)
	defer C.free(unsafe.Pointer(origin))
	handle := C.webview_new(title, C.int(o.width), C.int(o.height), startURL, origin)
	if handle == nil {
		return nil, ErrUnsupported
	}
	darwinActions.Lock()
	darwinActions.callback = onMenu
	darwinActions.Unlock()
	darwinCallbacks.Lock()
	darwinCallbacks.navigationError = onNavigationError
	darwinCallbacks.permission = permission
	darwinCallbacks.Unlock()
	installDarwinMenu(handle, o.Menu)
	return &darwinWindow{handle: handle}, nil
}

func (w *darwinWindow) Run() error {
	C.webview_run(w.handle)
	return nil
}

func (w *darwinWindow) Terminate() { C.webview_terminate(w.handle) }

func (w *darwinWindow) Print() error {
	C.webview_print(w.handle)
	return nil
}

func (w *darwinWindow) Destroy() {
	darwinActions.Lock()
	darwinActions.callback = nil
	darwinActions.Unlock()
	darwinCallbacks.Lock()
	darwinCallbacks.navigationError = nil
	darwinCallbacks.permission = nil
	darwinCallbacks.Unlock()
	C.webview_destroy(w.handle)
}

func installDarwinMenu(handle C.webview_handle, items []MenuItem) {
	C.webview_menu_prepare(handle)
	next := uintptr(1)
	var add func(unsafe.Pointer, []MenuItem)
	add = func(parent unsafe.Pointer, items []MenuItem) {
		for _, item := range items {
			title := C.CString(item.Label)
			if len(item.Children) != 0 {
				submenu := C.webview_menu_add_submenu(parent, title)
				C.free(unsafe.Pointer(title))
				add(submenu, item.Children)
				continue
			}
			shortcut := C.CString(item.Shortcut)
			action := next
			next++
			if item.Disabled {
				action = 0
			}
			C.webview_menu_add_command(parent, title, shortcut, C.int(boolToInt(!item.Disabled)), C.uintptr_t(action))
			C.free(unsafe.Pointer(title))
			C.free(unsafe.Pointer(shortcut))
		}
	}
	add(nil, items)
}

func boolToInt(value bool) int {
	if value {
		return 1
	}
	return 0
}

//export goWebviewMenuAction
func goWebviewMenuAction(action C.uintptr_t) {
	darwinActions.RLock()
	callback := darwinActions.callback
	darwinActions.RUnlock()
	if callback != nil {
		callback(uintptr(action))
	}
}

//export goWebviewNavigationError
func goWebviewNavigationError(url *C.char, code C.int, message *C.char) {
	darwinCallbacks.RLock()
	callback := darwinCallbacks.navigationError
	darwinCallbacks.RUnlock()
	if callback != nil {
		go callback(NavigationError{URL: C.GoString(url), Code: int(code), Message: C.GoString(message)})
	}
}

//export goWebviewPermission
func goWebviewPermission(origin *C.char, kind C.int) C.int {
	darwinCallbacks.RLock()
	callback := darwinCallbacks.permission
	darwinCallbacks.RUnlock()
	if callback == nil {
		return C.int(PermissionPrompt)
	}
	permissions := [...]Permission{
		PermissionCamera,
		PermissionMicrophone,
		PermissionCameraAndMicrophone,
		PermissionLocation,
		PermissionMotion,
	}
	if kind < 0 || int(kind) >= len(permissions) {
		return C.int(PermissionDeny)
	}
	return C.int(callback(PermissionRequest{Origin: C.GoString(origin), Permission: permissions[kind]}))
}
