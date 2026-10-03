package webview

import (
	"context"
	"errors"
	"runtime"
	"sync"
)

var processWindow = make(chan struct{}, 1)

// Run serves options.Handler privately and opens it in one native desktop
// window. It blocks until the window closes, ctx is cancelled, or setup fails.
func Run(ctx context.Context, in Options) error {
	if ctx == nil {
		return errors.New("webview: context is required")
	}
	o, err := validate(in)
	if err != nil {
		return err
	}
	select {
	case processWindow <- struct{}{}:
		defer func() { <-processWindow }()
	default:
		return ErrAlreadyRunning
	}

	startURL, origin := o.startURL, o.origin
	var server *localServer
	if o.Handler != nil {
		server, err = startServer(o.Handler, o.startPath)
		if err != nil {
			return err
		}
		defer server.Close()
		startURL, origin = server.URL(), server.Origin()
	}

	runtime.LockOSThread()
	defer runtime.UnlockOSThread()
	runner := newMenuRunner(ctx, o.Menu)
	app := newApp()
	window, err := newNativeWindow(o, startURL, origin, runner.invoke, o.OnNavigationError, func(request PermissionRequest) PermissionDecision {
		if o.PermissionPolicy == nil {
			return PermissionPrompt
		}
		return o.PermissionPolicy(ctx, request)
	})
	if err != nil {
		return err
	}
	defer window.Destroy()
	defer app.detach()
	runner.window = window
	app.attach(window)
	if o.OnReady != nil {
		o.OnReady(app)
	}

	done := make(chan struct{})
	go func() {
		select {
		case <-ctx.Done():
			window.Terminate()
		case <-done:
		}
	}()
	err = window.Run()
	close(done)
	if actionErr := runner.err(); actionErr != nil {
		return actionErr
	}
	if ctx.Err() != nil {
		return ctx.Err()
	}
	return err
}

type nativeWindow interface {
	Run() error
	Terminate()
	Destroy()
	Print() error
}

type menuRunner struct {
	ctx     context.Context
	actions map[uintptr]func(context.Context) error
	running map[uintptr]bool
	window  nativeWindow
	mu      sync.Mutex
	failure error
}

func newMenuRunner(ctx context.Context, items []MenuItem) *menuRunner {
	r := &menuRunner{ctx: ctx, actions: make(map[uintptr]func(context.Context) error), running: make(map[uintptr]bool)}
	var add func([]MenuItem)
	add = func(items []MenuItem) {
		for _, item := range items {
			if item.Action != nil {
				r.actions[uintptr(len(r.actions)+1)] = item.Action
			}
			add(item.Children)
		}
	}
	add(items)
	return r
}

func (r *menuRunner) invoke(id uintptr) {
	r.mu.Lock()
	action := r.actions[id]
	if action == nil || r.running[id] {
		r.mu.Unlock()
		return
	}
	r.running[id] = true
	r.mu.Unlock()

	go func() {
		err := action(r.ctx)
		r.mu.Lock()
		r.running[id] = false
		if err != nil && r.failure == nil {
			r.failure = err
			window := r.window
			r.mu.Unlock()
			window.Terminate()
			return
		}
		r.mu.Unlock()
	}()
}

func (r *menuRunner) err() error {
	r.mu.Lock()
	defer r.mu.Unlock()
	return r.failure
}
