# webview development plan

## Decisions to confirm before implementation

1. Build first-party platform backends directly on WKWebView, WebView2, and
   WebKitGTK. Do not add a third-party Go webview dependency.
2. Keep native menus in those build-tagged backends; do not introduce a
   desktop framework solely to obtain menu APIs.
3. Implement one navigation policy on every backend: same-origin navigation
   and popups stay in the current window; another origin opens in the default
   browser.

## Phase 1 — Package skeleton and option validation

- Create `go.mod` and package documentation.
- Define the public `Options`, `MenuItem`, and `Run` API from `SPEC.md`.
- Implement complete validation: required handler, dimensions, start path,
  menu IDs/labels, duplicate IDs, and invalid action/submenu combinations.
- Add table-driven unit tests for validation and defaults.

**Exit condition:** invalid input fails before listeners or native resources
are created.

## Phase 2 — Private application server

- Implement an internal loopback server using an ephemeral port.
- Serve the supplied handler unchanged so existing absolute and relative app
  routes preserve their browser behavior.
- Implement context-aware graceful shutdown and unit/integration tests proving
  that all application routes are reachable through the generated origin.

**Exit condition:** `Run` can own and cleanly stop a private HTTP endpoint
independently of a real window.

## Phase 3 — Webview backend and lifecycle

- Wrap the selected binding in an internal interface used solely to allow
  deterministic lifecycle tests; keep that interface unexported.
- Implement window creation, title/size defaults, navigation, close handling,
  UI-thread dispatch, context cancellation, and one-time teardown.
- Translate missing-runtime and native-startup failures into package errors
  with platform remediation.
- Test terminal-event races using a fake internal backend; add an optional
  manual smoke-test example for each supported desktop OS.

**Exit condition:** a minimal handler opens a chrome-free native window and
`Run` returns exactly once on every terminal path.

## Phase 4 — Menus

- Convert the validated menu tree to a backend-neutral internal model.
- Implement macOS global menu-bar installation in a Darwin build-tagged file,
  preserving application/Quit items.
- Implement native in-window menu bars for Windows and Linux where the
  backend supports them; otherwise implement a small accessible HTML fallback
  injected by the shell, with no application API changes.
- Dispatch menu actions off the UI thread; reject re-entry and route an action
  error through the common shutdown path.
- Add platform-independent model/dispatch tests and OS-gated smoke checks for
  visual placement and shortcuts.

**Exit condition:** the same `Options.Menu` model works on all supported OSes,
with macOS menus in the top system panel.

## Phase 5 — Documentation, examples, and release checks

- Write a short README with installation prerequisites, a minimal handler
  example, menu example, limitations, and cross-compilation notes.
- Add CI for formatting, `go vet`, unit tests, and native build checks on
  macOS, Windows, and Linux (installing WebKitGTK development packages on
  Linux).
- Add a manual release checklist that verifies close behavior, external-link
  policy, menu placement, disabled actions, and missing-runtime diagnostics.
- Tag the first release only after all acceptance criteria in `SPEC.md` pass.

## Deliberate v0 cuts

Use one window, one local handler, and static startup menus. Add multi-window
coordination, a richer Go↔web bridge, or dynamic menu APIs only after a real
consumer needs them; each changes lifecycle and compatibility commitments.
