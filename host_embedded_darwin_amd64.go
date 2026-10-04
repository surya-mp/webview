//go:build darwin && amd64

package webview

import _ "embed"

//go:embed internal/hostbin/darwin_amd64/webview-host
var darwinAMD64Host []byte

func embeddedHost() []byte { return darwinAMD64Host }
