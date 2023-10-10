#include <string.h>

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <ctime>
#include <string>

GtkWidget *main_window;
GtkWidget *main_hbox;
GtkWidget *main_lbox;
GtkWidget *drawing_area;
GtkWidget *text_label;


GstElement *gtkglsink, *videosink;
GstBus *bus;
GstEvent *event;
GstStateChangeReturn ret;

std::time_t starttime;
bool starttrigger=true;



gint64 seek_time_ns ;
bool numvideo = true;
int period=3;

bool step = true;


const gchar *video1;
const gchar *video2 ;



typedef struct _CustomData {
  GstElement *playbin; 
  GtkWidget *sink_widget;
} CustomData;
CustomData data;




static gboolean update_timer(gpointer user_data);
static gboolean state_video(gpointer datas);
static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data);
static gboolean cb_print_position (GstElement *pipeline);
static void     delete_event_cb (GtkWidget *widget, GdkEvent *event, CustomData *data);
static void     seek_to_time (GstElement *pipeline,gint64      time_nanoseconds);
static void     create_ui (CustomData *data) ;
static gboolean timercallback(gpointer datas);

int main(int argc, char *argv[]) {
  
  gtk_init (&argc, &argv);
  gst_init (&argc, &argv);

  char *numberStr = argv[1];
  period = atoi(numberStr);

  video1 = argv[2];
  video2 = argv[3];

  g_printerr("%s\n", video1);
   g_printerr("%s\n", video2);


  data.playbin = gst_element_factory_make ("playbin", "playbin");
  videosink = gst_element_factory_make ("glsinkbin", "glsinkbin");
  gtkglsink = gst_element_factory_make ("gtkglsink", "gtkglsink");



    g_object_set (videosink, "sink", gtkglsink, NULL);
    g_object_get (gtkglsink, "widget", &data.sink_widget, NULL);

  g_object_set (data.playbin, "uri", video2, NULL);
  g_object_set (data.playbin, "video-sink", videosink, NULL);

  create_ui (&data);

  gst_element_set_state (data.playbin, GST_STATE_PLAYING);
  bus = gst_element_get_bus(data.playbin);
  gst_bus_add_watch(bus, bus_callback, NULL);




  g_timeout_add_seconds(period,state_video,NULL);
  



  gtk_main ();




  gst_element_set_state (data.playbin, GST_STATE_NULL);
  gst_object_unref (data.playbin);
  gst_object_unref (videosink);

  return 0;
}






static gboolean timercallback(gpointer datas) 
{
      gchar *timeStr = g_strdup_printf("%ld", std::time(nullptr) - starttime);
      gtk_label_set_text(GTK_LABEL(text_label),timeStr);
  return G_SOURCE_CONTINUE;
}



static gboolean state_video(gpointer datas) 
{
  
  const gchar *currenturi;
  if (numvideo) {currenturi = video1;} else {currenturi = video2;}
  numvideo = !numvideo;
  gst_element_set_state(data.playbin, GST_STATE_NULL);
  g_object_set(data.playbin, "uri", currenturi, NULL);
  gst_element_set_state(data.playbin, GST_STATE_PLAYING);
  step=true;
  return G_SOURCE_CONTINUE;
}







static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer datas) {
    switch (GST_MESSAGE_TYPE(msg)) 
    {

                    case GST_MESSAGE_STATE_CHANGED: 
                    {

                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

                        if (new_state == GST_STATE_PLAYING && step) 
                        {            
                              if(numvideo){break;}
                              if(starttrigger){starttrigger=false;   starttime = std::time(nullptr);  g_timeout_add_seconds(1,timercallback,NULL);  }


                              seek_time_ns=(std::time(nullptr)-starttime)* GST_SECOND;
                              gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), seek_time_ns);
                              step=false;
                                             
                        }            
                        break;
                    }

        default:break;
    }

    return TRUE;
}







static gboolean cb_print_position (GstElement *pipeline)
{
  gint64 pos, len;

  if (gst_element_query_position (pipeline, GST_FORMAT_TIME, &pos)
    && gst_element_query_duration (pipeline, GST_FORMAT_TIME, &len)) {
    g_print ("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
         GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
  }

  return TRUE;
}



static void seek_to_time(GstElement *pipeline,gint64 time_nanoseconds)
{
  if (!gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,GST_SEEK_TYPE_SET, time_nanoseconds,GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) 
  {   g_print ("Seek failed!\n");  }
}




static void delete_event_cb (GtkWidget *widget, GdkEvent *event, CustomData *data) {gtk_main_quit ();}

static void create_ui (CustomData *data) {
  
main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
gtk_window_set_resizable(GTK_WINDOW(main_window), FALSE);
g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), data);
main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
main_lbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);


GdkRGBA color;
gdk_rgba_parse(&color, "gray");
text_label = gtk_label_new("");
PangoFontDescription *font_desc = pango_font_description_new();
pango_font_description_set_size(font_desc, 24 * PANGO_SCALE);
gtk_widget_override_font(text_label, font_desc);
gtk_widget_override_color(text_label, GTK_STATE_FLAG_NORMAL, &color);


GtkWidget *overlay = gtk_overlay_new();
gtk_container_add(GTK_CONTAINER(overlay), main_hbox);
gtk_overlay_add_overlay(GTK_OVERLAY(overlay), main_lbox);
gtk_box_pack_start(GTK_BOX(main_hbox), data->sink_widget, TRUE, TRUE, 0);
gtk_box_pack_start(GTK_BOX(main_lbox), text_label, FALSE, FALSE, 0);
gtk_container_add(GTK_CONTAINER(main_window), overlay);
gtk_widget_show_all(main_window);

}




