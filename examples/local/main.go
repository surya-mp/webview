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
		_, _ = w.Write([]byte(`<!doctype html><title>webview</title><h1>Local app</h1><p><a href="/settings">Internal route</a></p><p><a href="https://go.dev">External route</a></p><button onclick="window.print()">Print</button>`))
	})
	mux.HandleFunc("/settings", func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte(`<h1>Settings</h1><a href="/">Home</a>`))
	})
	if err := webview.Run(context.Background(), webview.Options{Title: "Local example", Handler: mux}); err != nil {
		log.Fatal(err)
	}
}
