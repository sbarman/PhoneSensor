struct snd_args {
	int argc;
	char **argv;
	fnc_info *function_info;
};

static void callback(GtkWidget *widget, gpointer data);
static void *gui_thread(void *args);
