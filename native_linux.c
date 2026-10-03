#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

typedef struct { GtkWidget *window; WebKitWebView *view; gchar *origin; } LinuxWebView;

static gboolean same_origin(const gchar *uri, const gchar *origin) {
  GUri *a = g_uri_parse(uri, G_URI_FLAGS_NONE, NULL);
  GUri *b = g_uri_parse(origin, G_URI_FLAGS_NONE, NULL);
  gboolean equal = a && b && g_strcmp0(g_uri_get_scheme(a), g_uri_get_scheme(b)) == 0 &&
    g_strcmp0(g_uri_get_host(a), g_uri_get_host(b)) == 0 && g_uri_get_port(a) == g_uri_get_port(b);
  if (a) g_uri_unref(a); if (b) g_uri_unref(b); return equal;
}

static gboolean decide_policy(WebKitWebView *view, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, gpointer data) {
  if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) return FALSE;
  WebKitNavigationPolicyDecision *navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
  WebKitURIRequest *request = webkit_navigation_action_get_request(webkit_navigation_policy_decision_get_navigation_action(navigation));
  const gchar *uri = webkit_uri_request_get_uri(request);
  LinuxWebView *shell = data;
  if (same_origin(uri, shell->origin)) return FALSE;
  g_app_info_launch_default_for_uri(uri, NULL, NULL);
  webkit_policy_decision_ignore(decision);
  return TRUE;
}

static gboolean on_close(GtkWidget *window, GdkEvent *event, gpointer data) { gtk_main_quit(); return FALSE; }

void *linux_webview_new(const char *title, int width, int height, const char *url, const char *origin) {
  if (!gtk_init_check(NULL, NULL)) return NULL;
  LinuxWebView *shell = g_new0(LinuxWebView, 1);
  shell->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  shell->view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  shell->origin = g_strdup(origin);
  gtk_window_set_title(GTK_WINDOW(shell->window), title);
  gtk_window_set_default_size(GTK_WINDOW(shell->window), width, height);
  gtk_container_add(GTK_CONTAINER(shell->window), GTK_WIDGET(shell->view));
  g_signal_connect(shell->window, "delete-event", G_CALLBACK(on_close), shell);
  g_signal_connect(shell->view, "decide-policy", G_CALLBACK(decide_policy), shell);
  webkit_web_view_load_uri(shell->view, url);
  gtk_widget_show_all(shell->window);
  return shell;
}

void linux_webview_run(void *raw) { gtk_main(); }
void linux_webview_terminate(void *raw) { gtk_main_quit(); }
void linux_webview_destroy(void *raw) { LinuxWebView *shell = raw; if (!shell) return; gtk_widget_destroy(shell->window); g_free(shell->origin); g_free(shell); }
void linux_webview_print(void *raw) { LinuxWebView *shell = raw; WebKitPrintOperation *op = webkit_print_operation_new(shell->view); webkit_print_operation_run_dialog(op, GTK_WINDOW(shell->window)); g_object_unref(op); }
