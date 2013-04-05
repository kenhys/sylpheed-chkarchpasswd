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

#include "../res/key_add.xpm"
#include "../res/key_delete.xpm"

#if defined(G_OS_WIN32)
#include "UNLHA32.H"
#include "UNZIP32.H"
#include "7-zip32.h"
#endif

#include <glib/gi18n-lib.h>
#include <locale.h>

#define CHKARCHPASSWD "chkarchpasswd"
#define CHKARCHPASSWDRC "chkarchpasswdrc"


#define PLUGIN_NAME N_("Check attachment password Plug-in")
#define PLUGIN_DESC N_("Check password of your mail attachment(*.zip), when you send mail.")

#define GET_RC_BOOLEAN(keyarg) g_key_file_get_boolean(g_opt.rcfile, CHKARCHPASSWD, keyarg, NULL)

static SylPluginInfo info = {
    N_(PLUGIN_NAME),
    "0.6.0",
    "HAYASHI Kentaro",
    N_(PLUGIN_DESC)
};


#if 0
static void exec_chkarchpasswd_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
#endif
static void exec_chkarchpasswd_menu_cb(void);
static void exec_chkarchpasswd_onoff_cb(void);
static void compose_created_cb(GObject *obj, gpointer compose);
static void compose_destroy_cb(GObject *obj, gpointer compose);
static gboolean compose_send_cb(GObject *obj, gpointer compose,
                                gint compose_mode, gint send_mode,
                                const gchar *msg_file, GSList *to_list);
static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey);
static GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey);

void check_attachement_cb(GObject *obj, gpointer data);

void my_rmdir(gchar *path);
void my_rmdir_list(gchar *path);
static gint extract_attachment(AttachInfo *ainfo, gchar *dest, gchar *passwd);

#if defined(G_OS_WIN32)
typedef int WINAPI (*WINAPI_SEVENZIP)(const HWND _hwnd, LPCSTR _szCmdLine, LPSTR _szOutput, const DWORD _dwSize);
typedef BOOL WINAPI (*WINAPI_SEVENZIPSETUNICODEMODE)(BOOL _bUnicode);

static HANDLE g_hdll = NULL;
static WINAPI_SEVENZIP hZip = NULL;
static WINAPI_SEVENZIPSETUNICODEMODE hUniZip = NULL;
#endif

static gboolean g_enable = FALSE;

static GtkWidget *g_plugin_on = NULL;
static GtkWidget *g_plugin_off = NULL;
static GtkWidget *g_onoff_switch = NULL;
static GtkTooltips *g_tooltip = NULL;

struct _ChkArchPasswdOption {
  /* General section */

  /* full path to ghostbiffrc*/
  gchar *rcpath;
  /* rcfile */
  GKeyFile *rcfile;

  /* startup check in general */
  GtkWidget *chk_startup;
  /* check twice when you send email without password. */
  GtkWidget *chk_twice;
  /* check encrypted attachment password */
  GtkWidget *chk_passwd;

  gboolean flg_startup;
  gboolean flg_twice;
  gboolean flg_passwd;

  GtkWidget *aq_dic_entry;
  GtkWidget *aq_dic_btn;
  GtkWidget *phont_entry;
  GtkWidget *phont_btn;
  GtkWidget *phont_cmb;
  GtkWidget *new_subject_btn;
  GtkWidget *new_content_btn;
  GtkWidget *show_subject_btn;
  GtkWidget *show_content_btn;
  GtkWidget *input_entry;

  /* extracted files. */
  GList *rmlist;
#ifdef DEBUG
  /* debug inbox folder */
  GtkWidget *dfolder_entry;
#endif
};

typedef struct _ChkArchPasswdOption ChkArchPasswdOption;

static ChkArchPasswdOption g_opt;

static gchar* g_copyright = N_("Chkarchpasswd is distributed under GPL license.\n"
"\n"
"Copyright (C) 2011 HAYASHI Kentaro <kenhys@gmail.com>"
"\n"
"chkarchpasswd contains following resource as statusbar icon.\n"
"\n\n"
"Silk icon set 1.3: Copyright (C) Mark James\n"
"Licensed under a Creative Commons Attribution 2.5 License.\n"
"http://www.famfamfam.com/lab/icons/silk/\n"
                               "\n"
                               );

