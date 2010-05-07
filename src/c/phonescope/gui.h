#ifndef GUI
#define GUI

#include <gtk/gtk.h>
#include "scope.h"
#include "datastream.h"

// variables for drawing area
class DrawingAreaVars {
public:
	DrawingAreaVars(short *v, int vc, int ts, int as, int o);
	short* values;
	int values_count;
	int time_scale;
	int ampl_scale;
	int offset;
  pthread_mutex_t *mutex_read;
  pthread_mutex_t *mutex_write;
};

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
	DataStream *datastream;
	DrawingAreaVars *drawing_area_vars;

	PhoneScopeGui(alsa_shared *shared_data);
	void run();
};
#endif  /* GUI */
