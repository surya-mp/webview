package main

import (
	"context"
	"errors"
	"log"
	"os"
	"os/signal"

	"github.com/surya-mp/webview"
)

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	if err := webview.Run(ctx, webview.Options{StartURL: "https://example.com"}); err != nil && !errors.Is(err, context.Canceled) {
		log.Fatal(err)
	}
}
