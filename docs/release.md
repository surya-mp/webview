# Release checklist

1. Run `go test ./...` and `go vet ./...`.
2. Build each host on its target OS and architecture.
3. Complete the platform verification runbook for every shipped artifact.
4. Sign macOS and Windows host artifacts and publish their checksums.
5. Confirm a clean sample application can use `go get` and `go run` without
   manual host configuration on platforms with a packaged host.
6. Confirm `go mod tidy` makes no unintended changes.
7. Review public API, behavior, examples, README, specification, and runbook
   changes together.
8. Tag `vMAJOR.MINOR.PATCH`, push the tag, and verify the release source and
   host artifacts are available.
