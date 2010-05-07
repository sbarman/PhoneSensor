#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>

#include "scope.h"
#include "gui.h"
#include "datastream.h"

gboolean screen_change_callback(GtkWidget *widget, GdkEventConfigure *event,
		gpointer user_data) {
	printf("screen change\n");
	PhoneScopeGui *gui = (PhoneScopeGui *) user_data;
	int values_count = widget->allocation.width;
	printf("val %d\n", values_count);
	DrawingAreaVars *dav = gui->drawing_area_vars;
	pthread_mutex_lock(dav->mutex_read);
	pthread_mutex_lock(dav->mutex_write);

	free(gui->drawing_area_vars->values);
	gui->drawing_area_vars->values = (short *) malloc(sizeof(short)
			* values_count);
	gui->drawing_area_vars->values_count = values_count;

	pthread_mutex_unlock(dav->mutex_write);
	pthread_mutex_unlock(dav->mutex_read);

	return TRUE;
}

gboolean expose_event_callback(GtkWidget *widget, GdkEventExpose *event,
		gpointer data) {
	printf("expose\n");
	PhoneScopeGui *gui = (PhoneScopeGui *) data;
	DrawingAreaVars *dav = gui->drawing_area_vars;

	pthread_mutex_lock(dav->mutex_read);

	short *values = dav->values;
	int screen_width = widget->allocation.width;
	printf("sc %d %d\n", screen_width, dav->values_count);
	if (dav->values_count < screen_width) {
		screen_width = dav->values_count;
	}
	int screen_height = widget->allocation.height;
	GdkPoint *points = (GdkPoint *) malloc(sizeof(GdkPoint) * screen_width);

	signed short v = 0;
	int origin = (screen_height / 2) + dav->offset;

	for (int c = 0; c < screen_width; c++) {
		v = values[c];
		int disp_v = (v / dav->ampl_scale) + origin;
		points[c].x = c;
		points[c].y = disp_v;
	}

	pthread_mutex_unlock(dav->mutex_read);

	gdk_draw_points(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)], points, screen_width);
	free(points);
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
	gdk_window_process_all_updates();
	return TRUE;
}

void *update_values_thread(void *args) {
	PhoneScopeGui *gui = (PhoneScopeGui *) args;
	DataStream *datastream = gui->datastream;
	DrawingAreaVars *dav = gui->drawing_area_vars;
	signed short *buffer = (signed short *) malloc(
			datastream->block_size_in_frames() * datastream->frame_size());

	while (1) {
		int block_size = datastream->block_size_in_frames();
		datastream->get_data(buffer, block_size);

		pthread_mutex_lock(dav->mutex_write);

		short *values = dav->values;
		int time_scale = dav->time_scale;

		int shift = block_size / time_scale;
		//	memcpy(values + shift, values, sizeof(short) * (dav->values_count - shift));

		for (int i = 0; i < dav->values_count; i++) {
			values[i] = buffer[2 * (i) * time_scale];
		}
		pthread_mutex_unlock(dav->mutex_write);

	}
	/*
	 // Render the screen

	 int buffer_size = datastream->block_size_in_frames();
	 */

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

	datastream = shared_data->source->getDataStream();
	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, 100, 100);
	gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(drawing_area), 0, 1,
			0, 1);
	gtk_widget_add_events(drawing_area, GDK_ALL_EVENTS_MASK);
	gtk_widget_show( drawing_area);

	int values_count = GTK_WIDGET(drawing_area)->allocation.width;
	short *values = (short *) malloc(sizeof(short) * values_count);
	drawing_area_vars = new DrawingAreaVars(values, values_count, 1, 1, 0);
	printf("first %d %d\n", values_count, drawing_area_vars->values_count);

	g_signal_connect(G_OBJECT(drawing_area), "expose_event", G_CALLBACK(
			expose_event_callback), this);
	g_signal_connect(G_OBJECT(drawing_area), "configure-event", G_CALLBACK(
			screen_change_callback), this);

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

	gtk_widget_show( window);

	g_timeout_add(100, update_drawing_area_callback, drawing_area);

	pthread_t read_tid;
	int ret = pthread_create(&read_tid, NULL, update_values_thread, this);
	if (ret < 0) {
		fprintf(stderr, "failed to create reader thread %d\n", ret);
	}

}

void PhoneScopeGui::run() {
	gtk_main();
}

DrawingAreaVars::DrawingAreaVars(short *v, int vc, int ts, int as, int o) {
	values = v;
	values_count = vc;
	time_scale = ts;
	ampl_scale = as;
	offset = o;
	mutex_read = new pthread_mutex_t();
	pthread_mutex_init(mutex_read, NULL);
	mutex_write = new pthread_mutex_t();
	pthread_mutex_init(mutex_write, NULL);
}

