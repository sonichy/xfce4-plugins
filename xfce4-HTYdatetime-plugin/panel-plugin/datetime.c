/*  $Id$
 *
 *  Copyright (C) 2019 John Doo <john@foo.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include "datetime.h"
#include "sample-dialogs.h"

/* default settings */
#define DEFAULT_SETTING1 NULL
#define DEFAULT_SETTING2 1
#define DEFAULT_SETTING3 FALSE



/* prototypes */
static void
sample_construct (XfcePanelPlugin *plugin);


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (sample_construct);



void sample_save (XfcePanelPlugin *plugin, SamplePlugin *sample)
{
  XfceRc *rc;
  gchar  *file;

  /* get the config file location */
  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (file == NULL)) {
       DBG ("Failed to open config file");
       return;
    }

  /* open the config file, read/write */
  rc = xfce_rc_simple_open (file, FALSE);
  g_free (file);

  if (G_LIKELY (rc != NULL)) {
      /* save the settings */
      DBG(".");
      if (sample->setting1)
        xfce_rc_write_entry    (rc, "tooltip", sample->setting1);

      xfce_rc_write_int_entry  (rc, "setting2", sample->setting2);
      xfce_rc_write_bool_entry (rc, "setting3", sample->setting3);

      /* close the rc file */
      xfce_rc_close (rc);
    }
}


static void sample_read (SamplePlugin *sample)
{
  XfceRc      *rc;
  gchar       *file;
  const gchar *value;

  /* get the plugin config file location */
  file = xfce_panel_plugin_save_location (sample->plugin, TRUE);

  if (G_LIKELY (file != NULL)) {
      /* open the config file, readonly */
      rc = xfce_rc_simple_open (file, TRUE);

      /* cleanup */
      g_free (file);

      if (G_LIKELY (rc != NULL)) {
          /* read the settings */
          value = xfce_rc_read_entry (rc, "tooltip", "");
          sample->setting1 = g_strdup (value);
		   
          sample->setting2 = xfce_rc_read_int_entry (rc, "setting2", DEFAULT_SETTING2);
          sample->setting3 = xfce_rc_read_bool_entry (rc, "setting3", DEFAULT_SETTING3);

          /* cleanup */
          xfce_rc_close (rc);

          /* leave the function, everything went well */
          return;
        }
    }

  /* something went wrong, apply default values */
  DBG ("Applying default settings");

  sample->setting1 = g_strdup (DEFAULT_SETTING1);
  sample->setting2 = DEFAULT_SETTING2;
  sample->setting3 = DEFAULT_SETTING3;
}


static void on_calendar_realized(GtkWidget *widget, SamplePlugin *sample)
{
  gint x, y;
  GtkWidget *parent;

  parent = g_object_get_data(G_OBJECT(widget), "calendar-parent");
  xfce_panel_plugin_position_widget(sample->plugin, widget, parent, &x, &y);
  gtk_window_move(GTK_WINDOW(widget), x, y);
}


static gboolean close_calendar_window(SamplePlugin *sample)
{
  gtk_widget_destroy(sample->calendar);
  sample->calendar = NULL;

  //xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (datetime->plugin), FALSE);
  //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sample->label), FALSE);

  return TRUE;
}


static GtkWidget * pop_calendar_window(SamplePlugin *sample)
{
  GtkWidget  *window;
  GtkWidget  *calendar;
  GtkWidget  *parent = sample->label;
  GdkScreen  *screen;
  GtkCalendarDisplayOptions display_options;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
  gtk_window_stick(GTK_WINDOW(window));
  g_object_set_data(G_OBJECT(window), "calendar-parent", parent);

  /* set screen number */
  screen = gtk_widget_get_screen(parent);
  gtk_window_set_screen(GTK_WINDOW(window), screen);

  calendar = gtk_calendar_new();
  display_options = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_WEEK_NUMBERS | GTK_CALENDAR_SHOW_DAY_NAMES;
  gtk_calendar_set_display_options(GTK_CALENDAR (calendar), display_options);
  gtk_container_add (GTK_CONTAINER(window), calendar);

  g_signal_connect_after(G_OBJECT(window), "realize", G_CALLBACK(on_calendar_realized), sample);
  //g_signal_connect_swapped(G_OBJECT(window), "delete-event", G_CALLBACK(close_calendar_window), sample);
  g_signal_connect_swapped(G_OBJECT(window), "focus-out-event", G_CALLBACK(close_calendar_window), sample);
  gtk_widget_show_all(window);

  //xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (datetime->plugin), TRUE);
  //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sample->label), TRUE);

  return window;
}