void plugin_load(void)
{
  GtkWidget *mainwin;
  GtkWidget *statusbar;
  GtkWidget *plugin_box;
  GdkPixbuf* on_pixbuf;
  GdkPixbuf* off_pixbuf;
  gchar *rcpath;
  
  debug_print("[PLUGIN] initializing chkarchpasswd plug-in\n");

#if defined(G_OS_WIN32)
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
#endif

  syl_init_gettext("chkarchpasswd", "lib/locale");

  debug_print(gettext(PLUGIN_NAME));
  debug_print(dgettext("chkarchpasswd", PLUGIN_DESC));

  info.name = g_strdup(_(PLUGIN_NAME));
  info.description = g_strdup(_(PLUGIN_DESC));
  
  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("Chkarchpasswd Settings [chkarchpasswd]"), exec_chkarchpasswd_menu_cb, NULL);

  syl_plugin_signal_connect("compose-created", G_CALLBACK(compose_created_cb), NULL);

  syl_plugin_signal_connect("compose-destroy", G_CALLBACK(compose_destroy_cb), NULL);

  syl_plugin_signal_connect("compose-send", G_CALLBACK(compose_send_cb), NULL);


  mainwin = syl_plugin_main_window_get();
  statusbar = syl_plugin_main_window_get_statusbar();
  plugin_box = gtk_hbox_new(FALSE, 0);

  on_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)key_add);
  g_plugin_on=gtk_image_new_from_pixbuf(on_pixbuf);
  /*g_plugin_on = gtk_label_new(_("AF ON"));*/
    
  off_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)key_delete);
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

  gtk_widget_hide(g_plugin_on);
  gtk_widget_show(g_plugin_off);
  gtk_tooltips_set_tip
    (g_tooltip, g_onoff_switch,
     _("Chkpasswd is disabled."),
     NULL);

  g_opt.flg_startup = FALSE;
  g_opt.flg_twice = TRUE;
  g_opt.flg_passwd = FALSE;
    
  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, CHKARCHPASSWDRC, NULL);
  g_opt.rcfile = g_key_file_new();
  g_opt.rcpath = g_strdup(rcpath);
  if (g_key_file_load_from_file(g_opt.rcfile, rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
    g_opt.flg_startup=GET_RC_BOOLEAN("startup");

    if (g_opt.flg_startup != FALSE){
      g_enable=TRUE;
      gtk_widget_hide(g_plugin_off);
      gtk_widget_show(g_plugin_on);
      gtk_tooltips_set_tip(g_tooltip, g_onoff_switch,
                           _("Chkarchpasswd is enabled. Click the icon to disable plugin."),
                           NULL);

#if defined(G_OS_WIN32)
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
#endif
    }
        
    g_opt.flg_twice= GET_RC_BOOLEAN( "twice");

    g_opt.flg_passwd=GET_RC_BOOLEAN( "passwd");
      
    g_free(rcpath);
  }

  debug_print("[PLUGIN] chkarchpasswdrc startup:%d\n", g_opt.flg_startup);
  debug_print("[PLUGIN] chkarchpasswdrc twice:%d\n", g_opt.flg_twice);
  debug_print("[PLUGIN] chkarchpasswdrc passwd:%d\n", g_opt.flg_passwd);
  debug_print("[PLUGIN] chkarchpasswd_tool plug-in loading done.\n");

}

void plugin_unload(void)
{
  gchar *path;

  debug_print("chkarchpasswd_tool plug-in unloaded.\n");
#if defined(G_OS_WIN32)
  if (g_hdll!=NULL){
      FreeLibrary(g_hdll);
  }
#endif
  /* NOTE: in older GTK version,
     you cant use GIO, so remove tempolary file listing by myself. */
  path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
                            PLUGIN_DIR,G_DIR_SEPARATOR_S, CHKARCHPASSWD, NULL);
  my_rmdir(path);
}

void my_rmdir(gchar *path)
{
  guint n = 0;

  g_opt.rmlist = NULL;
  my_rmdir_list(path);
  for (n = 0;n< g_list_length(g_opt.rmlist); n++){
    path = (gchar*)g_list_nth_data(g_opt.rmlist, n);
    debug_print("list[%d]:%s\n", n, path);
    if (g_file_test(path, G_FILE_TEST_IS_DIR)!=FALSE){
      g_rmdir(path);
    } else {
      g_remove(path);
    }
  }
  g_rmdir(path);
}

