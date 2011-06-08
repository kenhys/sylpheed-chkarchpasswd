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

#include "key_add.xpm"
#include "key_delete.xpm"

#include "unlha32.h"
#include "unzip32.h"
#include "7-zip32.h"

#include <glib/gi18n-lib.h>
#include <locale.h>

#define _(String)   dgettext("chkarchpasswd", String)
#define N_(String)  gettext_noop(String)
#define gettext_noop(String) (String)

#define PLUGIN_NAME N_("Check attachment password Plug-in")
#define PLUGIN_DESC N_("Check password of your mail attachment(*.zip), when you send mail.")

static SylPluginInfo info = {
    N_(PLUGIN_NAME),
    "0.4.0",
    "HAYASHI Kentaro",
    N_(PLUGIN_DESC)
};

static void exec_chkarchpasswd_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_chkarchpasswd_menu_cb(void);
static void exec_chkarchpasswd_onoff_cb(void);
static void compose_created_cb(GObject *obj, gpointer compose);
static void compose_destroy_cb(GObject *obj, gpointer compose);
static gboolean compose_send_cb(GObject *obj, gpointer compose,
                                gint compose_mode, gint send_mode,
                                const gchar *msg_file, GSList *to_list);
typedef int WINAPI (*WINAPI_SEVENZIP)(const HWND _hwnd, LPCSTR _szCmdLine, LPSTR _szOutput, const DWORD _dwSize);
typedef BOOL WINAPI (*WINAPI_SEVENZIPSETUNICODEMODE)(BOOL _bUnicode);

static HANDLE g_hdll = NULL;
static WINAPI_SEVENZIP hZip = NULL;
static WINAPI_SEVENZIPSETUNICODEMODE hUniZip = NULL;

static GtkWidget *g_plugin_on = NULL;
static GtkWidget *g_plugin_off = NULL;
static GtkWidget *g_onoff_switch = NULL;
static GtkTooltips *g_tooltip = NULL;

void plugin_load(void)
{
  debug_print("[PLUGIN] initializing chkarchpasswd plug-in\n");

  g_hdll = LoadLibrary(L"7-zip32.dll");
  if (g_hdll==NULL){
    debug_print("failed to load 7-zip32.dll\n");
  }else{
    hZip = (WINAPI_SEVENZIP)GetProcAddress(g_hdll, "SevenZip");
    hUniZip = (WINAPI_SEVENZIPSETUNICODEMODE)GetProcAddress(g_hdll, "SevenZipSetUnicodeMode");

    if (hUniZip){
      hUniZip(TRUE);
    }
  }

  syl_init_gettext("chkarchpasswd", "lib/locale");

  debug_print(gettext(PLUGIN_NAME));
  debug_print(dgettext("chkarchpasswd", PLUGIN_DESC));

   info.name = g_strdup(_(PLUGIN_NAME));
  info.description = g_strdup(_(PLUGIN_DESC));
  
  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("Chkarchpasswd Option"), exec_chkarchpasswd_menu_cb, NULL);

  syl_plugin_signal_connect("compose-created", G_CALLBACK(compose_created_cb), NULL);

  syl_plugin_signal_connect("compose-destroy", G_CALLBACK(compose_destroy_cb), NULL);

  syl_plugin_signal_connect("compose-send", G_CALLBACK(compose_send_cb), NULL);

  GtkWidget *mainwin = syl_plugin_main_window_get();
  GtkWidget *statusbar = syl_plugin_main_window_get_statusbar();
  GtkWidget *plugin_box = gtk_hbox_new(FALSE, 0);

    GdkPixbuf* on_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)key_add);
    g_plugin_on=gtk_image_new_from_pixbuf(on_pixbuf);
    /*g_plugin_on = gtk_label_new(_("AF ON"));*/
    
    GdkPixbuf* off_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)key_delete);
    g_plugin_off=gtk_image_new_from_pixbuf(off_pixbuf);
    /*g_plugin_off = gtk_label_new(_("AF OFF"));*/

    gtk_box_pack_start(GTK_BOX(plugin_box), g_plugin_on, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(plugin_box), g_plugin_off, FALSE, FALSE, 0);
    
    g_tooltip = gtk_tooltips_new();
    
    g_onoff_switch = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(g_onoff_switch), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS(g_onoff_switch, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(g_onoff_switch, 20, 20);

    gtk_container_add(GTK_CONTAINER(g_onoff_switch), plugin_box);
	g_signal_connect(G_OBJECT(g_onoff_switch), "clicked",
                     G_CALLBACK(exec_chkarchpasswd_onoff_cb), mainwin);
    gtk_box_pack_start(GTK_BOX(statusbar), g_onoff_switch, FALSE, FALSE, 0);

    gtk_widget_show_all(g_onoff_switch);

    if (g_hdll){
        gtk_widget_hide(g_plugin_off);
        gtk_widget_show(g_plugin_on);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("Chkpasswd is enabled."),
			 NULL);
    }else {
        gtk_widget_hide(g_plugin_on);
        gtk_widget_show(g_plugin_off);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("Chkpasswd is disabled."),
			 NULL);
    }

    debug_print("[PLUGIN] chkarchpasswd_tool plug-in loading done.\n");
}

