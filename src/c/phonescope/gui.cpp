#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>

#include "scope.h"
#include "gui.h"

gboolean expose_event_callback(GtkWidget *widget,
		GdkEventExpose *event, gpointer data) {
	alsa_shared *shared_data = (alsa_shared *) data;
	// Render the screen
	signed short *buffer = (signed short *) shared_data->buffer;

	int screen_width = widget->allocation.width;
	int time_div = 16;
	int voltage_div = 50;
	if (shared_data->frames_in_buffer < screen_width * time_div) {
		screen_width = shared_data->frames_in_buffer / time_div;
	}

	int buffer_position = shared_data->writer_position
			- (screen_width * time_div);

	if (buffer_position < 0) {
		buffer_position += shared_data->frames_in_buffer;
	}

	GdkPoint *points = (GdkPoint *) malloc(sizeof(GdkPoint) * screen_width);
	// free-running waveform or we found a valid value above threshold

	signed short v = buffer[(buffer_position)];
	int origin = widget->allocation.height / 2;

	for (int c = 0; c < screen_width; c++) {
		v = buffer[(buffer_position) * 2];
		buffer_position += time_div;
		if (buffer_position >= shared_data->frames_in_buffer) {
			buffer_position -= shared_data->frames_in_buffer;
		}

		int disp_v = (v / voltage_div) + origin;
		points[c].x = c;
		points[c].y = disp_v;

	}

	gdk_draw_points(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)], points, screen_width);
	delete points;
	return TRUE;
}

void close_application_callback(GtkWidget *widget, gpointer data) {
	alsa_shared *shared_data = (alsa_shared *) data;
	shared_data->running = false;

	gtk_main_quit();
}

gboolean update_drawing_area_callback(gpointer data) {
	GtkWidget *widget = (GtkWidget *) data;
	gtk_widget_queue_draw(widget);
	//gdk_window_process_all_updates();
	return TRUE;
}

void PhoneScopeGui::update_drawing_area() {
	GtkWidget *widget = drawing_area;
	gtk_widget_queue_draw(widget);
	//gdk_window_process_all_updates();
}

PhoneScopeGui::PhoneScopeGui(alsa_shared *shared_data) {

	gtk_init(NULL, NULL);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(
			close_application_callback), shared_data);

	gtk_window_set_title(GTK_WINDOW(window), "Phone Scope");
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);

	box1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), box1);
	gtk_widget_show( box1);

	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, TRUE, TRUE, 0);
	gtk_widget_show( box2);

	table = gtk_table_new(2, 2, FALSE);
	//gtk_table_set_row_spacing(GTK_TABLE(table), 0, 2);
	//gtk_table_set_col_spacing(GTK_TABLE(table), 0, 2);
	gtk_box_pack_start(GTK_BOX(box2), table, TRUE, TRUE, 0);
	gtk_widget_show( table);

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, 100, 100);
	gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(drawing_area), 0, 1,
			0, 1);
	g_signal_connect(G_OBJECT(drawing_area), "expose_event", G_CALLBACK(
			expose_event_callback), shared_data);
	gtk_widget_add_events(drawing_area, GDK_ALL_EVENTS_MASK);
	gtk_widget_show( drawing_area);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
	gtk_widget_show( separator);

	box2 = gtk_hbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, TRUE, 0);
	gtk_widget_show(box2);

	button = gtk_button_new_with_label("close");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			close_application_callback), shared_data);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default( button);
	gtk_widget_show(button);

	gtk_widget_show(window);

	g_timeout_add (100, update_drawing_area_callback, drawing_area);

}

void PhoneScopeGui::run() {
	gtk_main();
}

