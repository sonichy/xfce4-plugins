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

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "sample.h"
#include "sample-dialogs.h"

/* the website url */
#define PLUGIN_WEBSITE "https://github.com/sonichy/xfce4-CMUD-plugin/"



static void
sample_configure_response (GtkWidget    *dialog,
                           gint          response,
                           SamplePlugin *sample)
{
  gboolean result;

  if (response == GTK_RESPONSE_HELP)
    {
      /* show help */
      result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

      if (G_UNLIKELY (result == FALSE))
        g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }
  else
    {
      /* remove the dialog data from the plugin */
      g_object_set_data (G_OBJECT (sample->plugin), "dialog", NULL);

      /* unlock the panel menu */
      xfce_panel_plugin_unblock_menu (sample->plugin);

      /* save the plugin */
      sample_save (sample->plugin, sample);

      /* destroy the properties dialog */
      gtk_widget_destroy (dialog);
    }
}


static gboolean
textView_change_cb(GtkWidget *widget, GdkEventFocus *ev, SamplePlugin *sample)
{  
  GtkTextIter start, end;
  GtkTextBuffer *buffer;
  gchar *s;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  s = gtk_text_iter_get_text (&start, &end);
  sample->setting1 = s;
  update(sample);
  return FALSE;
}


void
sample_configure (XfcePanelPlugin *plugin,
                  SamplePlugin    *sample)
{
  GtkWidget *dialog, *hbox, *label, *textView; 
  GtkTextBuffer *buffer;
  gchar *s;

  /* block the plugin menu */
  xfce_panel_plugin_block_menu (plugin);

  /* create the dialog */
  dialog = xfce_titled_dialog_new_with_buttons ("CMUD",
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                "gtk-help", GTK_RESPONSE_HELP,
                                                "gtk-close", GTK_RESPONSE_OK,
                                                NULL);
  s = sample->setting1;
  //vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  label = gtk_label_new(_("Tooltip"));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_margin_top (label, 0); //无效
  textView = gtk_text_view_new ();
  gtk_widget_set_size_request(textView, 400, 200); 
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(textView));
  gtk_text_buffer_set_text(buffer, s, -1);
  gtk_box_pack_start (GTK_BOX(hbox), textView, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT(textView), "focus-out-event", G_CALLBACK (textView_change_cb), sample);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, FALSE, FALSE, 0);

  /* center dialog on the screen */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* set dialog icon */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  /* link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* connect the response signal to the dialog */
  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK(sample_configure_response), sample);

  /* show the entire dialog */
  gtk_widget_show_all (dialog);
}



void
sample_about (XfcePanelPlugin *plugin)
{
  /* about dialog code. you can use the GtkAboutDialog or the XfceAboutInfo widget */
  GdkPixbuf *icon;

  const gchar *auth[] =
    {
	   "sonichy@163.com",
      NULL
    };

  icon = xfce_panel_pixbuf_from_source ("xfce4-sample-plugin", NULL, 32);
  gtk_show_about_dialog (NULL,
                         "logo",         icon,
                         "license",      xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
                         "version",      PACKAGE_VERSION,
                         "program-name", PACKAGE_NAME,
                         "comments",     _("This is a cpu, mem, netspeed plugin."),
                         "website",      PLUGIN_WEBSITE,
                         "copyright",    _("Copyright \xc2\xa9 2023 sonichy"),
                         "authors",      auth,
                         NULL);

  if (icon)
    g_object_unref (G_OBJECT (icon));
}
