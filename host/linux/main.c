#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <errno.h>
#include <limits.h>

typedef struct {
  gchar *scheme;
  gchar *host;
  gint port;
} Origin;

typedef struct {
  GtkWidget *window;
  WebKitWebView *view;
  Origin origin;
} Host;

static const char *value(int argc, char **argv, const char *name) {
  for (int i = 1; i + 1 < argc; i += 2) {
    if (g_strcmp0(argv[i], name) == 0) return argv[i + 1];
  }
  return NULL;
}

static gboolean origin_for(const char *uri, Origin *origin) {
  if (!uri) return FALSE;
  GUri *parsed = g_uri_parse(uri, G_URI_FLAGS_NONE, NULL);
  if (!parsed) return FALSE;

  const gchar *scheme = g_uri_get_scheme(parsed);
  const gchar *host = g_uri_get_host(parsed);
  gboolean valid = scheme && host && *host &&
      (g_ascii_strcasecmp(scheme, "http") == 0 ||
       g_ascii_strcasecmp(scheme, "https") == 0);
  if (!valid) {
    g_uri_unref(parsed);
    return FALSE;
  }

  origin->scheme = g_ascii_strdown(scheme, -1);
  origin->host = g_ascii_strdown(host, -1);
  origin->port = g_uri_get_port(parsed);
  if (origin->port == -1) {
    origin->port = g_ascii_strcasecmp(scheme, "http") == 0 ? 80 : 443;
  }
  g_uri_unref(parsed);
  return TRUE;
}

static void clear_origin(Origin *origin) {
  g_free(origin->scheme);
  g_free(origin->host);
  origin->scheme = NULL;
  origin->host = NULL;
}

static gboolean same_origin(const char *uri, const Origin *origin) {
  Origin candidate = {0};
  gboolean same = origin_for(uri, &candidate) && candidate.port == origin->port &&
      g_strcmp0(candidate.scheme, origin->scheme) == 0 &&
      g_strcmp0(candidate.host, origin->host) == 0;
  clear_origin(&candidate);
  return same;
}

static void open_external(const char *uri) {
  GError *error = NULL;
  g_app_info_launch_default_for_uri(uri, NULL, &error);
  g_clear_error(&error);
}

static gboolean decide_policy(
    WebKitWebView *, WebKitPolicyDecision *decision,
    WebKitPolicyDecisionType type, gpointer data) {
  if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
      type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
    return FALSE;
  }

  WebKitNavigationPolicyDecision *navigation =
      WEBKIT_NAVIGATION_POLICY_DECISION(decision);
  WebKitURIRequest *request = webkit_navigation_action_get_request(
      webkit_navigation_policy_decision_get_navigation_action(navigation));
  const char *uri = webkit_uri_request_get_uri(request);
  Host *host = data;
  if (same_origin(uri, &host->origin)) {
    if (type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
      webkit_web_view_load_uri(host->view, uri);
      webkit_policy_decision_ignore(decision);
      return TRUE;
    }
    return FALSE;
  }

  open_external(uri);
  webkit_policy_decision_ignore(decision);
  return TRUE;
}

static gboolean close_window(GtkWidget *, GdkEvent *, gpointer) {
  gtk_main_quit();
  return FALSE;
}

static gint dimension(const char *raw, gint fallback) {
  if (!raw || !*raw) return fallback;
  errno = 0;
  gchar *end = NULL;
  gint64 parsed = g_ascii_strtoll(raw, &end, 10);
  if (errno != 0 || end == raw || *end != '\0' || parsed <= 0 || parsed > G_MAXINT) {
    return fallback;
  }
  return (gint)parsed;
}

int main(int argc, char **argv) {
  const char *url = value(argc, argv, "--url");
  const char *title = value(argc, argv, "--title");
  const char *width = value(argc, argv, "--width");
  const char *height = value(argc, argv, "--height");
  Host host = {0};

  if (!url || !origin_for(url, &host.origin)) return 2;
  if (!gtk_init_check(NULL, NULL)) {
    clear_origin(&host.origin);
    return 3;
  }

  host.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  host.view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  WebKitSettings *settings = webkit_web_view_get_settings(host.view);
  webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);
  gtk_window_set_title(GTK_WINDOW(host.window), title ? title : "Web App");
  gtk_window_set_default_size(
      GTK_WINDOW(host.window), dimension(width, 1024), dimension(height, 768));
  gtk_container_add(GTK_CONTAINER(host.window), GTK_WIDGET(host.view));
  g_signal_connect(host.window, "delete-event", G_CALLBACK(close_window), NULL);
  g_signal_connect(host.view, "decide-policy", G_CALLBACK(decide_policy), &host);
  webkit_web_view_load_uri(host.view, url);
  gtk_widget_show_all(host.window);
  gtk_main();
  clear_origin(&host.origin);
  return 0;
}
