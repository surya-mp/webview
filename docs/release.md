# Release checklist

1. Run `go test ./...` and `go vet ./...` on macOS, Linux, and Windows.
2. Complete every item in the platform verification runbook.
3. Confirm `go list -m` reports the intended module path and `go mod tidy`
   leaves no unexpected changes.
4. Review exported API changes for semantic-versioning impact and update the
   README, specification, and examples together.
5. Tag the commit as `vMAJOR.MINOR.PATCH`, push the tag, and verify a clean
   sample module can fetch it with `go get github.com/surya-mp/webview@<tag>`.
