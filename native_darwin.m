#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include <stdint.h>

extern void goWebviewMenuAction(uintptr_t action);
extern void goWebviewNavigationError(const char *url, int code, const char *message);
extern int goWebviewPermission(const char *origin, int kind);

@interface WebviewWindowDelegate : NSObject<NSWindowDelegate>
@end

@implementation WebviewWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
  [NSApp stop:nil];
  [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:YES];
}
@end

@interface WebviewMenuTarget : NSObject
@end

@implementation WebviewMenuTarget
- (void)activate:(id)sender { goWebviewMenuAction((uintptr_t)[sender tag]); }
@end

@interface WebviewDownloadDelegate : NSObject<WKDownloadDelegate>
@property(retain) NSWindow *window;
@end

@interface WebviewNavigationDelegate : NSObject<WKNavigationDelegate>
@property(retain) NSURL *allowedOrigin;
@property(retain) WebviewDownloadDelegate *downloadDelegate;
@end

@implementation WebviewNavigationDelegate
static NSInteger effectivePort(NSURL *URL) {
  if (URL.port != nil) return URL.port.integerValue;
  if ([URL.scheme isEqualToString:@"https"]) return 443;
  if ([URL.scheme isEqualToString:@"http"]) return 80;
  return -1;
}

- (BOOL)isAllowedURL:(NSURL *)URL {
  return [URL.scheme caseInsensitiveCompare:self.allowedOrigin.scheme] == NSOrderedSame &&
         [URL.host caseInsensitiveCompare:self.allowedOrigin.host] == NSOrderedSame &&
         effectivePort(URL) == effectivePort(self.allowedOrigin);
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)action decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
  if (action.targetFrame.isMainFrame && ![self isAllowedURL:action.request.URL]) {
    [[NSWorkspace sharedWorkspace] openURL:action.request.URL];
    decisionHandler(WKNavigationActionPolicyCancel);
    return;
  }
  decisionHandler(WKNavigationActionPolicyAllow);
}

- (void)webView:(WKWebView *)webView navigationAction:(WKNavigationAction *)action didBecomeDownload:(WKDownload *)download API_AVAILABLE(macos(11.0)) {
  download.delegate = self.downloadDelegate;
}

- (void)webView:(WKWebView *)webView navigationResponse:(WKNavigationResponse *)response didBecomeDownload:(WKDownload *)download API_AVAILABLE(macos(11.0)) {
  download.delegate = self.downloadDelegate;
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error {
  if (error.code != NSURLErrorCancelled) goWebviewNavigationError((webView.URL.absoluteString ?: @"").UTF8String, (int)error.code, error.localizedDescription.UTF8String);
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
  if (error.code != NSURLErrorCancelled) goWebviewNavigationError((webView.URL.absoluteString ?: @"").UTF8String, (int)error.code, error.localizedDescription.UTF8String);
}
@end

@interface WebviewUIDelegate : NSObject<WKUIDelegate>
@end

@implementation WebviewUIDelegate
- (WKWebView *)webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)action windowFeatures:(WKWindowFeatures *)windowFeatures {
  [webView loadRequest:action.request];
  return nil;
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler {
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = frame.request.URL.host ?: @"This page";
  alert.informativeText = message;
  [alert addButtonWithTitle:@"OK"];
  [alert beginSheetModalForWindow:webView.window completionHandler:^(NSModalResponse response) {
    completionHandler();
  }];
}

- (void)webView:(WKWebView *)webView runJavaScriptConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(BOOL))completionHandler {
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = frame.request.URL.host ?: @"This page";
  alert.informativeText = message;
  [alert addButtonWithTitle:@"OK"];
  [alert addButtonWithTitle:@"Cancel"];
  [alert beginSheetModalForWindow:webView.window completionHandler:^(NSModalResponse response) {
    completionHandler(response == NSAlertFirstButtonReturn);
  }];
}

- (void)webView:(WKWebView *)webView runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt defaultText:(NSString *)defaultText initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(NSString *))completionHandler {
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = frame.request.URL.host ?: @"This page";
  alert.informativeText = prompt;
  NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
  input.stringValue = defaultText ?: @"";
  alert.accessoryView = input;
  [alert addButtonWithTitle:@"OK"];
  [alert addButtonWithTitle:@"Cancel"];
  [alert beginSheetModalForWindow:webView.window completionHandler:^(NSModalResponse response) {
    completionHandler(response == NSAlertFirstButtonReturn ? input.stringValue : nil);
  }];
}

