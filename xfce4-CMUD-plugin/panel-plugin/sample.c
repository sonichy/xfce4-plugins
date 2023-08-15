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

#include "sample.h"
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


static gboolean plugin_clicked(GtkWidget *widget, GdkEventButton *event, SamplePlugin *sample)
{
  if (event->button != 1 || event->state & GDK_CONTROL_MASK)
    return FALSE;

  if (sample == NULL)
    return FALSE;

  g_spawn_command_line_sync("xfce4-taskmanager", NULL, NULL, NULL, NULL);
  return TRUE;
}


static gboolean update_cb (gpointer data)
{
  SamplePlugin *sample = data;
  update (sample);
  return TRUE;
}

char *uptime (char *hms)
{
  int ss, h, m, s; 
  float tt, it;
  FILE *fp;
  fp = fopen("/proc/uptime", "r");
  fscanf(fp, "%f %f", &tt, &it);
  fclose(fp);
  ss = (int) tt;
  h = ss / 3600;
  m = ss % 3600 / 60;
  s = ss % 3600 % 60;
  sprintf(hms, "%02d:%02d:%02d", h, m, s);
  return hms;
}


void stat(SamplePlugin *sample)
{  
  char ch[100], cpu[10];
  long long user, nice, sys, idle, iowait, irq, softirq, tt;
  FILE *fp;
  fp = fopen("/proc/stat", "r"); 
  fgets(ch, sizeof(ch), fp); 
  fclose(fp);  
  sscanf(ch, "%s%d%d%d%d%d%d%d", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
  tt = user + nice + sys + idle + iowait + irq + softirq;
  if (sample->i > 0)
    sample->cpu_usage = ((tt - sample->tt0) - (idle - sample->idle0)) * 100 / (tt - sample->tt0);
  else
    sample->cpu_usage = 0;
  //printf("%s,%d,%d,%d,%d,%d,%d,%d\n",cpu,user,nice,sys,idle,iowait,irq,softirq);
  //sprintf(cpu_usage, "%d%%", usage);
  sample->idle0 = idle;
  sample->tt0 = tt;
  sample->i++;
  if (sample->i > 1)
  	sample->i == 1;
}


//通过结构体保存多个值
void meminfo(SamplePlugin *sample)
{ 
  FILE *fp;
  char cmt[40], cma[40], a[40], b[40];
  fp = fopen("/proc/meminfo", "r"); 
  fgets(cmt, sizeof(cmt), fp);
  fgets(cma, sizeof(cma), fp);
  fgets(cma, sizeof(cma), fp);
  fclose(fp);
  sscanf(cmt, "%s%d%s", a, &sample->mt, b);
  sscanf(cma, "%s%d%s", a, &sample->ma, b);
}

void netdev(SamplePlugin *sample)
{
  FILE *fp;
  char s[150], itf[10];
  long long r1, r2, r3, r4, r5, r6, r7, r8, t1, t2, t3, t4, t5, t6, t7, t8;
  int i=0;
  sample->rb = 0;
  sample->tb = 0;
  fp = fopen("/proc/net/dev", "r");
  while (1) {        
    fgets(s, sizeof(s), fp);
    if (feof(fp)) {
      fclose(fp);
      break;
    }
    if (i > 1) {
      sscanf(s, "%s%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld%Ld", itf, &r1, &r2, &r3, &r4, &r5, &r6, &r7, &r8, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8);      
      //g_message("%d\t%s", i, s);
      sample->rb += r1;
      sample->tb += t1;
    }
    i++;
  }
}

char *B2G (long long b)
{  
  //static char g[10]; //value do not change
  //char g[10]; //null
  //char *g; //crash
  //https://blog.csdn.net/kturing/article/details/78874453  
  char *g = malloc(10);
  float f;
  if (b > 999999999) {
    f = b / 1073741824.0;
    if (f >= 100)
      sprintf(g, "%4.0f GB", f);
    else if (f >= 10)
      sprintf(g, "%.1f GB", f);
    else
      sprintf(g, "%.2f GB", f);
  } else {
    if (b > 999999) {
      f = b / 1048576.0;
      if (f >= 100)
        sprintf(g, "%4.0f MB", f);
      else if (f >= 10)
      	 sprintf(g, "%.1f MB", f);
      else
        sprintf(g, "%.2f MB", f);
    } else {
      if (b > 999) {
        f = b / 1024.0;
        if (f >= 100)
          sprintf(g, "%4.0f KB", f);
        else if (f >= 10)
          sprintf(g, "%.1f KB", f);
        else
          sprintf(g, "%.2f KB", f);
      } else
        sprintf(g, "%4d KB", 0);
    }
  }
  return g;
}


void update (SamplePlugin *sample)
{
  char text[100], text1[100], hms[10];
  int mem_usage;
  long long mu, rs, ts;
  char *css, *css1, *mus;
  uptime(hms);  
  stat(sample);
  meminfo(sample);
  netdev(sample);
  mu = sample->mt - sample->ma;
  mem_usage = mu * 100 / sample->mt;
  rs = sample->rb - sample->rb0;
  ts = sample->tb - sample->tb0;
  sample->rb0 = sample->rb;
  sample->tb0 = sample->tb;
  //sprintf(text, "CPU: %d%%\nMEM: %d%%", sample->cpu_usage, mem_usage);
  sprintf(text, "↑%s/s\n↓%s/s", B2G(ts), B2G(rs));
  sprintf(text1, "UPT: %s\nCPU: %d%%\nMEM: %s / %s = %d%%\n↑: %s\n↓: %s", hms, sample->cpu_usage, B2G(mu * 1024), B2G(sample->mt * 1024), mem_usage, B2G(sample->tb), B2G(sample->rb));
  //gtk_label_set_text (GTK_LABEL(sample->label), text);
  gtk_button_set_label (GTK_BUTTON(sample->label), text);
  gtk_widget_set_tooltip_text(sample->label, text1);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(sample->progress_cpu), (double) sample->cpu_usage / 100);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(sample->progress_mem), (double) mem_usage / 100);
  css = "progressbar progress { background-color:white; background-image:none; border-color:white; }";
  css1 = "progressbar progress { background-color:red; background-image:none; border-color:red; }";
  if (sample->cpu_usage < 90)
    gtk_css_provider_load_from_data ((GtkCssProvider*) g_object_get_data(G_OBJECT(sample->progress_cpu), "css_provider"), css, -1, NULL);
  else
    gtk_css_provider_load_from_data ((GtkCssProvider*) g_object_get_data(G_OBJECT(sample->progress_cpu), "css_provider"), css1, -1, NULL);
  if (mem_usage < 90)
    gtk_css_provider_load_from_data ((GtkCssProvider*) g_object_get_data(G_OBJECT(sample->progress_mem), "css_provider"), css, -1, NULL);
  else
    gtk_css_provider_load_from_data ((GtkCssProvider*) g_object_get_data(G_OBJECT(sample->progress_mem), "css_provider"), css1, -1, NULL);
}

