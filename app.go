package webview

import (
	"errors"
	"sync"
)

// App is the running desktop shell. It is passed to Options.OnReady and is
// safe for use from Go routines.
type App struct {
	mu     sync.RWMutex
	window nativeWindow
}

func newApp() *App { return &App{} }

func (a *App) attach(window nativeWindow) {
	a.mu.Lock()
	a.window = window
	a.mu.Unlock()
}

func (a *App) detach() {
	a.mu.Lock()
	a.window = nil
	a.mu.Unlock()
}

func (a *App) native() (nativeWindow, error) {
	a.mu.RLock()
	defer a.mu.RUnlock()
	if a.window == nil {
		return nil, errors.New("webview: app is not running")
	}
	return a.window, nil
}

// Print opens the platform print dialog for the current page.
func (a *App) Print() error {
	window, err := a.native()
	if err != nil {
		return err
	}
	return window.Print()
}

// Close requests orderly native-window shutdown.
func (a *App) Close() {
	if window, err := a.native(); err == nil {
		window.Terminate()
	}
}