void my_rmdir_list(gchar *dpath)
{
  gchar *fpath;
  GDir *g_dir = g_dir_open(dpath, 0, NULL);
  gchar *path = NULL;
  while ((path = (gchar*)g_dir_read_name(g_dir))!=NULL){
    debug_print("[PLUGIN] path %s\n", path);
    fpath = g_strconcat(dpath, G_DIR_SEPARATOR_S, path, NULL);
    if (g_file_test(fpath, G_FILE_TEST_IS_DIR)!=FALSE){
#if DEBUG
      debug_print("[PLUGIN] remove dir %s\n", fpath);
#endif
      my_rmdir_list(fpath);
      g_opt.rmlist = g_list_append(g_opt.rmlist, fpath);
    }else {
#if DEBUG
      debug_print("[PLUGIN] remove %s\n", fpath);
#endif
      g_opt.rmlist = g_list_append(g_opt.rmlist, fpath);
    }
  }
  g_dir_close(g_dir);
}


SylPluginInfo *plugin_info(void)
{
  return &info;
}

gint plugin_interface_version(void)
{
#if RELEASE_3_1
    /* emulate sylpheed 3.1.0 not svn HEAD */
    return 0x0107;
#else
    /* sylpheed 3.2.0 or later. */
    return 0x0108;
    /*return SYL_PLUGIN_INTERFACE_VERSION;*/
#endif
}


static void prefs_ok_cb(GtkWidget *widget, gpointer data)
{
  gsize sz;
  gchar *buf;

  g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

  g_opt.flg_startup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.chk_startup));
  g_key_file_set_boolean (g_opt.rcfile, CHKARCHPASSWD, "startup", g_opt.flg_startup);
  debug_print("startup:%s\n", g_opt.flg_startup ? "TRUE" : "FALSE");

  g_opt.flg_twice = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.chk_twice));
  g_key_file_set_boolean (g_opt.rcfile, CHKARCHPASSWD, "twice", g_opt.flg_twice);
  debug_print("check twice:%s\n", g_opt.flg_twice ? "TRUE" : "FALSE");

  g_opt.flg_passwd = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.chk_passwd));
  g_key_file_set_boolean (g_opt.rcfile, CHKARCHPASSWD, "passwd", g_opt.flg_passwd);
  debug_print("check passwd:%s\n", g_opt.flg_passwd ? "TRUE" : "FALSE");

  /**/
  buf = g_key_file_to_data(g_opt.rcfile, &sz, NULL);
  g_file_set_contents(g_opt.rcpath, buf, sz, NULL);

  gtk_widget_destroy(GTK_WIDGET(data));
}

static void prefs_cancel_cb(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}


static void exec_chkarchpasswd_menu_cb(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *confirm_area;
  GtkWidget *ok_btn;
  GtkWidget *cancel_btn;
  GtkWidget *notebook;

  debug_print("[PLUGIN] exec_chkarchpasswd_menu_cb is called.\n");

  /* show modal dialog */

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(window), 8);
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 300);
  gtk_widget_realize(window);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* notebook */ 
  notebook = gtk_notebook_new();
  /* main tab */
  create_config_main_page(notebook, g_opt.rcfile);
  /* about, copyright tab */
  create_config_about_page(notebook, g_opt.rcfile);

  gtk_widget_show(notebook);
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

  confirm_area = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(confirm_area), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(confirm_area), 6);


  ok_btn = gtk_button_new_from_stock(GTK_STOCK_OK);
  GTK_WIDGET_SET_FLAGS(ok_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), ok_btn, FALSE, FALSE, 0);
  gtk_widget_show(ok_btn);

  cancel_btn = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancel_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), cancel_btn, FALSE, FALSE, 0);
  gtk_widget_show(cancel_btn);

  gtk_widget_show(confirm_area);
	
  gtk_box_pack_end(GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
  gtk_widget_grab_default(ok_btn);

  gtk_window_set_title(GTK_WINDOW(window), _("Chkarchpasswd Settings"));

  g_signal_connect(G_OBJECT(ok_btn), "clicked",
                   G_CALLBACK(prefs_ok_cb), window);
  g_signal_connect(G_OBJECT(cancel_btn), "clicked",
                   G_CALLBACK(prefs_cancel_cb), window);
  gtk_widget_show(window);

}

