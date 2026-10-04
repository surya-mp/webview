#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include <WebView2.h>
#include <string>

using namespace Microsoft::WRL;
typedef HRESULT (STDAPICALLTYPE *CreateEnvironmentFn)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

struct Host { HWND hwnd; std::wstring url, origin; ComPtr<ICoreWebView2Controller> controller; ComPtr<ICoreWebView2> view; HRESULT failure = S_OK; };
static const wchar_t *klass = L"WebviewHost";

static std::wstring argument(int argc, wchar_t **argv, const wchar_t *name) {
  for (int i = 1; i + 1 < argc; i += 2) if (_wcsicmp(argv[i], name) == 0) return argv[i + 1];
  return L"";
}
static std::wstring origin_for(const std::wstring &url) {
  size_t scheme = url.find(L"://");
  if (scheme == std::wstring::npos) return L"";
  size_t end = url.find(L'/', scheme + 3);
  return url.substr(0, end == std::wstring::npos ? url.size() : end);
}
static bool same_origin(const std::wstring &url, const std::wstring &origin) {
  return url.size() >= origin.size() && _wcsnicmp(url.c_str(), origin.c_str(), origin.size()) == 0 && (url.size() == origin.size() || url[origin.size()] == L'/');
}
static LRESULT CALLBACK wndproc(HWND hwnd, UINT message, WPARAM, LPARAM) {
  Host *host = reinterpret_cast<Host*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (message == WM_SIZE && host && host->controller) { RECT bounds; GetClientRect(hwnd, &bounds); host->controller->put_Bounds(bounds); return 0; }
  if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
  return DefWindowProcW(hwnd, message, 0, 0);
}
static void events(Host *host) {
  EventRegistrationToken token{};
  host->view->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>([host](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs *args) {
    LPWSTR raw = nullptr; args->get_Uri(&raw); std::wstring uri(raw ? raw : L""); CoTaskMemFree(raw);
    if (!same_origin(uri, host->origin)) { ShellExecuteW(host->hwnd, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL); args->put_Cancel(TRUE); }
    return S_OK;
  }).Get(), &token);
  host->view->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>([host](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs *args) {
    LPWSTR raw = nullptr; args->get_Uri(&raw); std::wstring uri(raw ? raw : L""); CoTaskMemFree(raw);
    if (same_origin(uri, host->origin)) host->view->Navigate(uri.c_str()); else ShellExecuteW(host->hwnd, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    args->put_Handled(TRUE); return S_OK;
  }).Get(), &token);
}

int wmain(int argc, wchar_t **argv) {
  std::wstring url = argument(argc, argv, L"--url"), title = argument(argc, argv, L"--title");
  if (url.empty() || origin_for(url).empty()) return 2;
  int width = _wtoi(argument(argc, argv, L"--width").c_str());
  int height = _wtoi(argument(argc, argv, L"--height").c_str());
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  WNDCLASSW wc{}; wc.lpfnWndProc = wndproc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = klass; RegisterClassW(&wc);
  Host host{}; host.url = url; host.origin = origin_for(url);
  host.hwnd = CreateWindowExW(0, klass, title.empty() ? L"Web App" : title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width > 0 ? width : 1024, height > 0 ? height : 768, nullptr, nullptr, wc.hInstance, nullptr);
  if (!host.hwnd) return 3;
  SetWindowLongPtrW(host.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&host)); ShowWindow(host.hwnd, SW_SHOW);
  HMODULE loader = LoadLibraryW(L"WebView2Loader.dll");
  auto create = loader ? reinterpret_cast<CreateEnvironmentFn>(GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions")) : nullptr;
  if (!create) return 4;
  create(nullptr, nullptr, nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([&host](HRESULT result, ICoreWebView2Environment *environment) {
    if (FAILED(result)) { host.failure = result; PostQuitMessage(0); return S_OK; }
    environment->CreateCoreWebView2Controller(host.hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([&host](HRESULT result, ICoreWebView2Controller *controller) {
      if (FAILED(result)) { host.failure = result; PostQuitMessage(0); return S_OK; }
      host.controller = controller; controller->get_CoreWebView2(&host.view); events(&host); host.view->Navigate(host.url.c_str()); return S_OK;
    }).Get()); return S_OK;
  }).Get());
  MSG message; while (GetMessageW(&message, nullptr, 0, 0)) { TranslateMessage(&message); DispatchMessageW(&message); }
  if (host.controller) host.controller->Close();
  CoUninitialize();
  return FAILED(host.failure) ? 5 : 0;
}
