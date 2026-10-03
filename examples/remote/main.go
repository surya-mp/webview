package main

import (
	"context"
	"log"

	"github.com/surya-mp/webview"
)

func main() {
	if err := webview.Run(context.Background(), webview.Options{Title: "Remote example", StartURL: "https://example.com"}); err != nil {
		log.Fatal(err)
	}
}
