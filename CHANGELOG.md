# Changelog

All notable user-facing changes are recorded here. This project follows
[Semantic Versioning](https://semver.org/).

## Unreleased

## 0.2.3 - 2026-10-04

### Fixed

- Windows CI now captures the host's expected invalid-URL exit status without
  PowerShell treating it as a failed native command before the smoke-test
  assertion runs.

## 0.2.2 - 2026-10-04

### Changed

- Hardened Windows/WebView2 and Linux/WebKitGTK origin checks so comparison is
  based on scheme, host, and effective port rather than a URL prefix.
- Same-origin popup requests reuse the existing Windows or Linux host window;
  external popup requests use the operating system's default browser.
- Windows and Linux host builds now run in CI and reject malformed startup
  URLs in a no-display smoke test.
- Windows builds copy `WebView2Loader.dll` next to `webview-host.exe`; Linux
  builds normalize output names to `amd64` and `arm64` and report missing
  WebKitGTK development files clearly.

### Release notes

- Windows and Linux runtime UI verification, artifact signing, and checksums
  are still release gates. They are not implied by the CI smoke tests.
