package webview

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"sync"
	"time"
)

type localServer struct {
	listener net.Listener
	server   *http.Server
	start    string
	once     sync.Once
}

func startServer(handler http.Handler, startPath string) (*localServer, error) {
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		return nil, fmt.Errorf("webview: listen on loopback: %w", err)
	}

	local := &localServer{
		listener: listener,
		start:    startPath,
	}
	local.server = &http.Server{Handler: handler}
	go func() { _ = local.server.Serve(listener) }()
	return local, nil
}

func (s *localServer) URL() string {
	return s.Origin() + s.start
}

func (s *localServer) Origin() string {
	return "http://" + s.listener.Addr().String()
}

func (s *localServer) Close() {
	s.once.Do(func() {
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		if s.server.Shutdown(ctx) != nil {
			_ = s.server.Close()
		}
	})
}