static gboolean datetime_clicked(GtkWidget *widget, GdkEventButton *event, SamplePlugin *sample)
{
  if (event->button != 1 || event->state & GDK_CONTROL_MASK)
    return FALSE;

  if (sample == NULL)
    return FALSE;

  if (sample->calendar != NULL)  {
    close_calendar_window(sample);
  }  else  {
    sample->calendar = pop_calendar_window(sample);
  }
  return TRUE;
}


static gboolean update_cb (gpointer data)
{
  SamplePlugin *sample = data;
  update (sample);
  return TRUE;
}


void update (SamplePlugin *sample)
{
  gint64 timeval_ms;
  time_t timeval_s;
  struct tm *t;
  char text[100], text1[100], weekday[7][4] = {"日", "一", "二", "三", "四", "五", "六"}; // char weekday[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  timeval_ms = g_get_real_time() / 1000;
  timeval_s = timeval_ms / 1000;
  t = localtime(&timeval_s);
  //https://coder55.com/article/158257  
  sprintf(text, "%d/%d %s\n%02d:%02d", 1+t->tm_mon, t->tm_mday, weekday[t->tm_wday], t->tm_hour, t->tm_min);
  sprintf(text1, "%d年%d月%d日 %s %02d:%02d:%02d\n%s", 1900+t->tm_year, 1+t->tm_mon, t->tm_mday, weekday[t->tm_wday], t->tm_hour, t->tm_min, t->tm_sec, sample->setting1);
  //g_message("%s", text1);
  //gtk_label_set_text (GTK_LABEL(sample->label), text);
  gtk_button_set_label (GTK_BUTTON(sample->label), text);
  gtk_widget_set_tooltip_text(sample->label, text1);
}

static SamplePlugin *sample_new (XfcePanelPlugin *plugin)
{
  SamplePlugin   *sample;
  GtkOrientation  orientation;
  
  /* allocate memory for the plugin structure */
  sample = g_slice_new0 (SamplePlugin);

  /* pointer to plugin */
  sample->plugin = plugin;

  /* read the user settings */
  sample_read (sample);

  /* get the current orientation */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* create some panel widgets */
  sample->ebox = gtk_event_box_new ();
  gtk_widget_show (sample->ebox);

  sample->hvbox = gtk_box_new (orientation, 2);
  gtk_widget_show (sample->hvbox);
  gtk_container_add (GTK_CONTAINER (sample->ebox), sample->hvbox);

  /* some sample widgets */
  //sample->label = gtk_label_new ("8/7 一\n00:00");
  sample->label = gtk_button_new_with_label ("1/1 一\n00:00");
  g_signal_connect(sample->label, "button-press-event", G_CALLBACK(datetime_clicked), sample);
  gtk_widget_show (sample->label);
  gtk_box_pack_start (GTK_BOX (sample->hvbox), sample->label, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text(sample->label, "2023年1月1日 星期一 00:00:00");

  sample->timeout_id = g_timeout_add (1000, update_cb, sample);

  return sample;
}



static void
sample_free (XfcePanelPlugin *plugin,
             SamplePlugin    *sample)
{
  GtkWidget *dialog;

  /* check if the dialog is still open. if so, destroy it */
  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  /* destroy the panel widgets */
  gtk_widget_destroy (sample->hvbox);

  /* cleanup the settings */
  if (G_LIKELY (sample->setting1 != NULL))
    g_free (sample->setting1);

  /* free the plugin structure */
  g_slice_free (SamplePlugin, sample);
}



static void
sample_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            SamplePlugin    *sample)
{
  /* change the orientation of the box */
  gtk_orientable_set_orientation(GTK_ORIENTABLE(sample->hvbox), orientation);
}



static gboolean
sample_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     SamplePlugin    *sample)
{
  GtkOrientation orientation;

  /* get the orientation of the plugin */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* set the widget size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  /* we handled the orientation */
  return TRUE;
}



static void
sample_construct (XfcePanelPlugin *plugin)
{
  SamplePlugin *sample;

  /* setup transation domain */
  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* create the plugin */
  sample = sample_new (plugin);

  /* add the ebox to the panel */
  gtk_container_add (GTK_CONTAINER (plugin), sample->ebox);

  /* show the panel's right-click menu on this ebox */
  xfce_panel_plugin_add_action_widget (plugin, sample->ebox);

  /* connect plugin signals */
  g_signal_connect (G_OBJECT (plugin), "free-data",
                    G_CALLBACK (sample_free), sample);

  g_signal_connect (G_OBJECT (plugin), "save",
                    G_CALLBACK (sample_save), sample);

  g_signal_connect (G_OBJECT (plugin), "size-changed",
                    G_CALLBACK (sample_size_changed), sample);

  g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                    G_CALLBACK (sample_orientation_changed), sample);

  /* show the configure menu item and connect signal */
  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin",
                    G_CALLBACK (sample_configure), sample);

  /* show the about menu item and connect signal */
  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about",
                    G_CALLBACK (sample_about), NULL);
}


