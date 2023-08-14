# XFCE plugins
## Documentation
[HowTo](https://wiki.xfce.org/dev/howto/panel_plugins)  
[GTK](https://docs.gtk.org/gtk3/)  
[GLib](https://docs.gtk.org/glib/)  
[Sample_xfce_4.16](https://gitlab.xfce.org/panel-plugins/xfce4-sample-plugin/-/tree/4cadf5c2212669875982caf9e17fa808d69412e2)  
[DateTime](https://docs.xfce.org/panel-plugins/xfce4-datetime-plugin/start)  
[Systemload](https://docs.xfce.org/panel-plugins/xfce4-systemload-plugin/start)  
[lunar-calendar](https://blog.csdn.net/ericden/article/details/122334507)
[获取年月日时分秒](https://coder55.com/article/158257)  
[g_timeout_add 不运行：回调函数必须有返回值为TURE](https://blog.csdn.net/baomi2007/article/details/2721515)

## Dependence
libxfce4ui-2-dev, libxfce4panel-2.0-dev (xfce4-dev-tools)

## Compile
1.Download sample plugin source files.  
2.Run ./configure (autogen.sh) , install package needed.  
3.Modify ./panel-plugin/*.c  
4.Run make  
5.Run make install:
```
sudo cp panel-plugin/.libs/libsample.so /usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libsample.so
sudo cp panel-plugin/sample.desktop /usr/share/xfce4/panel/plugins/sample.desktop 
(sudo cp panel-plugin/sample.desktop /usr/local/share/xfce4/panel/plugins/sample.desktop)
```
### Modify Name : Failed
libsample.so ->  libHTYdatetime.so

./panel-plugin/Makefile.am
```
plugin_LTLIBRARIES = \
	libHTYdatetime.la

libsample_la_SOURCES = \
	sample.c \
	sample.h \
	sample-dialogs.c \
	sample-dialogs.h

desktop_DATA =								\
	HTYdatetime.desktop

EXTRA_DIST =								\
	HTYdatetime.desktop.in
```
./autogen.sh

#### No rule to make target "libHTYdatetime.c" needed by "libHTYdatetime.lo". stop.

## configure.ac.in
```
m4_define([sample_version_major], [1])
m4_define([sample_version_minor], [0])
m4_define([sample_version_micro], [0])
AC_INIT([PACKAGE_NAME], [version],  [url], [PACKAGE_TARNAME])
```
./autogen.sh

## Locale
[https://blog.csdn.net/u013293645/article/details/45692597](https://blog.csdn.net/u013293645/article/details/45692597)  
### Edit *.po
```
msgid "HTYdatetime"
msgstr "海天鹰日期时间"
```
### Use in *.c
`_("HTYdatetime")`
### Build
`sudo apt-get install gettext`

Decode po to gmo:  
`msgfmt -o po/zh_CN.gmo po/zh_CN.po`

Undecode gmo to po:  
`msgunfmt po/zh_CN.gmo -o po/zh_CN.po`

### Install
sudo make install
```
/bin/mkdir -p '/usr/local/share/icons/hicolor/48x48/apps'
/usr/bin/install -c -m 644 xfce4-sample-plugin.png '/usr/local/share/icons/hicolor/48x48/apps'
installing ar.gmo as /usr/local/share/locale/ar/LC_MESSAGES/xfce4-sample-plugin.mo
...
```
Why the system plugins do not have *.mo in these directory ?
### *.desktop Chinese locale mess code

## Settings
~/.config/xfce4/panel/*.rc

###  sample.c
#### Plugin Right Menu [Property]  
`g_signal_connect (G_OBJECT (plugin), "configure-plugin",  G_CALLBACK (sample_configure), sample);`

### sample-dialogs.c
```
void sample_configure (XfcePanelPlugin *plugin, SamplePlugin *sample)
{
    dialog = xfce_titled_dialog_new_with_buttons(...);
    textView = gtk_text_view_new ();
    g_signal_connect (G_OBJECT(textView), "focus-out-event", G_CALLBACK (textView_change_cb), sample);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK(sample_configure_response), sample);
}

static gboolean textView_change_cb(GtkWidget *widget, GdkEventFocus *ev, SamplePlugin *sample)
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

static void sample_configure_response (GtkWidget *dialog,  gint response, SamplePlugin *sample)
{
    gboolean result;

    if (response == GTK_RESPONSE_HELP) {
      result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);
      if (G_UNLIKELY (result == FALSE))
          g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }  else {
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
```
###  sample.c
```
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
          xfce_rc_write_entry    (rc, "setting1", sample->setting1);

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
            value = xfce_rc_read_entry (rc, "setting1", "");
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

void update (SamplePlugin *sample)
{
    gtk_widget_set_tooltip_text(sample->label, sample->setting1);
}
```

