package webview

import (
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
)

type hostProcess struct {
	cmd     *exec.Cmd
	cleanup func()
}

func startHost(ctx context.Context, o options, startURL string) (*hostProcess, error) {
	path, err := findHost(o.HostPath)
	if err != nil {
		if o.HostPath != "" || !errors.Is(err, ErrHostNotFound) {
			return nil, err
		}
		path, cleanup, extractErr := extractEmbeddedHost()
		if extractErr != nil {
			if errors.Is(extractErr, ErrHostNotFound) {
				return nil, err
			}
			return nil, extractErr
		}
		return launchHost(ctx, path, o, startURL, cleanup)
	}
	return launchHost(ctx, path, o, startURL, func() {})
}

func launchHost(ctx context.Context, path string, o options, startURL string, cleanup func()) (*hostProcess, error) {
	cmd := exec.CommandContext(ctx, path, hostArgs(o, startURL)...)
	if err := cmd.Start(); err != nil {
		cleanup()
		return nil, fmt.Errorf("webview: start host: %w", err)
	}
	return &hostProcess{cmd: cmd, cleanup: cleanup}, nil
}

func extractEmbeddedHost() (string, func(), error) {
	asset := embeddedHost()
	if len(asset) == 0 {
		return "", nil, ErrHostNotFound
	}
	directory, err := os.MkdirTemp("", "webview-host-")
	if err != nil {
		return "", nil, fmt.Errorf("webview: create host directory: %w", err)
	}
	cleanup := func() { _ = os.RemoveAll(directory) }
	path := filepath.Join(directory, hostName())
	if err := os.WriteFile(path, asset, 0700); err != nil {
		cleanup()
		return "", nil, fmt.Errorf("webview: extract host: %w", err)
	}
	return path, cleanup, nil
}

func hostArgs(o options, startURL string) []string {
	return []string{
		"--url", startURL,
		"--title", o.title,
		"--width", fmt.Sprint(o.width),
		"--height", fmt.Sprint(o.height),
	}
}

func findHost(configured string) (string, error) {
	if configured != "" {
		path, err := exec.LookPath(configured)
		if err != nil {
			return "", fmt.Errorf("webview: find configured host %q: %w", configured, err)
		}
		return path, nil
	}

	name := hostName()
	if executable, err := os.Executable(); err == nil {
		candidate := filepath.Join(filepath.Dir(executable), name)
		if info, err := os.Stat(candidate); err == nil && !info.IsDir() {
			return candidate, nil
		}
	}
	if path, err := exec.LookPath(name); err == nil {
		return path, nil
	}
	return "", fmt.Errorf("%w: package %q with the application, add it to PATH, or set Options.HostPath", ErrHostNotFound, name)
}

func hostName() string {
	if runtime.GOOS == "windows" {
		return "webview-host.exe"
	}
	return "webview-host"
}
