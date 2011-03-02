/*
 * Check archive password Plug-in -- check your attachment archive is
 * password encrypted or not when you send mail.
 * Copyright (C) 2011 HAYASHI Kentaro <kenhys@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "defs.h"

#include <glib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

#include "sylmain.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "utils.h"
#include "alertpanel.h"
#include "compose.h"

#include "unlha32.h"
#include "unzip32.h"
#include "7-zip32.h"

#include <glib/gi18n-lib.h>

static SylPluginInfo info = {
  "Check attachment password Plug-in",
  "0.1.0",
  "HAYASHI Kentaro",
  "Check password of your mail attachment(*.zip), when you send mail."
};

static void exec_chkarchpasswd_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_chkarchpasswd_menu_cb(void);
static void compose_created_cb(GObject *obj, gpointer compose);
static void compose_destroy_cb(GObject *obj, gpointer compose);
static gboolean mycompose_send_cb(GObject *obj, gpointer compose);
static gboolean mycompose_sendl_cb(GObject *obj, gpointer compose);

typedef int WINAPI (*WINAPI_SEVENZIP)(const HWND _hwnd, LPCSTR _szCmdLine, LPSTR _szOutput, const DWORD _dwSize);

static HANDLE g_hdll = NULL;
static WINAPI_SEVENZIP hZip = NULL;

void plugin_load(void)
{
  debug_print("[PLUGIN] initializing chkarchpasswd plug-in\n");

  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("Toggle chkarchpasswd"), exec_chkarchpasswd_menu_cb, NULL);

  syl_plugin_signal_connect("compose-created", G_CALLBACK(compose_created_cb), NULL);

  syl_plugin_signal_connect("compose-destroy", G_CALLBACK(compose_destroy_cb), NULL);

#if 0
  syl_plugin_signal_connect("compose-send", G_CALLBACK(compose_send_cb), NULL);

  syl_plugin_signal_connect("compose-sendl", G_CALLBACK(compose_sendl_cb), NULL);
#endif
  
  debug_print("[PLUGIN] chkarchpasswd_tool plug-in loading done.\n");
}

void plugin_unload(void)
{
  debug_print("chkarchpasswd_tool plug-in unloaded.\n");
}

SylPluginInfo *plugin_info(void)
{
  return &info;
}

gint plugin_interface_version(void)
{
  return SYL_PLUGIN_INTERFACE_VERSION;
}

static gboolean g_enable = FALSE;

static void exec_chkarchpasswd_menu_cb(void)
{
  debug_print("[PLUGIN] exec_chkarchpasswd_menu_cb is called.\n");

  if (g_enable != TRUE){
    syl_plugin_alertpanel_message(_("Check Archive Password"), _("chkarchpasswd plugin is enabled."), ALERT_NOTICE);
    debug_print("[PLUGIN] enable exec_chkarchpasswd_cb\n");
    g_enable=TRUE;
  }else{
    syl_plugin_alertpanel_message(_("Check Archive Password"), _("chkarchpasswd plugin is disabled."), ALERT_NOTICE);
    debug_print("[PLUGIN] disable exec_chkarchpasswd_cb\n");
    g_enable=FALSE;
  }
}

void exec_chkarchpasswd_cb(GObject *obj, FolderItem *item, const gchar *file, guint num)
{
  debug_print("[PLUGIN] exec_chkarchpasswd_cb\n");
  debug_print("[PLUGIN] file:%s\n", file);
  debug_print("[PLUGIN] guint num:%d\n", num);

  if (g_enable!=TRUE){
    return;
  }
#if 0
  if (item->stype != F_NORMAL && item->stype != F_INBOX){
    debug_print("[PLUGIN] neither F_NORMAL nor F_INBOX\n");
    return;
  }
    
  PrefsAccount *ac = (PrefsAccount*)account_get_default();
  debug_print("[PLUGIN] check account address\n");
  g_return_if_fail(ac != NULL);
    
  debug_print("[PLUGIN] account address:%s\n", ac->address);
  syl_plugin_send_message_set_forward_flags(ac->address);
  debug_print("[PLUGIN] syl_plugin_send_message_set_forward_flags done.\n");

  FILE *fp;
  gchar *rcpath;
  GSList* to_list=NULL;

  gchar buf[PREFSBUFSIZE];
  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, "chkarchpasswdrc", NULL);

  debug_print("[PLUGIN] rcpath:%s\n", rcpath);
  if ((fp = g_fopen(rcpath, "rb")) == NULL) {
    if (ENOENT != errno) FILE_OP_ERROR(rcpath, "fopen");
    g_free(rcpath);
    return;
  }
  g_free(rcpath);

  debug_print("[PLUGIN] read rcpath:%s\n", rcpath);
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    g_strstrip(buf);
    if (buf[0] == '\0') continue;
    to_list = address_list_append(to_list, buf);
  }
  fclose(fp);

  debug_print("[PLUGIN] check to_list\n");
  g_return_if_fail(to_list != NULL);

  syl_plugin_send_message(file, ac, to_list);
#endif
  debug_print("[PLUGIN] syl_plugin_send_message done.\n");
}

#if 0
static gboolean send_button_press(GtkWidget	*widget,
                                  GdkEventButton	*event,
                                  gpointer	 data);

static gulong g_hook_id = 0;

/* global button-press-event signal id */
static guint g_signal_id = 0;

