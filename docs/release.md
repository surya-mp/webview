# Release checklist

1. Run `CGO_ENABLED=0 go test ./...` and `CGO_ENABLED=0 go vet ./...` on macOS, Linux, and Windows.
2. Build, sign, checksum, and target-test every `webview-host` artifact.
3. Complete every item in the platform verification runbook.
4. Confirm `go list -m` reports the intended module path and `go mod tidy`
   leaves no unexpected changes.
5. Review exported API changes for semantic-versioning impact and update the
   README, specification, and examples together.
6. Tag the commit as `vMAJOR.MINOR.PATCH`, push the tag, and verify a clean
   sample module can fetch it with `go get github.com/surya-mp/webview@<tag>`.
