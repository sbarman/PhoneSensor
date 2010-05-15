#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>

#include "scope.h"
#include "gui.h"
#include "datastream.h"

gboolean expose_event_callback(GtkWidget *widget, GdkEventExpose *event,
		gpointer data) {
	PhoneScopeGui *gui = (PhoneScopeGui *) data;
	DrawingAreaVars *dav = gui->drawing_area_vars;

	pthread_mutex_lock(dav->mutex_read);

	short *values = dav->values;
	int values_count = dav->values_count;
	int cur_position = dav->cur_position;
	int time_scale = dav->time_scale;
	int ampl_scale = dav->ampl_scale;

	int screen_width = widget->allocation.width;
	if (values_count / time_scale < screen_width) {
		screen_width = values_count / time_scale;
	}
	int screen_height = widget->allocation.height;
	int origin = (screen_height / 2);

	GdkPoint *points = (GdkPoint *) malloc(sizeof(GdkPoint) * screen_width);

	int buff_position = cur_position - (screen_width * time_scale);
	if (buff_position < 0) {
		buff_position += values_count;
	}

	signed short a = 0;

	for (int c = 0; c < screen_width; c++) {
		a = values[(buff_position) * 2];
		buff_position += time_scale;
		if (buff_position >= values_count) {
			buff_position -= values_count;
		}

		a = (a / ampl_scale) + origin;
		points[c].x = c;
		points[c].y = a;
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

void increment_time_callback(GtkWidget *widget, gpointer data) {
	DrawingAreaVars *dav = (DrawingAreaVars *) data;
	dav->time_scale *= 2;
}

void decrement_time_callback(GtkWidget *widget, gpointer data) {
	DrawingAreaVars *dav = (DrawingAreaVars *) data;
	if (dav->time_scale > 1)
		dav->time_scale /= 2;
}

void increment_ampl_callback(GtkWidget *widget, gpointer data) {
	DrawingAreaVars *dav = (DrawingAreaVars *) data;
	dav->ampl_scale++;
}

void decrement_ampl_callback(GtkWidget *widget, gpointer data) {
	DrawingAreaVars *dav = (DrawingAreaVars *) data;
	if (dav->ampl_scale > 1)
		dav->ampl_scale--;
}

gboolean update_drawing_area_callback(gpointer data) {
	GtkWidget *widget = (GtkWidget *) data;
	gtk_widget_queue_draw(widget);
	gdk_window_process_all_updates();
	return TRUE;
}

gboolean update_heart_rate_callback(gpointer data) {
	PhoneScopeGui *gui = (PhoneScopeGui *) data;

	if (gui->data->heart_rate == 0) {
		gtk_label_set_text(GTK_LABEL(gui->label),"--");
	} else {
		char *text = new char[30];
		sprintf(text, "%.4g", gui->data->heart_rate);
		gtk_label_set_text(GTK_LABEL(gui->label),text);
		delete text;
	}
	return TRUE;
}

void save_file_callback(GtkWidget *widget, gpointer data) {
	PhoneScopeGui *gui = (PhoneScopeGui *) data;
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(gui->window),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
			FALSE);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), ".ekg");
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

void *update_values_thread(void *args) {
	PhoneScopeGui *gui = (PhoneScopeGui *) args;
	DataStream *datastream = gui->datastream;
	DrawingAreaVars *dav = gui->drawing_area_vars;
	int block_size = datastream->block_size_in_frames();
	int frame_size = datastream->frame_size();
	signed short *buffer = (signed short *) malloc(block_size * frame_size);

	while (1) {
		datastream->get_data(buffer, block_size);

		pthread_mutex_lock(dav->mutex_write);
		short * values_buffer = dav->values + (2 * dav->cur_position);
		memcpy(values_buffer, buffer, block_size * frame_size);
		dav->cur_position += block_size;
		if (dav->cur_position >= dav->values_count) {
			dav->cur_position -= dav->values_count;
		}
		pthread_mutex_unlock(dav->mutex_write);
	}
}

PhoneScopeGui::PhoneScopeGui(alsa_shared *shared_data) {

	data = shared_data;

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

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(box1), notebook, TRUE, TRUE, 0);
	gtk_widget_show( notebook);

	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box2, gtk_label_new("ECG"));
	gtk_widget_show( box2);

	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(box2), table, TRUE, TRUE, 0);
	gtk_widget_show( table);

	datastream = shared_data->gui_data_stream;
	drawing_area = gtk_drawing_area_new();
	gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(drawing_area), 0, 1,
			0, 2);
	gtk_widget_add_events(drawing_area, GDK_ALL_EVENTS_MASK);
	gtk_widget_show( drawing_area);

	int values_count = datastream->block_size_in_frames() * 200;
	short *values = (short *) malloc(sizeof(short) * values_count * 2);
	drawing_area_vars = new DrawingAreaVars(values, values_count, 1, 1, 0);

	g_signal_connect(G_OBJECT(drawing_area), "expose_event", G_CALLBACK(
			expose_event_callback), this);

	box2 = gtk_vbox_new(FALSE, 5);
	gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(box2), 1, 2, 0, 2, GTK_SHRINK,
			GTK_SHRINK, 0, 0);
	gtk_widget_show(box2);

	int button_size = 50;

	button = gtk_button_new_with_label("+");
	gtk_signal_connect(GTK_OBJECT(button), "pressed", GTK_SIGNAL_FUNC(
			increment_time_callback), drawing_area_vars);
	gtk_widget_set_size_request(button, button_size, button_size);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	gtk_widget_show( button);

	button = gtk_button_new_with_label("-");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			decrement_time_callback), drawing_area_vars);
	gtk_widget_set_size_request(button, button_size, button_size);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(box2), separator, FALSE, TRUE, 0);
	gtk_widget_show( separator);

	button = gtk_button_new_with_label("+");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			increment_ampl_callback), drawing_area_vars);
	gtk_widget_set_size_request(button, button_size, button_size);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("-");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			decrement_ampl_callback), drawing_area_vars);
	gtk_widget_set_size_request(button, button_size, button_size);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Save");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			save_file_callback), this);
	gtk_widget_set_size_request(button, button_size, button_size);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box2, gtk_label_new("Info"));
	gtk_widget_show(box2);

	box3 = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(box2), box3);
	gtk_widget_show( box3);

	label = gtk_label_new("Heart rate: ");
	gtk_box_pack_start(GTK_BOX(box3), label, FALSE, FALSE, 0);
	gtk_widget_show ( label);

	label = gtk_label_new("--");
	gtk_box_pack_start(GTK_BOX(box3), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
	gtk_widget_show(separator);

	box2 = gtk_hbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, TRUE, 0);
	gtk_widget_show(box2);

	button = gtk_button_new_with_label("close");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			close_application_callback), shared_data);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	gtk_widget_show( window);

	g_timeout_add(100, update_drawing_area_callback, drawing_area);
	g_timeout_add_seconds(1, update_heart_rate_callback, this);

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
	cur_position = 0;
	mutex_read = new pthread_mutex_t();
	pthread_mutex_init(mutex_read, NULL);
	mutex_write = new pthread_mutex_t();
	pthread_mutex_init(mutex_write, NULL);
}