static GtkToolItem *g_sendbtn = NULL;

/* original button handler id. */
static gulong g_sendbtn_id = 0;
#endif

static Compose* g_compose = NULL;

void compose_created_cb(GObject *obj, gpointer compose)
{
  g_compose = (Compose*)compose;
  GtkWidget *toolbar = g_compose->toolbar;
  GtkToolItem *toolitem = NULL;

  g_hdll = LoadLibrary(L"7-zip32.dll");
  if (g_hdll==NULL){
      debug_print("failed to load 7-zip32.dll\n");
      return;
  }
  hZip = (WINAPI_SEVENZIP)GetProcAddress(g_hdll, "SevenZip");

#if 0
  /* add check archive button for testing. */
  /* GtkWidget *icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_LARGE_TOOLBAR);*/
  GtkWidget *icon = gtk_image_new_from_stock(GTK_STOCK_CDROM, GTK_ICON_SIZE_LARGE_TOOLBAR);  
  GtkToolItem *toolitem = gtk_tool_button_new(icon, "check attachement");
  GtkTooltips *tooltips = gtk_tooltips_new();
  gtk_tool_item_set_tooltip(toolitem, tooltips, "check your mail attachement", "");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
  
  g_signal_connect(G_OBJECT(toolitem), "clicked",
                   G_CALLBACK(check_attachement_cb), compose);
  g_signal_connect(G_OBJECT(GTK_BIN(toolitem)->child),
                   "button_press_event",
                   G_CALLBACK(check_attachement_cb), compose);
  gtk_widget_show_all(toolbar);
#endif
  
  /* remove orignal callback from plugin. dirty hack. */
  guint signal_id;
  guint nmatch = 0;
  int btn_index = 0;
  for (btn_index = 0; btn_index <= 1; btn_index++){
      toolitem = gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), btn_index);

      signal_id = g_signal_lookup("button_press_event", GTK_TYPE_BUTTON);
      debug_print("signal_id:%d\n", signal_id);

      nmatch += g_signal_handlers_disconnect_matched(G_OBJECT(GTK_BIN(toolitem)->child),
                                                          G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA,
                                                          signal_id, 0,
                                                          NULL, NULL, compose);
      debug_print("removed:%d\n", nmatch);

      signal_id = g_signal_lookup("clicked", GTK_TYPE_TOOL_BUTTON);
      debug_print("signal_id:%d\n", signal_id);

      nmatch += g_signal_handlers_disconnect_matched(G_OBJECT(toolitem),
                                                    G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA,
                                                    signal_id, 0,
                                                    NULL, NULL, compose);
      debug_print("removed:%d\n", nmatch);
  }
  
  if (nmatch == 4){
      toolitem = gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 0);
      
      g_signal_connect(G_OBJECT(toolitem), "clicked",
                   G_CALLBACK(mycompose_send_cb), compose);
      g_signal_connect(G_OBJECT(GTK_BIN(toolitem)->child),
                   "button_press_event",
                   G_CALLBACK(mycompose_send_cb), compose);
      toolitem = gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 1);
      
      g_signal_connect(G_OBJECT(toolitem), "clicked",
                   G_CALLBACK(mycompose_sendl_cb), compose);
      g_signal_connect(G_OBJECT(GTK_BIN(toolitem)->child),
                   "button_press_event",
                   G_CALLBACK(mycompose_sendl_cb), compose);
  }
  debug_print("Compose*:%p\n", g_compose);
  debug_print("compose:%p\n", compose);
  debug_print("model:%p\n", GTK_TREE_MODEL(g_compose->attach_store));
}

