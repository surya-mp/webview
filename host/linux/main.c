#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

typedef struct {
  GtkWidget *window;
  WebKitWebView *view;
  gchar *origin;
} Host;

static const char *value(int argc, char **argv, const char *name) {
  for (int i = 1; i + 1 < argc; i += 2) if (g_strcmp0(argv[i], name) == 0) return argv[i + 1];
  return NULL;
}

static gchar *origin_for(const char *uri) {
  GUri *parsed = g_uri_parse(uri, G_URI_FLAGS_NONE, NULL);
  if (!parsed) return NULL;
  gchar *origin = g_strdup_printf("%s://%s:%d", g_uri_get_scheme(parsed), g_uri_get_host(parsed), g_uri_get_port(parsed));
  g_uri_unref(parsed);
  return origin;
}

static gboolean same_origin(const char *uri, const char *origin) {
  gchar *candidate = origin_for(uri);
  gboolean same = candidate && g_strcmp0(candidate, origin) == 0;
  g_free(candidate);
  return same;
}

static gboolean decide_policy(WebKitWebView *view, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, gpointer data) {
  if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) return FALSE;
  WebKitNavigationPolicyDecision *navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
  WebKitURIRequest *request = webkit_navigation_action_get_request(webkit_navigation_policy_decision_get_navigation_action(navigation));
  const char *uri = webkit_uri_request_get_uri(request);
  Host *host = data;
  if (same_origin(uri, host->origin)) return FALSE;
  g_app_info_launch_default_for_uri(uri, NULL, NULL);
  webkit_policy_decision_ignore(decision);
  return TRUE;
}

static gboolean close_window(GtkWidget *window, GdkEvent *event, gpointer data) {
  gtk_main_quit();
  return FALSE;
}

int main(int argc, char **argv) {
  const char *url = value(argc, argv, "--url");
  const char *title = value(argc, argv, "--title");
  const char *width = value(argc, argv, "--width");
  const char *height = value(argc, argv, "--height");
  if (!url || !g_uri_is_valid(url, G_URI_FLAGS_NONE, NULL)) return 2;
  if (!gtk_init_check(NULL, NULL)) return 3;

  Host host = {0};
  host.origin = origin_for(url);
  if (!host.origin) return 2;
  host.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  host.view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  gtk_window_set_title(GTK_WINDOW(host.window), title ? title : "Web App");
  gtk_window_set_default_size(GTK_WINDOW(host.window), width ? atoi(width) : 1024, height ? atoi(height) : 768);
  gtk_container_add(GTK_CONTAINER(host.window), GTK_WIDGET(host.view));
  g_signal_connect(host.window, "delete-event", G_CALLBACK(close_window), NULL);
  g_signal_connect(host.view, "decide-policy", G_CALLBACK(decide_policy), &host);
  webkit_web_view_load_uri(host.view, url);
  gtk_widget_show_all(host.window);
  gtk_main();
  g_free(host.origin);
  return 0;
}
