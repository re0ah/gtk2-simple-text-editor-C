#include <stdint.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#define NO_FILE_LOADED (name_current_file[0] == '\0')
char* title = "Untitled";
char name_current_file[64];

struct
{
	GtkWindow* self;
	GtkVBox* vbox;
	GtkAccelGroup* accel;
	GtkLabel* lncol;
} main_window;
GtkClipboard* clipboard;

struct
{
	GtkMenuShell* menubar;

	GtkMenuShell* filemenu;
	struct
	{
		GtkMenuItem* self;

		GtkWidget* new;
		GtkWidget* open;
		GtkWidget* save;
		GtkWidget* saveas;
		GtkWidget* quit;
	} file;
	GtkMenuShell* editmenu;
	struct
	{
		GtkMenuItem* self;

		GtkWidget* cut;
		GtkWidget* copy;
		GtkWidget* paste;
		GtkWidget* delete;
		GtkWidget* selectall;
		GtkWidget* wrapping;
		GtkWidget* select_font;
	} edit;
	GtkMenuShell* helpmenu;
	struct
	{
		GtkMenuItem* self;

		GtkWidget* about;
	} help;
} gui_menu;

struct
{
	GtkTextBuffer* buffer;
	GtkTextView* area;
	GtkScrolledWindow* scroll;
} gui_text;
uint8_t modified = FALSE;
uint8_t wrapping = 0;
