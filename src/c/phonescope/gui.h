#ifndef GUI
#define GUI

#include <gtk/gtk.h>
#include "scope.h"

class PhoneScopeGui {
	// widgets
	GtkWidget *window;
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *button;
	GtkWidget *separator;
	GtkWidget *table;
	GtkWidget *drawing_area;

public:
	PhoneScopeGui(alsa_shared *shared_data);
	void run();
	void update_drawing_area();

};

#endif  /* GUI */
