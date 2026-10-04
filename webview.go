// Package webview runs an existing web application in a native desktop host.
package webview

import (
	"errors"
	"fmt"
	"net/http"
	"net/url"
	"os"
	"path"
	"strings"
)

var (
	// ErrHostNotFound is returned when the prebuilt native host is unavailable.
	ErrHostNotFound = errors.New("webview: prebuilt host not found")
)

// Options configures a native host window for an existing web application. Set
// Handler for an application served by this process, or StartURL for a deployed
// HTTP(S) application.
type Options struct {
	// Title is the native host window title. It defaults to the executable name.
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
	// HostPath uses this native host executable instead of the packaged host.
	// It is useful for development and custom host distributions.
	HostPath string
}

type options struct {
	Options
	title     string
	width     int
	height    int
	startPath string
	startURL  string
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
	}
	out.title = in.Title
	if out.title == "" {
		out.title = path.Base(os.Args[0])
	}
	return out, nil
}
