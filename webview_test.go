package webview

import (
	"context"
	"errors"
	"net/http"
	"runtime"
	"slices"
	"testing"
)

func TestValidateDefaults(t *testing.T) {
	o, err := validate(Options{Handler: http.NotFoundHandler()})
	if err != nil {
		t.Fatal(err)
	}
	if o.startPath != "/" {
		t.Fatalf("defaults = %#v", o)
	}
	if o.width != 1024 || o.height != 768 || o.title == "" {
		t.Fatalf("window defaults = %#v", o)
	}
}

func TestValidateStartURL(t *testing.T) {
	o, err := validate(Options{StartURL: "https://app.example.test/settings"})
	if err != nil {
		t.Fatal(err)
	}
	if o.startURL != "https://app.example.test/settings" {
		t.Fatalf("start URL = %q", o.startURL)
	}
	if _, err := validate(Options{Handler: http.NotFoundHandler(), StartURL: "https://app.example.test"}); err == nil {
		t.Fatal("validate accepted Handler and StartURL")
	}
}

func TestRunWithCanceledContextDoesNotLaunchBrowser(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	if err := Run(ctx, Options{Handler: http.NotFoundHandler()}); !errors.Is(err, context.Canceled) {
		t.Fatalf("Run error = %v, want context cancellation", err)
	}
}

func TestHostArgs(t *testing.T) {
	args := hostArgs(options{title: "Example", width: 640, height: 480}, "http://127.0.0.1:8080/")
	want := []string{"--url", "http://127.0.0.1:8080/", "--title", "Example", "--width", "640", "--height", "480"}
	if !slices.Equal(args, want) {
		t.Fatalf("host args = %#v, want %#v", args, want)
	}
}

func TestEmbeddedHostAvailableOnDarwin(t *testing.T) {
	if runtime.GOOS == "darwin" && len(embeddedHost()) == 0 {
		t.Fatal("embedded macOS host is empty")
	}
}
