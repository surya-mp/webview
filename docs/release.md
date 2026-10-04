# Release checklist

## 1. Choose and document the version

1. Select the next `vMAJOR.MINOR.PATCH` version according to the public API
   and behavior changes.
2. Move the completed changes from [Unreleased](../CHANGELOG.md) into a dated
   version section and add the comparison link if the hosting platform uses
   them.
3. Review the public API, behavior, examples, README, specification, runbook,
   and change log together.

## 2. Run source and module gates

```sh
go test ./...
go vet ./...
go mod tidy
git diff --exit-code -- go.mod go.sum
```

Run the three-platform CI workflow and require its Linux and Windows native
host build/smoke jobs to pass.

## 3. Build and verify distributable hosts

Build each host on its target OS and architecture, then complete every
interactive check in the [platform runbook](operations.md). Record the OS,
architecture, browser-engine runtime, SDK, distribution, and desktop-session
versions used.

Package these files with each application release:

| Target | Required files |
| --- | --- |
| macOS arm64 / amd64 | The matching signed `webview-host` embedded in the module |
| Windows amd64 | Signed `webview-host.exe` and `WebView2Loader.dll` |
| Linux amd64 / arm64 | Matching executable `webview-host` |

## 4. Sign and checksum artifacts

Sign macOS and Windows host artifacts with the release identity. Publish a
SHA-256 checksum for every distributed host file, including
`WebView2Loader.dll`. For example:

```sh
shasum -a 256 webview-host
```

```powershell
Get-FileHash .\webview-host.exe, .\WebView2Loader.dll -Algorithm SHA256
```

## 5. Publish

1. Confirm a clean sample application can use `go get` and `go run` without
   manual host configuration on platforms with a packaged host.
2. Commit the release notes and release artifacts or their documented download
   locations.
3. Tag `vMAJOR.MINOR.PATCH`, push the commit and tag, and verify the release
   source, signatures, checksums, and host artifacts are available.
