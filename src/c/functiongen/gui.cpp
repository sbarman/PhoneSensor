#include <gtk/gtk.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <SDL/SDL.h>

#import "sin.h"
#import "gui.h"

fnc_info function_info;

void text_view_toggle_editable(GtkWidget *checkbutton, GtkWidget *text_view) {
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), GTK_TOGGLE_BUTTON(
			checkbutton)->active);
	function_info.freq += 100;
	function_info.ampl += .1;
}

void range_set_amplitude(GtkRange *range, gpointer user_data) {
	function_info.ampl = gtk_range_get_value(range);
}

void range_set_frequency(GtkRange *range, gpointer user_data) {
	function_info.freq = gtk_range_get_value(range);
}

void range_set_offset(GtkRange *range, gpointer user_data) {
	function_info.offset = gtk_range_get_value(range);
}

void toggle_set_sin(GtkToggleButton *togglebutton, gpointer user_data) {
	if (gtk_toggle_button_get_active(togglebutton)) {
		function_info.type = SIN;
	}
}

void toggle_set_square(GtkToggleButton *togglebutton, gpointer user_data) {
	if (gtk_toggle_button_get_active(togglebutton)) {
		function_info.type = SQUARE;
	}
}

void toggle_set_triangle(GtkToggleButton *togglebutton, gpointer user_data) {
	if (gtk_toggle_button_get_active(togglebutton)) {
		function_info.type = TRIANGLE;
	}
}

void close_application(GtkWidget *widget, gpointer data) {
	gtk_main_quit();
}

int gui_start(int argc, char *argv[]) {
	GtkWidget *window;
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *check;
	GtkWidget *separator;
	GtkWidget *table;
	GtkWidget *vscrollbar;
	GtkWidget *text_view;
	GtkTextBuffer *buffer;
	GtkWidget *radio_button;
	GtkWidget *hscale;
	GdkColormap *cmap;
	GdkColor color;
	GdkFont *fixed_font;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(
			close_application), NULL);
	gtk_window_set_title(GTK_WINDOW(window), "Function Generator");
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);

	box1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), box1);
	gtk_widget_show(box1);

	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, TRUE, TRUE, 0);
	gtk_widget_show(box2);

	table = gtk_table_new(5, 3, FALSE);
	gtk_table_set_row_spacing(GTK_TABLE(table), 0, 2);
	gtk_table_set_col_spacing(GTK_TABLE(table), 0, 2);
	gtk_box_pack_start(GTK_BOX(box2), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	radio_button = gtk_radio_button_new_with_label(NULL, "sin");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), true);
	gtk_table_attach_defaults(GTK_TABLE(table), radio_button, 2, 3, 0, 1);
	gtk_signal_connect(GTK_OBJECT(radio_button), "toggled", GTK_SIGNAL_FUNC(
			toggle_set_sin), NULL);
	gtk_widget_show(radio_button);

	radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(
			radio_button), "square");
	gtk_table_attach_defaults(GTK_TABLE(table), radio_button, 2, 3, 1, 2);
	gtk_signal_connect(GTK_OBJECT(radio_button), "toggled", GTK_SIGNAL_FUNC(
			toggle_set_square), NULL);
	gtk_widget_show(radio_button);

	radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(
			radio_button), "triangle");
	gtk_table_attach_defaults(GTK_TABLE(table), radio_button, 2, 3, 2, 3);
	gtk_signal_connect(GTK_OBJECT(radio_button), "toggled", GTK_SIGNAL_FUNC(
			toggle_set_triangle), NULL);
	gtk_widget_show(radio_button);

	hscale = gtk_hscale_new_with_range(0, 10000, 10);
	gtk_table_attach_defaults(GTK_TABLE(table), hscale, 0, 2, 2, 3);
	gtk_scale_set_draw_value(GTK_SCALE(hscale), true);
	gtk_range_set_value(GTK_RANGE(hscale), function_info.freq);
	gtk_widget_show(hscale);
	gtk_signal_connect(GTK_OBJECT(hscale), "value-changed", GTK_SIGNAL_FUNC(
			range_set_frequency), NULL);

	hscale = gtk_hscale_new_with_range(0, 1, .005);
	gtk_table_attach_defaults(GTK_TABLE(table), hscale, 0, 2, 1, 2);
	gtk_scale_set_draw_value(GTK_SCALE(hscale), true);
	gtk_range_set_value(GTK_RANGE(hscale), function_info.ampl);
	gtk_widget_show(hscale);
	gtk_signal_connect(GTK_OBJECT(hscale), "value-changed", GTK_SIGNAL_FUNC(
			range_set_amplitude), NULL);

	hscale = gtk_hscale_new_with_range(-5000, 5000, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), hscale, 0, 2, 3, 4);
	gtk_scale_set_draw_value(GTK_SCALE(hscale), true);
	gtk_range_set_value(GTK_RANGE(hscale), function_info.offset);
	gtk_widget_show(hscale);
	gtk_signal_connect(GTK_OBJECT(hscale), "value-changed", GTK_SIGNAL_FUNC(
			range_set_offset), NULL);

	/* Create the GtkText widget */
	text_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), text_view, 0, 1, 0, 1);
	gtk_widget_show(text_view);

	/* Add a vertical scrollbar to the GtkText widget */
	vscrollbar = gtk_vscrollbar_new(gtk_container_get_focus_vadjustment(
			GTK_CONTAINER(text_view)));
	gtk_table_attach_defaults(GTK_TABLE(table), vscrollbar, 1, 2, 0, 1);
	gtk_widget_show(vscrollbar);

	/* Realizing a widget creates a window for it,
	 * ready for us to insert some text */
	gtk_widget_realize(text_view);

	/* Insert some colored text */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(buffer, "Hello, this is some text", -1);

	hbox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(box2), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	check = gtk_check_button_new_with_label("Editable");
	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(check), "toggled", GTK_SIGNAL_FUNC(
			text_view_toggle_editable), text_view);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
	gtk_widget_show(check);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
	gtk_widget_show(separator);

	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, TRUE, 0);
	gtk_widget_show(box2);

	button = gtk_button_new_with_label("close");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(
			close_application), NULL);
	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	gtk_widget_show(window);

	gtk_main();

	return (0);
}

static void *sound_thread(void *args) {
	snd_args *sound_args = (snd_args *) args;
	sin_start(sound_args->argc, sound_args->argv, sound_args->function_info);
}

static void *input_thread(void *args) {
	fnc_info *function_info = (fnc_info *) args;
	while (1) {
		// Read keyboard events. Quit on backspace
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_q:
					// Frequency scale up

					break;
				case SDLK_a:
					// Frequency scale up
					function_info->freq -= 100;
					break;
				default:
					// Ignore
					break;
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	snd_args sound_args = { argc, argv, &function_info };

	pthread_t sound_tid;
	if (pthread_create(&sound_tid, NULL, sound_thread, &sound_args) != 0) {
		fprintf(stdout, "Count not create gui thread.\n");
	}

	pthread_t input_tid;
	if (pthread_create(&input_tid, NULL, input_thread, &function_info) != 0) {
		fprintf(stdout, "Count not create input thread.\n");
	}

	gui_start(argc, argv);

	return 1;
}
