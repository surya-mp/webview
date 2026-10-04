package webview

import (
	"context"
	"errors"
	"fmt"
)

// Run serves options.Handler privately and opens it in the prebuilt native
// host. It returns when the host window closes, ctx is cancelled, or startup
// fails.
func Run(ctx context.Context, in Options) error {
	if ctx == nil {
		return errors.New("webview: context is required")
	}
	if err := ctx.Err(); err != nil {
		return err
	}
	o, err := validate(in)
	if err != nil {
		return err
	}
	startURL := o.startURL
	var server *localServer
	if o.Handler != nil {
		server, err = startServer(o.Handler, o.startPath)
		if err != nil {
			return err
		}
		defer server.Close()
		startURL = server.URL()
	}
	host, err := startHost(ctx, o, startURL)
	if err != nil {
		return err
	}
	defer host.cleanup()
	err = host.cmd.Wait()
	if ctx.Err() != nil {
		return ctx.Err()
	}
	if err != nil {
		return fmt.Errorf("webview: host exited: %w", err)
	}
	return nil
}