- (void)webView:(WKWebView *)webView runBeforeUnloadConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(BOOL))completionHandler {
  [self webView:webView runJavaScriptConfirmPanelWithMessage:message initiatedByFrame:frame completionHandler:completionHandler];
}

- (void)webView:(WKWebView *)webView runOpenPanelWithParameters:(WKOpenPanelParameters *)parameters initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(NSArray<NSURL *> *))completionHandler {
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  panel.canChooseFiles = YES;
  panel.canChooseDirectories = parameters.allowsDirectories;
  panel.allowsMultipleSelection = parameters.allowsMultipleSelection;
  [panel beginSheetModalForWindow:webView.window completionHandler:^(NSModalResponse response) {
    completionHandler(response == NSModalResponseOK ? panel.URLs : nil);
  }];
}

- (void)webView:(WKWebView *)webView requestMediaCapturePermissionForOrigin:(WKSecurityOrigin *)origin initiatedByFrame:(WKFrameInfo *)frame type:(WKMediaCaptureType)type decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
  NSInteger kind = type == WKMediaCaptureTypeCamera ? 0 : type == WKMediaCaptureTypeMicrophone ? 1 : 2;
  NSString *source = [NSString stringWithFormat:@"%@://%@:%ld", origin.protocol, origin.host, (long)origin.port];
  int decision = goWebviewPermission(source.UTF8String, (int)kind);
  decisionHandler(decision == 1 ? WKPermissionDecisionGrant : decision == 2 ? WKPermissionDecisionDeny : WKPermissionDecisionPrompt);
}

- (void)webView:(WKWebView *)webView requestDeviceOrientationAndMotionPermissionForOrigin:(WKSecurityOrigin *)origin initiatedByFrame:(WKFrameInfo *)frame decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
  NSString *source = [NSString stringWithFormat:@"%@://%@:%ld", origin.protocol, origin.host, (long)origin.port];
  int decision = goWebviewPermission(source.UTF8String, 4);
  decisionHandler(decision == 1 ? WKPermissionDecisionGrant : decision == 2 ? WKPermissionDecisionDeny : WKPermissionDecisionPrompt);
}

- (void)webView:(WKWebView *)webView requestGeolocationPermissionForOrigin:(WKSecurityOrigin *)origin initiatedByFrame:(WKFrameInfo *)frame decisionHandler:(void (^)(WKPermissionDecision))decisionHandler {
  NSString *source = [NSString stringWithFormat:@"%@://%@:%ld", origin.protocol, origin.host, (long)origin.port];
  int decision = goWebviewPermission(source.UTF8String, 3);
  decisionHandler(decision == 1 ? WKPermissionDecisionGrant : decision == 2 ? WKPermissionDecisionDeny : WKPermissionDecisionPrompt);
}
@end

@implementation WebviewDownloadDelegate
- (void)download:(WKDownload *)download decideDestinationUsingResponse:(NSURLResponse *)response suggestedFilename:(NSString *)suggestedFilename completionHandler:(void (^)(NSURL *))completionHandler API_AVAILABLE(macos(11.0)) {
  NSSavePanel *panel = [NSSavePanel savePanel];
  panel.canCreateDirectories = YES;
  panel.nameFieldStringValue = suggestedFilename.length > 0 ? suggestedFilename.lastPathComponent : @"download";
  [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
    completionHandler(result == NSModalResponseOK ? panel.URL : nil);
  }];
}

- (void)downloadDidFinish:(WKDownload *)download API_AVAILABLE(macos(11.0)) {}

- (void)download:(WKDownload *)download didFailWithError:(NSError *)error resumeData:(NSData *)resumeData API_AVAILABLE(macos(11.0)) {
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = @"Download failed";
  alert.informativeText = error.localizedDescription;
  [alert addButtonWithTitle:@"OK"];
  [alert beginSheetModalForWindow:self.window completionHandler:nil];
}
@end

typedef struct {
  NSWindow *window;
  WebviewWindowDelegate *delegate;
  WebviewNavigationDelegate *navigationDelegate;
  WebviewUIDelegate *uiDelegate;
  WebviewDownloadDelegate *downloadDelegate;
  WKWebView *webView;
} WebviewHandle;

static WebviewMenuTarget *menuTarget;

