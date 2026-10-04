param(
    [string]$WebView2Sdk = $env:WEBVIEW2_SDK
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root "dist\windows\amd64\webview-host.exe"

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw "cl.exe was not found. Run this script from a Visual Studio x64 developer shell."
}

if (-not $WebView2Sdk -and $env:WEBVIEW2_INCLUDE) {
    $include = (Resolve-Path $env:WEBVIEW2_INCLUDE).Path
    $WebView2Sdk = (Resolve-Path (Join-Path $include "..\..")).Path
}
if (-not $WebView2Sdk) {
    throw "Set WEBVIEW2_SDK to the Microsoft.Web.WebView2 NuGet package directory (or WEBVIEW2_INCLUDE to its build\\native\\include directory)."
}

$WebView2Sdk = (Resolve-Path $WebView2Sdk).Path
$include = Join-Path $WebView2Sdk "build\native\include"
$loader = Join-Path $WebView2Sdk "build\native\x64\WebView2Loader.dll"
if (-not (Test-Path (Join-Path $include "WebView2.h"))) {
    throw "WebView2.h was not found under $include. WEBVIEW2_SDK must name a Microsoft.Web.WebView2 package directory."
}
if (-not (Test-Path $loader)) {
    throw "WebView2Loader.dll was not found at $loader. Build an x64 host with an x64 WebView2 SDK package."
}

New-Item -ItemType Directory -Force (Split-Path -Parent $output) | Out-Null
cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /DUNICODE /D_UNICODE /I $include (Join-Path $root "host\windows\main.cpp") /Fe:$output /link ole32.lib shell32.lib user32.lib wininet.lib
Copy-Item -Force $loader (Join-Path (Split-Path -Parent $output) "WebView2Loader.dll")
