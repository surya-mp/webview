import AppKit
import Darwin
import WebKit

struct Configuration {
    let url: URL
    let title: String
    let width: CGFloat
    let height: CGFloat

    init(arguments: [String]) throws {
        var values: [String: String] = [:]
        var index = 1
        while index + 1 < arguments.count {
            values[arguments[index]] = arguments[index + 1]
            index += 2
        }
        guard let rawURL = values["--url"], let url = URL(string: rawURL),
              let scheme = url.scheme, scheme == "http" || scheme == "https" else {
            throw ConfigurationError.invalidURL
        }
        self.url = url
        self.title = values["--title"] ?? "Web App"
        self.width = CGFloat(Int(values["--width"] ?? "1024") ?? 1024)
        self.height = CGFloat(Int(values["--height"] ?? "768") ?? 768)
    }
}

enum ConfigurationError: LocalizedError {
    case invalidURL

    var errorDescription: String? { "webview-host: provide an HTTP(S) --url" }
}

final class ApplicationDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate, WKNavigationDelegate, WKUIDelegate {
    private let configuration: Configuration
    private var window: NSWindow!
    private var webView: WKWebView!

    init(configuration: Configuration) {
        self.configuration = configuration
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.regular)
        installMainMenu()

        let frame = NSRect(x: 0, y: 0, width: configuration.width, height: configuration.height)
        window = NSWindow(
            contentRect: frame,
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = configuration.title
        window.delegate = self

        let preferences = WKPreferences()
        preferences.javaScriptCanOpenWindowsAutomatically = true
        let webConfiguration = WKWebViewConfiguration()
        webConfiguration.preferences = preferences
        webView = WKWebView(frame: frame, configuration: webConfiguration)
        webView.autoresizingMask = [.width, .height]
        webView.navigationDelegate = self
        webView.uiDelegate = self
        window.contentView = webView
        window.center()
        window.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
        webView.load(URLRequest(url: configuration.url))
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        guard let url = navigationAction.request.url else {
            decisionHandler(.cancel)
            return
        }
        guard navigationAction.targetFrame?.isMainFrame ?? true else {
            decisionHandler(.allow)
            return
        }
        if isSameOrigin(url) {
            decisionHandler(.allow)
        } else {
            NSWorkspace.shared.open(url)
            decisionHandler(.cancel)
        }
    }

    func webView(_ webView: WKWebView, createWebViewWith configuration: WKWebViewConfiguration, for navigationAction: WKNavigationAction, windowFeatures: WKWindowFeatures) -> WKWebView? {
        guard let url = navigationAction.request.url else { return nil }
        if isSameOrigin(url) {
            webView.load(URLRequest(url: url))
        } else {
            NSWorkspace.shared.open(url)
        }
        return nil
    }

    func webView(_ webView: WKWebView, runOpenPanelWith parameters: WKOpenPanelParameters, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping ([URL]?) -> Void) {
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = parameters.allowsDirectories
        panel.allowsMultipleSelection = parameters.allowsMultipleSelection
        panel.beginSheetModal(for: window) { response in
            completionHandler(response == .OK ? panel.urls : nil)
        }
    }

    func webView(_ webView: WKWebView, runJavaScriptAlertPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping () -> Void) {
        let alert = NSAlert()
        alert.messageText = frame.request.url?.host ?? configuration.title
        alert.informativeText = message
        alert.addButton(withTitle: "OK")
        alert.beginSheetModal(for: window) { _ in completionHandler() }
    }

    func webView(_ webView: WKWebView, runJavaScriptConfirmPanelWithMessage message: String, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (Bool) -> Void) {
        let alert = NSAlert()
        alert.messageText = frame.request.url?.host ?? configuration.title
        alert.informativeText = message
        alert.addButton(withTitle: "OK")
        alert.addButton(withTitle: "Cancel")
        alert.beginSheetModal(for: window) { response in completionHandler(response == .alertFirstButtonReturn) }
    }

    func webView(_ webView: WKWebView, runJavaScriptTextInputPanelWithPrompt prompt: String, defaultText: String?, initiatedByFrame frame: WKFrameInfo, completionHandler: @escaping (String?) -> Void) {
        let alert = NSAlert()
        let input = NSTextField(frame: NSRect(x: 0, y: 0, width: 300, height: 24))
        alert.messageText = frame.request.url?.host ?? configuration.title
        alert.informativeText = prompt
        input.stringValue = defaultText ?? ""
        alert.accessoryView = input
        alert.addButton(withTitle: "OK")
        alert.addButton(withTitle: "Cancel")
        alert.beginSheetModal(for: window) { response in
            completionHandler(response == .alertFirstButtonReturn ? input.stringValue : nil)
        }
    }

    private func isSameOrigin(_ url: URL) -> Bool {
        guard url.scheme?.lowercased() == configuration.url.scheme?.lowercased(),
              url.host?.lowercased() == configuration.url.host?.lowercased() else {
            return false
        }
        return effectivePort(url) == effectivePort(configuration.url)
    }

    private func effectivePort(_ url: URL) -> Int? {
        if let port = url.port { return port }
        switch url.scheme?.lowercased() {
        case "http": return 80
        case "https": return 443
        default: return nil
        }
    }

    private func installMainMenu() {
        let mainMenu = NSMenu()
        let applicationItem = NSMenuItem()
        let applicationMenu = NSMenu(title: configuration.title)
        applicationMenu.addItem(withTitle: "Quit \(configuration.title)", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        applicationItem.submenu = applicationMenu
        mainMenu.addItem(applicationItem)
        NSApp.mainMenu = mainMenu
    }
}

do {
    let configuration = try Configuration(arguments: CommandLine.arguments)
    let application = NSApplication.shared
    let delegate = ApplicationDelegate(configuration: configuration)
    application.delegate = delegate
    application.run()
} catch {
    fputs("\(error.localizedDescription)\n", stderr)
    exit(2)
}
