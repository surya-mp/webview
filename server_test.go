package webview

import (
	"io"
	"net/http"
	"testing"
)

func TestLocalServerPreservesApplicationPaths(t *testing.T) {
	s, err := startServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		_, _ = io.WriteString(w, r.URL.Path)
	}), "/hello")
	if err != nil {
		t.Fatal(err)
	}
	defer s.Close()

	response, err := http.Get(s.URL())
	if err != nil {
		t.Fatal(err)
	}
	body, _ := io.ReadAll(response.Body)
	_ = response.Body.Close()
	if got := string(body); got != "/hello" {
		t.Fatalf("handler path = %q, want /hello", got)
	}

	response, err = http.Get(s.Origin() + "/other")
	if err != nil {
		t.Fatal(err)
	}
	body, _ = io.ReadAll(response.Body)
	_ = response.Body.Close()
	if got := string(body); got != "/other" {
		t.Fatalf("handler path = %q, want /other", got)
	}
}
