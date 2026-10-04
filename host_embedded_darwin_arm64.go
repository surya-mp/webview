//go:build darwin && arm64

package webview

import _ "embed"

//go:embed internal/hostbin/darwin_arm64/webview-host
var darwinARM64Host []byte

func embeddedHost() []byte { return darwinARM64Host }
