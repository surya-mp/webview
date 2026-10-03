// Package webview runs an existing Go web application in a native desktop
// browser shell. The application keeps serving the same http.Handler it uses
// in a browser; webview only starts a private local server and opens a window.
package webview

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"net/url"
	"os"
	"path"
	"strings"
)

var (
	// ErrAlreadyRunning is returned when a process tries to own two desktop UI
	// event loops at once.
	ErrAlreadyRunning = errors.New("webview: an application window is already running")
	// ErrUnsupported is returned when this build has no native window backend
	// for the current operating system.
	ErrUnsupported = errors.New("webview: native window backend is not implemented for this platform")
)

// Options configures the desktop shell for an existing web application. Set
// Handler for an application served by this process, or StartURL for a
// deployed HTTP(S) application.
type Options struct {
	// Title is the native window title. The executable name is the default.
	Title string
	// Width and Height default to 1024 and 768 when zero.
	Width, Height int
	// Handler is served from a loopback-only HTTP server. Set exactly one of
	// Handler and StartURL.
	Handler http.Handler
	// StartURL is an absolute HTTP(S) application URL. Set exactly one of
	// StartURL and Handler.
	StartURL string
	// StartPath is the initial absolute path for Handler mode and defaults to /.
	StartPath string
	// Menu is the ordered static native menu model.
	Menu []MenuItem
	// OnReady runs after native window creation and receives the shell handle.
	OnReady func(*App)
	// OnNavigationError receives failed main-frame navigation details.
	OnNavigationError func(NavigationError)
	// PermissionPolicy controls capability requests from web content.
	PermissionPolicy PermissionPolicy
}

// MenuItem is a static command or submenu installed when Run starts.
type MenuItem struct {
	ID       string
	Label    string
	Shortcut string
	Disabled bool
	Children []MenuItem
	Action   func(context.Context) error
}

// Permission identifies a browser capability requested by web content.
type Permission string

const (
	PermissionCamera              Permission = "camera"
	PermissionMicrophone          Permission = "microphone"
	PermissionCameraAndMicrophone Permission = "camera-and-microphone"
	PermissionLocation            Permission = "location"
	PermissionMotion              Permission = "motion"
)

// PermissionDecision determines how the platform handles a permission
// request. Prompt delegates to the operating system's normal prompt.
type PermissionDecision uint8

const (
	PermissionPrompt PermissionDecision = iota
	PermissionAllow
	PermissionDeny
)

// PermissionRequest identifies the requesting web origin and capability.
type PermissionRequest struct {
	Origin     string
	Permission Permission
}

// PermissionPolicy decides a web-content permission request synchronously.
type PermissionPolicy func(context.Context, PermissionRequest) PermissionDecision

// NavigationError reports a failed main-frame navigation.
type NavigationError struct {
	URL     string
	Code    int
	Message string
}

type options struct {
	Options
	title     string
	width     int
	height    int
	startPath string
	startURL  string
	origin    string
}

func validate(in Options) (options, error) {
	if (in.Handler == nil) == (in.StartURL == "") {
		return options{}, errors.New("webview: provide exactly one of Handler or StartURL")
	}
	if in.Width < 0 || in.Height < 0 {
		return options{}, errors.New("webview: Width and Height must be positive when provided")
	}

	out := options{Options: in, width: in.Width, height: in.Height, startPath: in.StartPath, startURL: in.StartURL}
	if out.width == 0 {
		out.width = 1024
	}
	if out.height == 0 {
		out.height = 768
	}
	if in.Handler != nil && out.startPath == "" {
		out.startPath = "/"
	}
	if in.Handler != nil && (!strings.HasPrefix(out.startPath, "/") || path.Clean(out.startPath) != out.startPath) {
		return options{}, fmt.Errorf("webview: StartPath must be an absolute clean path, got %q", in.StartPath)
	}
	if in.StartURL != "" {
		if in.StartPath != "" {
			return options{}, errors.New("webview: StartPath cannot be used with StartURL")
		}
		parsed, err := url.Parse(in.StartURL)
		if err != nil || (parsed.Scheme != "http" && parsed.Scheme != "https") || parsed.Host == "" {
			return options{}, fmt.Errorf("webview: StartURL must be an absolute HTTP(S) URL, got %q", in.StartURL)
		}
		out.origin = parsed.Scheme + "://" + parsed.Host
	}
	out.title = in.Title
	if out.title == "" {
		out.title = path.Base(os.Args[0])
	}

	ids := make(map[string]struct{})
	if err := validateMenu(in.Menu, ids); err != nil {
		return options{}, err
	}
	return out, nil
}

func validateMenu(items []MenuItem, ids map[string]struct{}) error {
	for _, item := range items {
		if item.ID == "" {
			return errors.New("webview: every menu item requires an ID")
		}
		if item.Label == "" {
			return fmt.Errorf("webview: menu item %q requires a Label", item.ID)
		}
		if _, exists := ids[item.ID]; exists {
			return fmt.Errorf("webview: duplicate menu ID %q", item.ID)
		}
		ids[item.ID] = struct{}{}
		if len(item.Children) != 0 && item.Action != nil {
			return fmt.Errorf("webview: menu item %q cannot have both Children and Action", item.ID)
		}
		if len(item.Children) == 0 && item.Action == nil {
			return fmt.Errorf("webview: menu item %q requires Action or Children", item.ID)
		}
		if err := validateMenu(item.Children, ids); err != nil {
			return err
		}
	}
	return nil
}
