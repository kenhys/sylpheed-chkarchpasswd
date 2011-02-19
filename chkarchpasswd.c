/*
 * Auto mail forward Plug-in -- forward functionality plug-in for Sylpheed
 * Copyright (C) 2011 HAYASHI Kentaro <kenhys@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

void plugin_load(void)
{
  debug_print("[PLUGIN] initializing chkarchpasswd plug-in\n");

  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("Toggle chkarchpasswd"), exec_chkarchpasswd_menu_cb, NULL);

#if 0
  g_signal_connect(syl_app_get(), "add-msg", G_CALLBACK(exec_chkarchpasswd_cb), NULL);
#endif
  
  syl_plugin_signal_connect("compose-created", G_CALLBACK(compose_created_cb), NULL);

  syl_plugin_signal_connect("compose-destroy", G_CALLBACK(compose_destroy_cb), NULL);

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

static gboolean send_button_press(GtkWidget	*widget,
                                  GdkEventButton	*event,
                                  gpointer	 data);

static gulong g_hook_id = 0;

/* global button-press-event signal id */
static guint g_signal_id = 0;

static GtkToolItem *g_sendbtn = NULL;

/* original button handler id. */
static gulong g_sendbtn_id = 0;

static gboolean
button_press_emission_hook (GSignalInvocationHint	*ihint,
                            guint			n_param_values,
                            const GValue			*param_values,
                            gpointer			data)
{

  GObject *object;
  GdkEventCrossing *event;

  debug_print("[PLUGIN] button_press_emission_hook called.\n");

  debug_print("[PLUGIN] signal id is %d.\n", ihint->signal_id);
  debug_print("[PLUGIN] GQuark is %x.\n", ihint->detail);
  debug_print("[PLUGIN] GSignalFlags %x.\n", ihint->run_type);


  object = g_value_get_object (param_values + 0);
  event  = g_value_get_boxed (param_values + 1);

    
  debug_print("[PLUGIN] button handler id is %ld.\n", g_sendbtn_id);

#if 0
  g_signal_stop_emission(G_OBJECT(g_sendbtn), g_signal_id, 0);

  /*gboolean bconnected = g_signal_handler_is_connected(G_OBJECT(g_sendbtn), g_sendbtn_id);
  debug_print("[PLUGIN] button handler id connected ? %s.\n", bconnected ? "TRUE" : "FALSE");
  */
  /*g_signal_handler_block(G_OBJECT(g_sendbtn), g_sendbtn_id);*/


  guint blocked = 0;
  /*g_signal_handlers_block_matched(G_OBJECT(g_sendbtn),
                                                 G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_UNBLOCKED,
                                                 g_signal_id,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL);*/
#else
  gboolean bconnected = g_signal_handler_is_connected(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_sendbtn_id);
  debug_print("[PLUGIN] button handler id connected ? %s.\n", bconnected ? "TRUE" : "FALSE");
  
  /* g_signal_handler_block(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_sendbtn_id);*/

  guint blocked = g_signal_handlers_block_matched(G_OBJECT(GTK_BIN(g_sendbtn)->child),
                                                 G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_UNBLOCKED,
                                                 g_signal_id,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL);
#endif
  debug_print("[PLUGIN] number of blocked handler is %d.\n", blocked);
  
  /*g_signal_stop_emission(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_signal_id, 0);*/
  g_signal_stop_emission_by_name(G_OBJECT(GTK_BIN(g_sendbtn)), "button_pressed_event");
  g_signal_stop_emission_by_name(G_OBJECT(GTK_BIN(g_sendbtn)), "clicked");

  return TRUE;
}

static void destroy_notify(gpointer data)
{
  debug_print("[PLUGIN] destroy_notify called.\n");
}