void plugin_unload(void)
{
  debug_print("chkarchpasswd_tool plug-in unloaded.\n");
  if (g_hdll!=NULL){
      FreeLibrary(g_hdll);
  }
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

#if 0
  if (g_enable != TRUE){
    syl_plugin_alertpanel_message(_("Check Archive Password"), _("chkarchpasswd plugin is enabled."), ALERT_NOTICE);
    debug_print("[PLUGIN] enable exec_chkarchpasswd_cb\n");
    g_enable=TRUE;
  }else{
    syl_plugin_alertpanel_message(_("Check Archive Password"), _("chkarchpasswd plugin is disabled."), ALERT_NOTICE);
    debug_print("[PLUGIN] disable exec_chkarchpasswd_cb\n");
    g_enable=FALSE;
  }
#endif
}

static void exec_chkarchpasswd_onoff_cb(void)
{

    if (g_enable != TRUE){
        syl_plugin_alertpanel_message(_("Chkarchpasswd"), _("chkarchpasswd plugin is enabled."), ALERT_NOTICE);
        g_enable=TRUE;
        gtk_widget_hide(g_plugin_off);
        gtk_widget_show(g_plugin_on);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("Chkarchpasswd is enabled. Click the icon to disable plugin."),
			 NULL);
    }else{
        syl_plugin_alertpanel_message(_("Chkarchpasswd"), _("chkarchpasswd plugin is disabled."), ALERT_NOTICE);
        g_enable=FALSE;
        gtk_widget_hide(g_plugin_on);
        gtk_widget_show(g_plugin_off);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("Chkarchpasswd is disabled. Click the icon to enable plugin."),
			 NULL);
    }
}

static Compose* g_compose = NULL;

void compose_created_cb(GObject *obj, gpointer compose)
{
  g_compose = (Compose*)compose;
  GtkWidget *toolbar = g_compose->toolbar;
  GtkToolItem *toolitem = NULL;


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
  
}

void compose_destroy_cb(GObject *obj, gpointer compose)
{
  debug_print("[PLUGIN] compose_destroy_cb is called.\n");

  /**/
}

