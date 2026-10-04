//go:build !(darwin && (arm64 || amd64))

package webview

func embeddedHost() []byte { return nil }