void compose_created_cb(GObject *obj, gpointer compose)
{
  Compose* pComp = (Compose*)compose;
  GtkWidget *toolbar = pComp->toolbar;

  /* get global button-press-eventsignal id */
#if 1
  g_signal_id = g_signal_lookup("button-press-event", GTK_TYPE_BUTTON);
  debug_print("[PLUGIN] signal_id of button_press_event:%d\n", g_signal_id);
#else
  g_signal_id = g_signal_lookup("clicked", GTK_TYPE_TOOL_BUTTON);
  debug_print("[PLUGIN] signal_id of clicked:%d\n", g_signal_id);
#endif
  
  if (g_signal_id == 0){
    return;
  }

  g_sendbtn = gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 0);
  if (g_sendbtn != NULL){
    debug_print("[PLUGIN] toolitem:%p\n", g_sendbtn);

#if 0
    /**/
    g_sendbtn_id = g_signal_handler_find(G_OBJECT(g_sendbtn),
                                         G_SIGNAL_MATCH_ID|G_SIGNAL_MATCH_UNBLOCKED,
                                         g_signal_id, 0, NULL, NULL, NULL);
    if (g_sendbtn_id == 0) {
      debug_print("[PLUGIN] can not find toolitem signal handler id.\n");
    } else {
      debug_print("[PLUGIN] toolitem signal handler id is %ld.\n", g_sendbtn_id);

      if (g_hook_id == 0){
        debug_print("[PLUGIN] add emission hook for signal %d.\n", g_signal_id);
        g_hook_id = g_signal_add_emission_hook(g_signal_id, 0, button_press_emission_hook, compose, destroy_notify);
        debug_print("[PLUGIN] emission hook handler %ld.\n", g_hook_id);
      }
      /* block sending mail */
      /*g_signal_handler_block(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_sendbtn_id);*/
    }

#else
    g_sendbtn_id = g_signal_handler_find(G_OBJECT(GTK_BIN(g_sendbtn)->child),
                                         G_SIGNAL_MATCH_ID|G_SIGNAL_MATCH_UNBLOCKED,
                                         g_signal_id, 0, NULL, NULL, NULL);
    if (g_sendbtn_id == 0) {
      debug_print("[PLUGIN] can not find toolitem signal handler id.\n");
    } else {
      debug_print("[PLUGIN] toolitem signal handler id is %ld.\n", g_sendbtn_id);

      if (g_hook_id == 0){
        debug_print("[PLUGIN] add emission hook for signal %d.\n", g_signal_id);
        g_hook_id = g_signal_add_emission_hook(g_signal_id, 0, button_press_emission_hook, compose, destroy_notify);
        debug_print("[PLUGIN] emission hook handler %ld.\n", g_hook_id);
      }
      /* block sending mail */
      gboolean bconnected = g_signal_handler_is_connected(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_sendbtn_id);
      debug_print("[PLUGIN] button handler id connected ? %s.\n", bconnected ? "TRUE" : "FALSE");
  
      /* g_signal_handler_block(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_sendbtn_id);*/

      guint blocked = g_signal_handlers_block_matched(G_OBJECT(GTK_BIN(g_sendbtn)->child),
                                                      G_SIGNAL_MATCH_ID,
                                                      g_signal_id,
                                                      0,
                                                      NULL,
                                                      NULL,
                                                      NULL);
      debug_print("[PLUGIN] number of blocked handler is %d.\n", blocked);

      g_signal_handler_block(G_OBJECT(GTK_BIN(g_sendbtn)->child), g_sendbtn_id);
      
    }
#endif
  }
}

void compose_destroy_cb(GObject *obj, gpointer compose)
{
  debug_print("[PLUGIN] compose_destroy_cb is called.\n");
  debug_print("[PLUGIN] g_signal_id is %d.\n", g_signal_id);
  debug_print("[PLUGIN] g_hook_id is %ld.\n", g_hook_id);

  if (g_signal_id != 0 && g_hook_id != 0){
    g_signal_remove_emission_hook(g_signal_id, g_hook_id);

    g_hook_id = 0;
  }
  
}

gboolean send_button_press(GtkWidget	*widget,
                           GdkEventButton	*event,
                           gpointer	 data)
{
  debug_print("[PLUGIN] send_button_press.\n");

  /* stop furthor event handling. */
  return TRUE;
}