static gboolean compose_send_cb(GObject *obj, gpointer compose,
                                gint compose_mode, gint send_mode,
                                const gchar *msg_file, GSList *to_list)
{

  debug_print("[PLUGIN] compose_send_cb is called.\n");

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
  
  gint npasstotal = 0;
  gint npassok = 0;
  gint val = 0;
  gboolean bpasswd = FALSE;

  debug_print("text:%p\n", g_compose->text);
  GtkTextView *text = GTK_TEXT_VIEW(g_compose->text);
  if (text==NULL){
      debug_print("text is NULL\n");
      return TRUE;
  }else{
      debug_print("text:%p\n", text);
  }
  GtkTextBuffer *buffer;
  buffer = gtk_text_view_get_buffer(text);
  GtkTextIter tsiter, teiter;
  if (buffer == NULL){
      debug_print("buffer is NULL\n");
      return TRUE;
  }else{
      debug_print("buffer:%p\n", buffer);
  }
  gtk_text_buffer_get_bounds(buffer, &tsiter, &teiter);
  gchar *pwtext = gtk_text_buffer_get_text(buffer, &tsiter, &teiter, TRUE);
  if (pwtext==NULL){
      debug_print("pwtext is NULL\n");
      return TRUE;
  }else{
      debug_print("pwtext:%p\n", pwtext);
  }
  GScanner *gscan = g_scanner_new(NULL);
  gscan->config->scan_identifier_1char=TRUE;
  g_scanner_input_text(gscan, pwtext, strlen(pwtext));

  debug_print("scan loop\n");
  GTokenValue gvalue;
  GList *pwlist = NULL;
  int index=0;
  while( g_scanner_eof(gscan) != TRUE){
    GTokenType gtoken = g_scanner_get_next_token (gscan);
    switch (gtoken){
    case G_TOKEN_CHAR:
      gvalue = g_scanner_cur_value(gscan);
      g_print("char:%s\n", gvalue.v_identifier);
      break;
    case G_TOKEN_IDENTIFIER:
      gvalue = g_scanner_cur_value(gscan);
      for (index = 0; index<strlen(gvalue.v_identifier);index++){
        if (index == strlen(gvalue.v_identifier)-1 &&
            gvalue.v_identifier[index] & 0xffff0000){
          gvalue.v_identifier[index]= '\0';
        }else{
          g_print("char :%0x08\n", gvalue.v_identifier[index]);
        }
      }
      g_print("identifier:%s\n", gvalue.v_identifier);
      pwlist = g_list_append(pwlist, g_strdup(gvalue.v_identifier));
      break;
    default:
      break;
    }
  }
  
  /* get password candidate from text */
  gchar *msg=NULL;

#if 0
  GtkTreeView *treeview = GTK_TREE_VIEW(g_compose->attach_treeview);
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Author",
                                                     renderer,
                                                     "text", 0,
                                                     "background", 1,
                                                     "background-set", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
#endif
  
  for (valid = gtk_tree_model_get_iter_first(model, &iter); valid;
	     valid = gtk_tree_model_iter_next(model, &iter)) {
      gtk_tree_model_get(model, &iter, 3, &ainfo, -1);
      /* see 3 as COL_ATTACH_INFO in compose.c */
      if (memcmp("application/zip", ainfo->content_type, sizeof("application/zip")) == 0 ||
          memcmp("application/octet-stream", ainfo->content_type, sizeof("application/octet-stream")) == 0) {
          npasstotal += 1;
          debug_print("file:%s\n", ainfo->file);
          debug_print("content_type:%s\n", ainfo->content_type);
          debug_print("name:%s\n", ainfo->name);
          debug_print("size:%d\n", ainfo->size);


#if 0
          gchar *msg=g_strdup_printf("添付ファイル(%s)のパスワードを入力してください。", ainfo->name);
          gchar *passwd= syl_plugin_input_dialog("パスワード", msg, "guest");
          
          if (passwd==NULL){
              msg=g_strdup_printf("添付ファイル(%s)のパスワードチェックをスキップします。", ainfo->name);
              syl_plugin_alertpanel("情報", msg, GTK_STOCK_OK,NULL, NULL);
              continue;
          }
#endif
          bpasswd=FALSE;
          char buf[1024];
          DWORD dwSize = 0;
          /* input password for archive */
          gchar *com = g_strdup_printf("x \"%s\" -aoa -p\"\" -hide -o\"%s\" -r",
                                       ainfo->file, path);
          nblank = hZip(NULL, com, buf, dwSize);
          g_print("%s blank password result:%08x\n", ainfo->name,nblank);

#if 0
          com = g_strdup_printf("x \"%s\" -aoa -p\"%s\" -hide -o\"%s\" -r",
                                ainfo->file, passwd, path);
#endif
          com = g_strdup_printf("x \"%s\" -aoa -p\"test\" -hide -o\"%s\" -r",
                                ainfo->file, path);
          npasswd = hZip(NULL, com, buf, dwSize);
          g_print("%s invalid password result:%08x\n",ainfo->name, npasswd);

          if (nblank == 0x0000800a && npasswd == 0x00000000){
              /* passwd ok */
              g_print("%s valid password result\n", ainfo->name);
              bpasswd = TRUE;
          } else if (nblank == 0x0000800a && npasswd == 0x0000800a){
              /* passwd but not match */
              g_print("%s invalid password result\n", ainfo->name);
              bpasswd = TRUE;
              /* does not care invalid password */
          } else if (nblank == 0x00000000 && npasswd == 0x00000000){
              /* no password */
              g_print("%s no password result\n", ainfo->name);
          }
          if (bpasswd==TRUE){
              /* check pwlist */
              guint pwidx = 0;
              gboolean bmatch = FALSE;
              for (pwidx = 0; pwidx < g_list_length(pwlist); pwidx++){
                gchar *passwd = g_list_nth_data(pwlist, pwidx);
                com = g_strdup_printf("x \"%s\" -aoa -p\"%s\" -hide -o\"%s\" -r",
                                      ainfo->file, passwd, path);
                g_print("check password %s for %s\n",passwd, ainfo->name);
                nblank = hZip(NULL, com, buf, dwSize);
                if (nblank == 0x00000000){
                  g_print("%s blank password result:%08x\n", ainfo->name,nblank);
                  bmatch = TRUE;
                  msg=g_strdup_printf(_("attachment(%s) password (%s) is described in mail body."),
                                      ainfo->name, passwd);
                  syl_plugin_alertpanel("", msg, GTK_STOCK_OK,NULL, NULL);
                  return FALSE;
                }
              }
              npassok += 1;
          }
      }else{
      }
  }
  gboolean bsend = FALSE;
  if ( npasstotal > 0 && npassok == npasstotal){
#if 0
    syl_plugin_alertpanel("", _("パスワードが設定されていることを確認しました。メールを送信します。"),
                                    GTK_STOCK_OK,NULL, NULL);
#endif
    bsend = TRUE;
  }else if (npasstotal > 0 && npassok < npasstotal){
    gint val = syl_plugin_alertpanel("", _("password is empty. do you want to send email without password?"),
                                     GTK_STOCK_YES, GTK_STOCK_NO, NULL);
    if (val != 0){
      return FALSE;
    }
    val = syl_plugin_alertpanel("", _("password is empty. do you really want to send this attachment without password?"),
                                GTK_STOCK_NO, GTK_STOCK_YES, NULL);
    if (val == 0){
      return FALSE;
    }
    bsend = TRUE;
  }else if (npasstotal == 0){
      bsend = TRUE;
  }else{
      syl_plugin_alertpanel("", "abort to sending email.",
                            GTK_STOCK_OK, NULL, NULL);
  }
  g_print("compose_send_cb:%s\n", bsend ? "TRUE" : "FALSE");
  return bsend;
}