static SamplePlugin *sample_new (XfcePanelPlugin *plugin)
{
  SamplePlugin   *sample;
  GtkOrientation  orientation;
  char *css;
  PangoFontDescription *font;
  
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
  sample->progress_mem = gtk_progress_bar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE(sample->progress_mem), GTK_ORIENTATION_VERTICAL);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(sample->progress_mem), 0.5);
  gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR(sample->progress_mem), TRUE);
  //systemload.cc: L309, L514
  css = "progressbar progress { background-color:white; background-image:none; border-color:white; }";
  GtkCssProvider *css_provider_mem = gtk_css_provider_new ();
  gtk_style_context_add_provider (GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (sample->progress_mem))), GTK_STYLE_PROVIDER (css_provider_mem),  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data (css_provider_mem, css, -1, NULL);
  g_object_set_data (G_OBJECT(sample->progress_mem), "css_provider", css_provider_mem);
  gtk_widget_show (sample->progress_mem);
  gtk_box_pack_start (GTK_BOX (sample->hvbox), sample->progress_mem, FALSE, FALSE, 0);
  
  //sample->label = gtk_label_new ("00:00:00");
  sample->label = gtk_button_new_with_label ("↑   0 KB/s\n↓   0 KB/s");
  g_signal_connect (sample->label, "button-press-event", G_CALLBACK(plugin_clicked), sample);  
  gtk_widget_show (sample->label);
  gtk_box_pack_start (GTK_BOX (sample->hvbox), sample->label, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (sample->label, "UPT:\nMEM:\n↑:\n↓:");
  font = pango_font_description_from_string ("Mono");
  gtk_widget_override_font (GTK_WIDGET(sample->label), font);
  pango_font_description_free (font);

  sample->progress_cpu = gtk_progress_bar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE(sample->progress_cpu), GTK_ORIENTATION_VERTICAL);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(sample->progress_cpu), 0.2);
  gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR(sample->progress_cpu), TRUE);
  GtkCssProvider *css_provider_cpu = gtk_css_provider_new ();
  gtk_style_context_add_provider (GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (sample->progress_cpu))), GTK_STYLE_PROVIDER (css_provider_cpu),  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data (css_provider_cpu, css, -1, NULL);
  g_object_set_data (G_OBJECT(sample->progress_cpu), "css_provider", css_provider_cpu);
  gtk_widget_show (sample->progress_cpu);
  gtk_box_pack_start (GTK_BOX (sample->hvbox), sample->progress_cpu, FALSE, FALSE, 0);

  sample->i = 0;
  sample->rb = 0;
  sample->tb = 0;
  sample->rb0 = 0;
  sample->tb0 = 0;
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


