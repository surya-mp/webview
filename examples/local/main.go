package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"

	"github.com/surya-mp/webview"
)

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte(`<!doctype html><title>webview</title><h1>Local app</h1><p><a href="/settings">Internal route</a></p><p><a href="https://go.dev">External route</a></p><button onclick="window.print()">Print</button>`))
	})
	mux.HandleFunc("/settings", func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte(`<h1>Settings</h1><a href="/">Home</a>`))
	})
	if err := webview.Run(ctx, webview.Options{Handler: mux}); err != nil && !errors.Is(err, context.Canceled) {
		log.Fatal(err)
	}
}
