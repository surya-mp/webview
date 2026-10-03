package webview

import (
	"context"
	"net/http"
	"testing"
)

func TestValidateDefaults(t *testing.T) {
	o, err := validate(Options{Handler: http.NotFoundHandler()})
	if err != nil {
		t.Fatal(err)
	}
	if o.width != 1024 || o.height != 768 || o.startPath != "/" || o.title == "" {
		t.Fatalf("defaults = %#v", o)
	}
}

func TestValidateRejectsBadMenu(t *testing.T) {
	_, err := validate(Options{Handler: http.NotFoundHandler(), Menu: []MenuItem{{
		ID: "file", Label: "File", Action: func(context.Context) error { return nil },
		Children: []MenuItem{{ID: "quit", Label: "Quit", Action: func(context.Context) error { return nil }}},
	}}})
	if err == nil {
		t.Fatal("validate accepted an action submenu")
	}
}

func TestValidateStartURL(t *testing.T) {
	o, err := validate(Options{StartURL: "https://app.example.test/settings"})
	if err != nil {
		t.Fatal(err)
	}
	if o.origin != "https://app.example.test" {
		t.Fatalf("origin = %q", o.origin)
	}
	if _, err := validate(Options{Handler: http.NotFoundHandler(), StartURL: "https://app.example.test"}); err == nil {
		t.Fatal("validate accepted Handler and StartURL")
	}
}
