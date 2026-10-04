$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root "dist\windows\amd64\webview-host.exe"
New-Item -ItemType Directory -Force (Split-Path -Parent $output) | Out-Null
cl /std:c++17 /EHsc /O2 /I $env:WEBVIEW2_INCLUDE (Join-Path $root "host\windows\main.cpp") /Fe:$output /link ole32.lib shell32.lib user32.lib