static void exec_chkarchpasswd_onoff_cb(void)
{

  if (g_enable != TRUE){
#if defined(G_OS_WIN32)
    if (g_hdll!=NULL){
      syl_plugin_alertpanel_message(_("Chkarchpasswd"), _("chkarchpasswd plugin is enabled."), ALERT_NOTICE);
      g_enable=TRUE;
      gtk_widget_hide(g_plugin_off);
      gtk_widget_show(g_plugin_on);
      gtk_tooltips_set_tip
        (g_tooltip, g_onoff_switch,
         _("Chkarchpasswd is enabled. Click the icon to disable plugin."),
         NULL);
    }
#endif
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

void compose_created_cb(GObject *obj, gpointer data)
{
  Compose *compose = (Compose*)data;


  /* add check archive button for testing. */
  /* GtkWidget *icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_LARGE_TOOLBAR);*/
  GtkWidget *toolbar = compose->toolbar;
  GtkWidget *icon = gtk_image_new_from_stock(GTK_STOCK_CDROM, GTK_ICON_SIZE_LARGE_TOOLBAR);  
  GtkToolItem *toolitem = gtk_tool_button_new(icon, "check attachement password");
  GtkTooltips *tooltips = gtk_tooltips_new();
  gtk_tool_item_set_tooltip(toolitem, tooltips, "check your mail attachement password", "");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
  
  g_signal_connect(G_OBJECT(toolitem), "clicked",
                   G_CALLBACK(check_attachement_cb), compose);
  g_signal_connect(G_OBJECT(GTK_BIN(toolitem)->child),
                   "button_press_event",
                   G_CALLBACK(check_attachement_cb), compose);
  gtk_widget_show_all(toolbar);
  
}

void compose_destroy_cb(GObject *obj, gpointer compose)
{
  debug_print("[PLUGIN] compose_destroy_cb is called.\n");

  /**/
}

void check_attachement_cb(GObject *obj, gpointer data)
{
}

#define MIME_ZIP "application/zip"
#define MIME_7Z "application/octet-stream"

static gboolean compose_send_cb(GObject *obj, gpointer data,
                                gint compose_mode, gint send_mode,
                                const gchar *msg_file, GSList *to_list)
{

  Compose *compose = (Compose*)data;
  GtkTreeModel *model = GTK_TREE_MODEL(compose->attach_store);
  GtkTreeIter iter;
  AttachInfo *ainfo;
  gboolean valid;
  gint nblank;
  gint npasswd;
  gchar* path;
  gint npasstotal = 0;
  gint npassok = 0;
  gboolean bpasswd = FALSE;
  GtkTextView *text;
  GtkTextBuffer *buffer;
  GtkTextIter tsiter, teiter;
  gchar *pwtext;
  GScanner *gscan;
  GTokenValue gvalue;
  GList *pwlist = NULL;
  int index=0;
  GTokenType gtoken;
  gchar *msg=NULL;

  gchar *passwd=NULL;
  guint pwidx = 0;
  gboolean bmatch = FALSE;
  gboolean bcancel = TRUE;
  gint val;
  
  debug_print("[PLUGIN] compose_send_cb is called.\n");

  debug_print("Compose* compose:%p\n", compose);
  debug_print("gpointer compose:%p\n", data);


  debug_print("model:%p\n", model);
  
  /* create working directory for unzip */
  path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
                            PLUGIN_DIR,G_DIR_SEPARATOR_S, "chkarchpasswd", NULL);
  g_mkdir_with_parents(path, 0);
  

  debug_print("text:%p\n", compose->text);
  text = GTK_TEXT_VIEW(compose->text);
  if (text==NULL){
    debug_print("text is NULL\n");
    return TRUE;
  }else{
    debug_print("text:%p\n", text);
  }
  buffer = gtk_text_view_get_buffer(text);
  if (buffer == NULL){
    debug_print("buffer is NULL\n");
    return TRUE;
  }else{
    debug_print("buffer:%p\n", buffer);
  }
  gtk_text_buffer_get_bounds(buffer, &tsiter, &teiter);
  pwtext  = gtk_text_buffer_get_text(buffer, &tsiter, &teiter, TRUE);
  gscan  = g_scanner_new(NULL);
  gscan->config->scan_identifier_1char=TRUE;

  if (pwtext==NULL){
    debug_print("pwtext is NULL\n");
  }else{
    g_scanner_input_text(gscan, pwtext, strlen(pwtext));
    debug_print("pwtext:%p\n", pwtext);
  }

  debug_print("scan loop\n");
  while( g_scanner_eof(gscan) != TRUE){
    gtoken = g_scanner_get_next_token (gscan);
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

#if 0
  GtkTreeView *treeview = GTK_TREE_VIEW(compose->attach_treeview);
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
      if (memcmp(MIME_ZIP, ainfo->content_type, sizeof(MIME_ZIP)) == 0 ||
          (memcmp(MIME_7Z, ainfo->content_type, sizeof(MIME_7Z)) == 0 &&
           g_strrstr(ainfo->file, ".7z")!=NULL)) {
        /* check zip and 7z archive. */
        npasstotal += 1;

        debug_print("file:%s\n", ainfo->file);
        debug_print("content_type:%s\n", ainfo->content_type);
        debug_print("name:%s\n", ainfo->name);
        debug_print("size:%ld\n", ainfo->size);

        bpasswd=FALSE;
        /* input password for archive */
        nblank = extract_attachment(ainfo, path, NULL);
        g_print("%s blank password result:%08x\n", ainfo->name,nblank);

        npasswd = extract_attachment(ainfo, path, "test");
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
          if (g_opt.flg_passwd != FALSE){
            msg = g_strdup_printf(_("enter password for attachment(%s)."), ainfo->name);
            passwd = syl_plugin_input_dialog("password", msg, "input password here");
            if (passwd==NULL){
              debug_print("[PLUGIN] skip password check for attachement(%s)\n", ainfo->name);
              return TRUE;
            }else {
              npasswd = extract_attachment(ainfo, path, passwd);
              if (npasswd == 0x00000000){
              } else if (npasswd == 0x0000800a){
                /* bad password */
                msg=g_strdup_printf(_("password (%s) for attachment(%s) is not valid."),
                                    passwd, ainfo->name);
                syl_plugin_alertpanel("", msg, GTK_STOCK_OK,NULL, NULL);
                return TRUE;
              }
            }
          }
          /* check pwlist */
          for (pwidx = 0; pwidx < g_list_length(pwlist); pwidx++){
            passwd = g_list_nth_data(pwlist, pwidx);
              g_print("check password %s for %s\n",passwd, ainfo->name);
              nblank = extract_attachment(ainfo, path, passwd);
              if (nblank == 0x00000000){
                g_print("%s blank password result:%08x\n", ainfo->name,nblank);
                bmatch = TRUE;
                msg=g_strdup_printf(_("attachment(%s) password (%s) is described in mail body."),
                                    ainfo->name, passwd);
                syl_plugin_alertpanel("", msg, GTK_STOCK_OK,NULL, NULL);
                return TRUE;
              }
            }
            npassok += 1;
          }
      }else{
      }
  }
  if ( npasstotal > 0 && npassok == npasstotal){
    debug_print("[PLUGIN] password is not empty. sending mail...");
    bcancel = FALSE;
  }else if (npasstotal > 0 && npassok < npasstotal){
    val = syl_plugin_alertpanel("", _("attachment password is empty. Do you want to send?"),
                                     GTK_STOCK_YES, GTK_STOCK_NO, NULL);
    if (val != 0){
      /* no is selected. */
      return TRUE;
    }
    if ( g_opt.flg_twice != FALSE){
      val = syl_plugin_alertpanel("", _("attachment password is empty. Do you really want to send?"),
                                  GTK_STOCK_NO, GTK_STOCK_YES, NULL);
      if (val == 0){
        /* no is selected. */
        return TRUE;
      }
    }
    bcancel = FALSE;
  }else if (npasstotal == 0){
      bcancel = FALSE;
  }else{
      syl_plugin_alertpanel("", "abort to sending email.",
                            GTK_STOCK_OK, NULL, NULL);
  }
  g_print("compose_send_cb:%s\n", bcancel ? "TRUE" : "FALSE");
  return bcancel;
}

