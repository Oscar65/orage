/* xfcalendar
 *
 * Copyright (C) 2003 Mickael Graf (korbinus@linux.se)
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.  You
 * should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/i18n.h>
#include <libxfce4util/util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfce4mcs/mcs-client.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "calendar-icon.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "xfce_trayicon.h"

#define CHANNEL  "xfcalendar"

/* session client handler */
static SessionClient	*session_client = NULL;

/* main window */
static GtkWidget	*mainWindow = NULL;

/* MCS client */
extern McsClient        *client;

/* tray icon */
XfceTrayIcon 		*trayIcon = NULL;

extern settings calsets;
extern gboolean normalmode;

void
createRCDir(void)
{
  gchar *calpath;

  calpath = xfce_get_userfile("xfcalendar", NULL);

  if (!g_file_test(calpath, G_FILE_TEST_IS_DIR)) {
    if (mkdir(calpath, 0755) < 0) {
      g_error("Unable to create directory %s: %s",
	      calpath, g_strerror(errno));
    }
  }

  g_free(calpath);
}

/*
 * SaveYourself callback
 *
 * This is called when the session manager requests the client to save its
 * state.
 */
/* ARGUSED */
static void
save_yourself_cb(gpointer data, int save_style, gboolean shutdown,
                 int interact_style, gboolean fast)
{
  settings_set_showCal(mainWindow);
  apply_settings();
}

/*
 * Die callback
 *
 * This is called when the session manager requests the client to go down.
 */
static void
die_cb(gpointer data)
{
  gtk_main_quit();
}

/*
 */
static GdkFilterReturn
selection_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  XClientMessageEvent *xev;

  xev = (XClientMessageEvent *)xevent;

  switch (xev->type) {
  case ClientMessage:
    if (xev->message_type == XInternAtom(GDK_DISPLAY(),
					 "_XFCE_CALENDAR_RAISE", False)) {
      g_print("RAISING...\n");
      gtk_widget_show(mainWindow);
      gtk_window_stick(GTK_WINDOW(mainWindow));
      gdk_window_raise(mainWindow->window);
      gdk_window_focus(mainWindow->window, GDK_CURRENT_TIME);
      return(GDK_FILTER_REMOVE);
    }
    break;
  }

  return(GDK_FILTER_CONTINUE);
}

int
main(int argc, char *argv[])
{
  GtkWidget *menuItem;
  GtkWidget *hidden;
  GtkWidget *trayMenu;
  GdkPixbuf *pixbuf;
  Window xwindow;
  GdkAtom atom;
  Display *dpy;
  int scr;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  gtk_init(&argc, &argv);

  dpy = GDK_DISPLAY();
  scr = DefaultScreen(dpy);

  atom = gdk_atom_intern("_XFCE_CALENDAR_RUNNING", FALSE);

  /*
   * Check if xfcalendar is already running on the display
   */
  if ((xwindow = XGetSelectionOwner(GDK_DISPLAY(),
				    gdk_x11_atom_to_xatom(atom))) != None) {
    XClientMessageEvent xev;

    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = xwindow;
    xev.message_type = XInternAtom(GDK_DISPLAY(), "_XFCE_CALENDAR_RAISE", FALSE);
    xev.format = 32;

    XSendEvent(GDK_DISPLAY(), xwindow, False, NoEventMask,
	       (XEvent *)&xev);
    XSync(GDK_DISPLAY(), False);

    return(EXIT_SUCCESS);
  }

  /* 
   * try to connect to the session manager
   */
  session_client = client_session_new(argc, argv, NULL,
				      SESSION_RESTART_IF_RUNNING, 50);
  session_client->save_yourself = save_yourself_cb;
  session_client->die = die_cb;
  (void)session_init(session_client);

  add_pixmap_directory(PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE
		       G_DIR_SEPARATOR_S "pixmaps");

  /*
   * Create the XFCalendar.
   */
  mainWindow = create_XFCalendar();
  set_mainWin(mainWindow);
  set_cal(mainWindow);
  init_settings(mainWindow);
  mark_appointments(mainWindow);
  setup_signals(mainWindow);

  /*
   */
  hidden = gtk_invisible_new();
  gtk_widget_show(hidden);
  gdk_window_add_filter(hidden->window, (GdkFilterFunc)selection_filter,
			NULL);
  if (!gdk_selection_owner_set(hidden->window, atom,
			       gdk_x11_get_server_time(hidden->window),
			       FALSE)) {
    g_warning("Unable acquire ownership of selection");
  }

  /*
   * Create the tray icon and its popup menu
   */
  trayIcon = create_TrayIcon(mainWindow);
  xfce_tray_icon_connect(trayIcon);
	
  /*
   * Now it's serious, the application is running, so we create the RC
   * directory
   */
  createRCDir();

  client = mcs_client_new(dpy, scr, notify_cb, watch_cb, mainWindow);
  if(client)
    {
      mcs_client_add_channel(client, CHANNEL);
    }
  else
    {
      g_warning(_("Cannot create MCS client channel"));
    }
	
  gtk_main();
  keep_tidy();

  return(EXIT_SUCCESS);
}