void compose_destroy_cb(GObject *obj, gpointer compose)
{
  debug_print("[PLUGIN] compose_destroy_cb is called.\n");

  /**/
  if (g_hdll!=NULL){
      FreeLibrary(g_hdll);
  }
}

gboolean mycompose_send_cb(GObject *obj, gpointer compose)
{
  debug_print("[PLUGIN] mycompose_send_cb is called.\n");

  debug_print("Compose* g_compose:%p\n", g_compose);
  debug_print("gpointer compose:%p\n", compose);

  GtkTreeModel *model = GTK_TREE_MODEL(g_compose->attach_store);
  GtkTreeIter iter;
  AttachInfo *ainfo;
  gboolean valid;

  debug_print("model:%p\n", model);
  gint nblank;
  gint npasswd;
  
  gchar* path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, PLUGIN_DIR,G_DIR_SEPARATOR_S, "chkarchpasswd", NULL);
  g_mkdir_with_parents(path, 0);
  
  for (valid = gtk_tree_model_get_iter_first(model, &iter); valid;
	     valid = gtk_tree_model_iter_next(model, &iter)) {
      gtk_tree_model_get(model, &iter, 3, &ainfo, -1);
      /* see 3 as COL_ATTACH_INFO in compose.c */
      if (memcmp("application/zip", ainfo->content_type, sizeof("application/zip")) == 0) {
          debug_print("file:%s\n", ainfo->file);
          debug_print("content_type:%s\n", ainfo->content_type);
          debug_print("name:%s\n", ainfo->name);
          debug_print("size:%d\n", ainfo->size);

          gchar *passwd= syl_plugin_input_dialog("Password", "what's archive password?", "guest");
          
          char buf[1024];
          DWORD dwSize = 0;
          /* input password for archive */
          gchar *com = g_strdup_printf("x \"%s\" -aoa -p\"\" -hide -o\"%s\" -r",
                                       ainfo->file, path);
          nblank = hZip(NULL, com, buf, dwSize);
          g_print("%s blank password result:%08x\n", ainfo->name,nblank);

          com = g_strdup_printf("x \"%s\" -aoa -p\"%s\" -hide -o\"%s\" -r",
                                ainfo->file, passwd, path);
          npasswd = hZip(NULL, com, buf, dwSize);
          g_print("%s invalid password result:%08x\n",ainfo->name, npasswd);

          if (nblank == 0x0000800a && npasswd == 0x00000000){
              /* passwd ok */
              g_print("%s valid password result\n", ainfo->name);
          } else if (nblank == 0x00000000 && npasswd == 0x00000000){
              /* no password */
              g_print("%s no password result\n", ainfo->name);
          }
      }
  }

  /* stop furthor event handling. */
  return TRUE;
}

gboolean mycompose_sendl_cb(GObject *obj, gpointer compose)
{
  debug_print("[PLUGIN] mycompose_sendl_cb is called.\n");
  /* stop furthor event handling. */
  return TRUE;
}

#if 0
gboolean send_button_press(GtkWidget	*widget,
                           GdkEventButton	*event,
                           gpointer	 data)
{
  debug_print("[PLUGIN] send_button_press.\n");

  /* stop furthor event handling. */
  return TRUE;
}
#endif