static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey)
{
  gboolean status;
  GtkWidget *vbox;
  GtkWidget *label;

  debug_print("create_config_main_page\n");
  if (notebook == NULL){
    return NULL;
  }
  /* startup */
  if (pkey!=NULL){
  }
  vbox = gtk_vbox_new(FALSE, 6);

  g_opt.chk_startup = gtk_check_button_new_with_label(_("Enable plugin on startup."));
  gtk_widget_show(g_opt.chk_startup);
  gtk_box_pack_start(GTK_BOX(vbox), g_opt.chk_startup, FALSE, FALSE, 0);

  g_opt.chk_twice = gtk_check_button_new_with_label(_("Enable confirmation twice before sending mail without password for attachement."));
  gtk_box_pack_start(GTK_BOX(vbox), g_opt.chk_twice, FALSE, FALSE, 0);

  g_opt.chk_passwd = gtk_check_button_new_with_label(_("Enable password validation for attachment."));
  gtk_box_pack_start(GTK_BOX(vbox), g_opt.chk_passwd, FALSE, FALSE, 0);

  label = gtk_label_new(_("General"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
  gtk_widget_show_all(notebook);

  status=g_key_file_get_boolean(pkey, CHKARCHPASSWD, "startup", NULL);
  debug_print("startup:%s\n", status ? "TRUE" : "FALSE");
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.chk_startup), TRUE);
  }

  status=g_key_file_get_boolean(pkey, CHKARCHPASSWD, "twice", NULL);
  debug_print("startup:%s\n", status ? "TRUE" : "FALSE");
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.chk_twice), TRUE);
  }

  status=g_key_file_get_boolean(pkey, CHKARCHPASSWD, "passwd", NULL);
  debug_print("startup:%s\n", status ? "TRUE" : "FALSE");
  if (status!=FALSE){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.chk_passwd), TRUE);
  }

  return NULL;
}

