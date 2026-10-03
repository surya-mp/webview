#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include <WebView2.h>
#include <string>
#include <cstring>

using namespace Microsoft::WRL;
typedef HRESULT (STDAPICALLTYPE *CreateEnvironmentFn)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

struct WindowsWebView { HWND hwnd; std::wstring url, origin; ComPtr<ICoreWebView2Controller> controller; ComPtr<ICoreWebView2> view; HRESULT failure; };
static const wchar_t *klass = L"GoWebViewWindow";

static std::wstring utf8(const char *text) {
  int size = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
  std::wstring result(size, L'\0');
  if (size) MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), size);
  if (size) result.pop_back();
  return result;
}

static bool same_origin(const std::wstring &url, const std::wstring &origin) {
  return url.size() >= origin.size() && _wcsnicmp(url.c_str(), origin.c_str(), origin.size()) == 0 && (url.size() == origin.size() || url[origin.size()] == L'/');
}
static LRESULT CALLBACK wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  WindowsWebView *shell = reinterpret_cast<WindowsWebView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (message == WM_SIZE && shell && shell->controller) { RECT bounds; GetClientRect(hwnd, &bounds); shell->controller->put_Bounds(bounds); return 0; }
  if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
  return DefWindowProcW(hwnd, message, wparam, lparam);
}
static void install_events(WindowsWebView *shell) {
  EventRegistrationToken token{};
  shell->view->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>([shell](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs *args) {
    LPWSTR raw = nullptr; args->get_Uri(&raw); std::wstring uri(raw ? raw : L""); CoTaskMemFree(raw);
    if (!same_origin(uri, shell->origin)) { ShellExecuteW(shell->hwnd, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL); args->put_Cancel(TRUE); }
    return S_OK;
  }).Get(), &token);
  shell->view->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>([shell](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs *args) {
    LPWSTR raw = nullptr; args->get_Uri(&raw); std::wstring uri(raw ? raw : L""); CoTaskMemFree(raw);
    if (same_origin(uri, shell->origin)) shell->view->Navigate(uri.c_str()); else ShellExecuteW(shell->hwnd, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    args->put_Handled(TRUE); return S_OK;
  }).Get(), &token);
}
void *windows_webview_new(const char *title, int width, int height, const char *url, const char *origin) {
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  WNDCLASSW wc{}; wc.lpfnWndProc = wndproc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = klass; RegisterClassW(&wc);
  std::wstring name = utf8(title);
  auto *shell = new WindowsWebView{}; shell->url = utf8(url); shell->origin = utf8(origin);
  shell->hwnd = CreateWindowExW(0, klass, name.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, wc.hInstance, nullptr); SetWindowLongPtrW(shell->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(shell)); ShowWindow(shell->hwnd, SW_SHOW);
  HMODULE loader = LoadLibraryW(L"WebView2Loader.dll"); if (!loader) { delete shell; return nullptr; }
  auto create = reinterpret_cast<CreateEnvironmentFn>(GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions")); if (!create) { delete shell; return nullptr; }
  create(nullptr, nullptr, nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([shell](HRESULT result, ICoreWebView2Environment *env) {
    if (FAILED(result)) { shell->failure = result; PostQuitMessage(0); return S_OK; }
    env->CreateCoreWebView2Controller(shell->hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([shell](HRESULT result, ICoreWebView2Controller *controller) {
      if (FAILED(result)) { shell->failure = result; PostQuitMessage(0); return S_OK; }
      shell->controller = controller; controller->get_CoreWebView2(&shell->view); install_events(shell); shell->view->Navigate(shell->url.c_str()); return S_OK;
    }).Get()); return S_OK;
  }).Get()); return shell;
}
int windows_webview_run(void *raw) { WindowsWebView *shell = (WindowsWebView*)raw; MSG msg; while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); } return FAILED(shell->failure); }
void windows_webview_terminate(void *raw) { WindowsWebView *shell = (WindowsWebView*)raw; PostMessageW(shell->hwnd, WM_CLOSE, 0, 0); }
void windows_webview_destroy(void *raw) { WindowsWebView *shell = (WindowsWebView*)raw; if (shell) { if (shell->controller) shell->controller->Close(); DestroyWindow(shell->hwnd); delete shell; } CoUninitialize(); }
void windows_webview_print(void *raw) { WindowsWebView *shell = (WindowsWebView*)raw; ComPtr<ICoreWebView2_16> print; if (shell->view && SUCCEEDED(shell->view.As(&print))) print->ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER); }
