#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <wrl.h>
#include <WebView2.h>

#include <string>

using namespace Microsoft::WRL;

typedef HRESULT(STDAPICALLTYPE *CreateEnvironmentFn)(
    PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

struct Origin {
  std::wstring scheme;
  std::wstring host;
  INTERNET_PORT port;
};

struct Host {
  HWND hwnd = nullptr;
  std::wstring url;
  Origin origin;
  ComPtr<ICoreWebView2Controller> controller;
  ComPtr<ICoreWebView2> view;
  HRESULT failure = S_OK;
  bool closing = false;
};

static const wchar_t* klass = L"WebviewHost";

static std::wstring argument(int argc, wchar_t** argv, const wchar_t* name) {
  for (int i = 1; i + 1 < argc; i += 2) {
    if (_wcsicmp(argv[i], name) == 0) return argv[i + 1];
  }
  return L"";
}

static bool origin_for(const std::wstring& url, Origin* origin) {
  wchar_t scheme[16] = {};
  wchar_t host[INTERNET_MAX_HOST_NAME_LENGTH] = {};
  URL_COMPONENTSW parts{};
  parts.dwStructSize = sizeof(parts);
  parts.lpszScheme = scheme;
  parts.dwSchemeLength = ARRAYSIZE(scheme);
  parts.lpszHostName = host;
  parts.dwHostNameLength = ARRAYSIZE(host);

  if (!InternetCrackUrlW(url.c_str(), 0, 0, &parts) ||
      parts.dwHostNameLength == 0 ||
      (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS)) {
    return false;
  }

  origin->scheme.assign(scheme, parts.dwSchemeLength);
  origin->host.assign(host, parts.dwHostNameLength);
  origin->port = parts.nPort;
  if (origin->port == 0) {
    origin->port = parts.nScheme == INTERNET_SCHEME_HTTP
                       ? INTERNET_DEFAULT_HTTP_PORT
                       : INTERNET_DEFAULT_HTTPS_PORT;
  }
  return true;
}

static bool same_origin(const std::wstring& url, const Origin& origin) {
  Origin candidate;
  return origin_for(url, &candidate) && candidate.port == origin.port &&
         _wcsicmp(candidate.scheme.c_str(), origin.scheme.c_str()) == 0 &&
         _wcsicmp(candidate.host.c_str(), origin.host.c_str()) == 0;
}

static void resize_controller(Host* host) {
  if (!host->controller) return;
  RECT bounds;
  GetClientRect(host->hwnd, &bounds);
  host->controller->put_Bounds(bounds);
}

static void fail_and_close(Host* host, HRESULT failure) {
  host->failure = failure;
  if (host->hwnd && IsWindow(host->hwnd)) DestroyWindow(host->hwnd);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT message, WPARAM, LPARAM) {
  Host* host = reinterpret_cast<Host*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  switch (message) {
    case WM_SIZE:
      if (host) resize_controller(host);
      return 0;
    case WM_DESTROY:
      if (host) host->closing = true;
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, message, 0, 0);
  }
}

static void open_external(Host* host, const std::wstring& uri) {
  ShellExecuteW(host->hwnd, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static void install_events(Host* host) {
  EventRegistrationToken token{};
  host->view->add_NavigationStarting(
      Callback<ICoreWebView2NavigationStartingEventHandler>(
          [host](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) {
            LPWSTR raw = nullptr;
            if (FAILED(args->get_Uri(&raw))) {
              args->put_Cancel(TRUE);
              return S_OK;
            }
            std::wstring uri(raw ? raw : L"");
            CoTaskMemFree(raw);
            if (!same_origin(uri, host->origin)) {
              open_external(host, uri);
              args->put_Cancel(TRUE);
            }
            return S_OK;
          })
          .Get(),
      &token);
  host->view->add_NewWindowRequested(
      Callback<ICoreWebView2NewWindowRequestedEventHandler>(
          [host](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) {
            LPWSTR raw = nullptr;
            if (FAILED(args->get_Uri(&raw))) {
              args->put_Handled(TRUE);
              return S_OK;
            }
            std::wstring uri(raw ? raw : L"");
            CoTaskMemFree(raw);
            if (same_origin(uri, host->origin)) {
              host->view->Navigate(uri.c_str());
            } else {
              open_external(host, uri);
            }
            args->put_Handled(TRUE);
            return S_OK;
          })
          .Get(),
      &token);
}

int wmain(int argc, wchar_t** argv) {
  std::wstring url = argument(argc, argv, L"--url");
  std::wstring title = argument(argc, argv, L"--title");
  Origin origin;
  if (url.empty() || !origin_for(url, &origin)) return 2;

  int width = _wtoi(argument(argc, argv, L"--width").c_str());
  int height = _wtoi(argument(argc, argv, L"--height").c_str());
  HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return 3;
  bool should_uninitialize = SUCCEEDED(initialized);

  WNDCLASSW wc{};
  wc.lpfnWndProc = wndproc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = klass;
  if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    if (should_uninitialize) CoUninitialize();
    return 3;
  }

  Host host{};
  host.url = url;
  host.origin = origin;
  host.hwnd = CreateWindowExW(
      0, klass, title.empty() ? L"Web App" : title.c_str(), WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, width > 0 ? width : 1024,
      height > 0 ? height : 768, nullptr, nullptr, wc.hInstance, nullptr);
  if (!host.hwnd) {
    if (should_uninitialize) CoUninitialize();
    return 3;
  }
  SetWindowLongPtrW(host.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&host));
  ShowWindow(host.hwnd, SW_SHOW);

  HMODULE loader = LoadLibraryW(L"WebView2Loader.dll");
  auto create = loader ? reinterpret_cast<CreateEnvironmentFn>(
                             GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions"))
                       : nullptr;
  if (!create) {
    if (loader) FreeLibrary(loader);
    DestroyWindow(host.hwnd);
    if (should_uninitialize) CoUninitialize();
    return 4;
  }

  HRESULT created = create(
      nullptr, nullptr, nullptr,
      Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [&host](HRESULT result, ICoreWebView2Environment* environment) {
            if (FAILED(result) || !environment || host.closing) {
              if (FAILED(result)) fail_and_close(&host, result);
              return S_OK;
            }
            HRESULT controller_created = environment->CreateCoreWebView2Controller(
                host.hwnd,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [&host](HRESULT controller_result, ICoreWebView2Controller* controller) {
                      if (FAILED(controller_result) || !controller || host.closing) {
                        if (FAILED(controller_result)) fail_and_close(&host, controller_result);
                        return S_OK;
                      }
                      host.controller = controller;
                      controller->get_CoreWebView2(&host.view);
                      if (!host.view) {
                        fail_and_close(&host, E_FAIL);
                        return S_OK;
                      }
                      resize_controller(&host);
                      install_events(&host);
                      host.view->Navigate(host.url.c_str());
                      return S_OK;
                    })
                    .Get());
            if (FAILED(controller_created)) fail_and_close(&host, controller_created);
            return S_OK;
          })
          .Get());
  if (FAILED(created)) fail_and_close(&host, created);

  MSG message;
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  if (host.controller) host.controller->Close();
  host.view.Reset();
  host.controller.Reset();
  FreeLibrary(loader);
  if (should_uninitialize) CoUninitialize();
  return FAILED(host.failure) ? 5 : 0;
}