static NSEventModifierFlags modifiersForShortcut(NSString *shortcut, NSString **key) {
  NSEventModifierFlags modifiers = 0;
  NSArray<NSString *> *parts = [[shortcut lowercaseString] componentsSeparatedByString:@"+"];
  for (NSString *part in parts) {
    if ([part isEqualToString:@"cmd"] || [part isEqualToString:@"command"]) modifiers |= NSEventModifierFlagCommand;
    else if ([part isEqualToString:@"ctrl"] || [part isEqualToString:@"control"]) modifiers |= NSEventModifierFlagControl;
    else if ([part isEqualToString:@"alt"] || [part isEqualToString:@"option"]) modifiers |= NSEventModifierFlagOption;
    else if ([part isEqualToString:@"shift"]) modifiers |= NSEventModifierFlagShift;
    else if ([part length] > 0) *key = part;
  }
  return modifiers;
}

void *webview_new(const char *title, int width, int height, const char *url, const char *allowedOrigin) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSRect frame = NSMakeRect(0, 0, width, height);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
    WKWebView *view = [[WKWebView alloc] initWithFrame:frame configuration:[[WKWebViewConfiguration alloc] init]];
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [window setContentView:view];
    [window setTitle:[NSString stringWithUTF8String:title]];
    WebviewWindowDelegate *delegate = [[WebviewWindowDelegate alloc] init];
    [window setDelegate:delegate];
    WebviewNavigationDelegate *navigationDelegate = [[WebviewNavigationDelegate alloc] init];
    navigationDelegate.allowedOrigin = [NSURL URLWithString:[NSString stringWithUTF8String:allowedOrigin]];
    view.navigationDelegate = navigationDelegate;
    WebviewUIDelegate *uiDelegate = [[WebviewUIDelegate alloc] init];
    view.UIDelegate = uiDelegate;
    NSURL *startURL = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    [view loadRequest:[NSURLRequest requestWithURL:startURL]];
    [window center];
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    WebviewHandle *handle = calloc(1, sizeof(WebviewHandle));
    handle->window = window;
    handle->delegate = delegate;
    handle->navigationDelegate = navigationDelegate;
    handle->uiDelegate = uiDelegate;
    handle->webView = view;
    WebviewDownloadDelegate *downloadDelegate = [[WebviewDownloadDelegate alloc] init];
    downloadDelegate.window = window;
    navigationDelegate.downloadDelegate = downloadDelegate;
    handle->downloadDelegate = downloadDelegate;
    menuTarget = [[WebviewMenuTarget alloc] init];
    return handle;
  }
}

void webview_run(void *raw) { [NSApp run]; }

void webview_terminate(void *raw) {
  dispatch_async(dispatch_get_main_queue(), ^{
    [NSApp stop:nil];
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:YES];
  });
}

void webview_destroy(void *raw) {
  WebviewHandle *handle = raw;
  [handle->window setDelegate:nil];
  [handle->window close];
  free(handle);
}

void webview_print(void *raw) {
  WebviewHandle *handle = raw;
  WKWebView *view = handle->webView;
  dispatch_async(dispatch_get_main_queue(), ^{
    NSPrintOperation *operation = [view printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
    [operation runOperationModalForWindow:view.window delegate:nil didRunSelector:NULL contextInfo:nil];
  });
}

void webview_menu_prepare(void *raw) {
  WebviewHandle *handle = raw;
  NSMenu *main = [[NSMenu alloc] initWithTitle:@""];
  NSString *appTitle = handle->window.title;
  NSMenuItem *app = [[NSMenuItem alloc] initWithTitle:appTitle action:nil keyEquivalent:@""];
  NSMenu *appMenu = [[NSMenu alloc] initWithTitle:appTitle];
  NSMenuItem *quit = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
  [quit setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
  [appMenu addItem:quit];
  [app setSubmenu:appMenu];
  [main addItem:app];
  [NSApp setMainMenu:main];
}

void *webview_menu_add_submenu(void *parent, const char *title) {
  NSMenu *menu = parent ? (NSMenu *)parent : [NSApp mainMenu];
  NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:title] action:nil keyEquivalent:@""];
  NSMenu *submenu = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:title]];
  [item setSubmenu:submenu];
  [menu addItem:item];
  return submenu;
}

void webview_menu_add_command(void *parent, const char *title, const char *shortcut, int enabled, uintptr_t action) {
  NSMenu *menu = parent ? (NSMenu *)parent : [NSApp mainMenu];
  NSString *key = @"";
  NSEventModifierFlags modifiers = modifiersForShortcut([NSString stringWithUTF8String:shortcut], &key);
  NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:title] action:@selector(activate:) keyEquivalent:key];
  [item setKeyEquivalentModifierMask:modifiers];
  [item setTag:(NSInteger)action];
  [item setEnabled:enabled];
  [item setTarget:menuTarget];
  [menu addItem:item];
}