/* about, copyright tab */
static GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey)
{
  GtkWidget *hbox, *vbox;
  GtkTextBuffer *tbuffer;
  GtkWidget *tview;
  GtkWidget *misc;
  GtkWidget *scrolled;
  GtkWidget *label;

  debug_print("create_config_about_page\n");
  if (notebook == NULL){
    return NULL;
  }
  hbox = gtk_hbox_new(TRUE, 6);
  vbox = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 6);

  misc = gtk_label_new("Chkarchpasswd");
  gtk_box_pack_start(GTK_BOX(vbox), misc, FALSE, TRUE, 6);

  misc = gtk_label_new(PLUGIN_DESC);
  gtk_box_pack_start(GTK_BOX(vbox), misc, FALSE, TRUE, 6);

  /* copyright */
  scrolled = gtk_scrolled_window_new(NULL, NULL);

  tbuffer = gtk_text_buffer_new(NULL);
  gtk_text_buffer_set_text(tbuffer, _(g_copyright), strlen(g_copyright));
  tview = gtk_text_view_new_with_buffer(tbuffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(tview), FALSE);
  gtk_container_add(GTK_CONTAINER(scrolled), tview);
    
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 6);
    
  /**/
  label = gtk_label_new(_("About"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, label);
  gtk_widget_show_all(notebook);
  return NULL;
}

static gint extract_attachment(AttachInfo *ainfo, gchar *dest, gchar *passwd)
{
  gint nresult = 0;
#if defined(G_OS_WIN32)
  gchar buf[1024];
  DWORD dwSize = 0;
#endif
  gchar *com = NULL;
  if (passwd!=NULL){
      com = g_strdup_printf("x \"%s\" -aoa -p\"%s\" -hide -o\"%s\" -r",
                            ainfo->file, passwd, dest);
  } else {
      com = g_strdup_printf("x \"%s\" -aoa -p\"\" -hide -o\"%s\" -r",
                               ainfo->file, dest);
  }
#if defined(G_OS_WIN32)
  nresult = hZip(NULL, com, buf, dwSize);
#endif
  g_free(com);
  return nresult;
}
