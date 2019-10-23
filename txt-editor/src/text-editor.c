#include "text-editor.h"
#include <stdlib.h>
#include "u64to_str.h"

void gui_set_title(char* filename)
{
	size_t len = strlen(filename);
	char* end = filename + len;
#ifdef _WIN32
	#define SLASH '\\'
#else
	#define SLASH '/'
#endif
	while(end > filename)
	{
		end--;
		if(*end == SLASH)
		{
			end++;
			break;
		}
	}
	gtk_window_set_title(main_window.self, end);
}

char gui_file_save(char* filename)
{
	FILE* fptr = fopen(filename, "w");
	/*unfortunately, i don't know how to replace GtkTextBuffer. I hate him.
	  Developers of this structure didn't even do such a simple thing, how
	  return of pointer to data, and have to call 3 functions for get pointer.*/
	GtkTextIter start;
	GtkTextIter end;
	gtk_text_buffer_get_start_iter(gui_text.buffer, &start);
	gtk_text_buffer_get_end_iter(gui_text.buffer, &end);

	char* buf = gtk_text_buffer_get_text(gui_text.buffer,
										&start,
										&end,
										TRUE);

	fwrite(buf, strlen(buf), 1, fptr);
	free(buf);
	fclose(fptr);

	gui_set_title(filename);

	modified = FALSE;
	return 1;
}

void gui_file_save_as()
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	int res;

	dialog = gtk_file_chooser_dialog_new("Save the file",
										 main_window.self,
										 action,
										 "_Save",
										 GTK_RESPONSE_ACCEPT,
										 "_Cancel",
										 GTK_RESPONSE_CANCEL,
										 NULL);

	chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	char* filename;
	if(res == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(
						GTK_FILE_CHOOSER(dialog));
		if(gui_file_save(filename))
		{
			strcpy(name_current_file, filename);
		}
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

void gui_file_save_check()
{
	if(NO_FILE_LOADED == 1)
	{
		gui_file_save_as();
	}
	else
	{
		gui_file_save(name_current_file);
	}
}

void gui_update_lncol()
{
	/*this function is bottleneck in efficiency.*/
	static char text[64] = "Ln: ";
	uint32_t row;
	uint32_t col;
	/*I hate this crutches GTK functions*/
	GtkTextIter iter;

	gtk_text_buffer_get_iter_at_mark(gui_text.buffer, &iter,
							gtk_text_buffer_get_insert(gui_text.buffer));
	row = gtk_text_iter_get_line(&iter);
	col = gtk_text_iter_get_line_offset(&iter);
	
	char buf[17];
	u64to_str(row, buf);
	size_t row_size = strlen(buf);
	memcpy(text + 4, buf, row_size);
	static char cpstr[6] = " Col: ";
	uint64_t* cpptr = (uint64_t*)&cpstr[0];
	uint64_t* cpptr2 = (uint64_t*)&text[4 + row_size];
	*cpptr2 = *cpptr;
	u64to_str(col, text + 10 + row_size);
	/*result look like format string ("Ln: %d Col: %d", row, col)*/

	gtk_label_set_text(main_window.lncol, text);
}

char gui_file_open(char* filename)
{
	FILE* fptr = fopen(filename, "r");
	if(fptr == NULL)
	{
		gui_set_title(filename);
		modified = FALSE;
		return 0;
	}
	char* buf;
	fseek(fptr, 0, SEEK_END);
	long fsize = ftell(fptr);
	rewind(fptr);

	buf = malloc(fsize + 1);
	fread(buf, fsize, 1, fptr);
	buf[fsize] = '\0';

	gtk_text_buffer_set_text(gui_text.buffer, buf, -1);

	free(buf);
	fclose(fptr);

	gui_set_title(filename);

	modified = FALSE;

	return 1;
}

void gui_show_menu_help_about()
{
	GtkWidget *dialog = gtk_about_dialog_new();
	GtkAboutDialog *dlg = GTK_ABOUT_DIALOG(dialog);
	gtk_about_dialog_set_name(dlg, "Text editor");
	gtk_about_dialog_set_version(dlg, "0.1");
	gtk_about_dialog_set_copyright(dlg, "(c) Roman Ermakov");
	gtk_about_dialog_set_comments(dlg, "Text editor");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

int gui_ask_save_cancel()
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(main_window.self,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				"File has been modified. Do you want to save it?");
	gtk_dialog_add_button(GTK_DIALOG(dialog),
						  "Cancel", GTK_RESPONSE_CANCEL);
	gtk_window_set_title(GTK_WINDOW(dialog), "File has been modified");
	
	int response = gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);

	return response;
}

void gui_text_changed()
{
	modified = TRUE;
	gui_update_lncol();
}

void gui_text_cut()
{
	gtk_text_buffer_cut_clipboard(gui_text.buffer, clipboard, TRUE);
}

void gui_text_copy()
{
	gtk_text_buffer_copy_clipboard(gui_text.buffer, clipboard);
}

void gui_text_paste()
{
	gtk_text_buffer_paste_clipboard(gui_text.buffer, clipboard, NULL, TRUE);
}

void gui_text_delete()
{
	gtk_text_buffer_delete_selection(gui_text.buffer, TRUE, TRUE);
}

void gui_text_select_all()
{
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(gui_text.buffer, &start);
	gtk_text_buffer_get_end_iter(gui_text.buffer, &end);

	gtk_text_buffer_select_range(gui_text.buffer, &start, &end);
}

void gui_text_switch_wrapping()
{
	GtkWrapMode mode;
	if (wrapping == 1)
	{
		mode = GTK_WRAP_CHAR;
	}
	else
	{
		mode = GTK_WRAP_NONE;
	}

	wrapping ^= 1;

	gtk_text_view_set_wrap_mode(gui_text.area, mode);
}

void gui_select_font()
{
	GtkResponseType result;

	GtkWidget *dialog = gtk_font_selection_dialog_new("Select a font");

	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_APPLY) {
		PangoFontDescription *font_desc;
		char *fontname = gtk_font_selection_dialog_get_font_name(
			GTK_FONT_SELECTION_DIALOG(dialog));

		font_desc = pango_font_description_from_string(fontname);

		gtk_widget_modify_font(GTK_WIDGET(gui_text.area), font_desc);

		g_free(fontname);
	}

	gtk_widget_destroy(dialog);
}

/*create a new file*/
void gui_file_new()
{
	name_current_file[0] = '\0';

	gtk_text_buffer_set_text(gui_text.buffer, "", -1);

	gtk_window_set_title(main_window.self, "Untitled");
}

/* Open an existing file */
void gui_ask_file_open()
{
	if (modified == TRUE)
	{
		int response = gui_ask_save_cancel();
		switch (response)
		{
			case GTK_RESPONSE_YES:
				gui_file_save_check();
				break;
			case GTK_RESPONSE_NO:
				break;
			case GTK_RESPONSE_DELETE_EVENT: case GTK_RESPONSE_CANCEL:
				return;
				break;

			default:
				return;
				break;
		}
	}

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	int res;

	dialog = gtk_file_chooser_dialog_new("Open a file",
										 main_window.self,
										 action,
										 "_Open",
										 GTK_RESPONSE_ACCEPT,
										 "_Cancel",
										 GTK_RESPONSE_CANCEL,
										 NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	char* filename;
	if (res == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(
						GTK_FILE_CHOOSER(dialog));
		
		gui_file_open(filename);
		strcpy(name_current_file, filename);

		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

void setup_menubar()
{
	gui_menu.menubar = (GtkMenuShell*)gtk_menu_bar_new();

	gui_menu.filemenu = (GtkMenuShell*)gtk_menu_new();
	gui_menu.file.self = (GtkMenuItem*)gtk_menu_item_new_with_mnemonic("_File");
	gui_menu.file.new  = gtk_menu_item_new_with_mnemonic("_New");
	gui_menu.file.open = gtk_menu_item_new_with_mnemonic("_Open");
	gui_menu.file.save = gtk_menu_item_new_with_mnemonic("_Save");
	gui_menu.file.saveas = gtk_menu_item_new_with_mnemonic("Save _as...");
	gui_menu.file.quit = gtk_menu_item_new_with_mnemonic("_Quit");

	gui_menu.editmenu = (GtkMenuShell*)gtk_menu_new();
	gui_menu.edit.self = (GtkMenuItem*)gtk_menu_item_new_with_mnemonic("_Edit");
	gui_menu.edit.cut  = gtk_menu_item_new_with_mnemonic("Cu_t");
	gui_menu.edit.copy = gtk_menu_item_new_with_mnemonic("_Copy");
	gui_menu.edit.paste = gtk_menu_item_new_with_mnemonic("_Paste");
	gui_menu.edit.delete = gtk_menu_item_new_with_mnemonic("_Delete");
	gui_menu.edit.selectall = gtk_menu_item_new_with_mnemonic("_Select All");
	gui_menu.edit.wrapping = gtk_check_menu_item_new_with_mnemonic("_Wrap long "
												"lines");
	gui_menu.edit.select_font = gtk_menu_item_new_with_mnemonic("_Font...");

	gui_menu.helpmenu = (GtkMenuShell*)gtk_menu_new();
	gui_menu.help.self = (GtkMenuItem*)gtk_menu_item_new_with_mnemonic("_Help");
	gui_menu.help.about = gtk_menu_item_new_with_mnemonic("_About");

	gtk_menu_item_set_submenu(gui_menu.file.self, (GtkWidget*)gui_menu.filemenu);
	gtk_menu_shell_append(gui_menu.filemenu, gui_menu.file.new);
	gtk_menu_shell_append(gui_menu.filemenu, gui_menu.file.open);
	gtk_menu_shell_append(gui_menu.filemenu, gui_menu.file.save);
	gtk_menu_shell_append(gui_menu.filemenu, gui_menu.file.saveas);
	gtk_menu_shell_append(gui_menu.filemenu,
							gtk_separator_menu_item_new());
	gtk_menu_shell_append(gui_menu.filemenu, gui_menu.file.quit);

	gtk_menu_item_set_submenu(gui_menu.edit.self, (GtkWidget*)gui_menu.editmenu);
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.cut);
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.copy);
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.paste);
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.delete);
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.selectall);
	gtk_menu_shell_append(gui_menu.editmenu,
							gtk_separator_menu_item_new());
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.wrapping);
	gtk_menu_shell_append(gui_menu.editmenu, gui_menu.edit.select_font);

	gtk_menu_item_set_submenu(gui_menu.help.self, (GtkWidget*)gui_menu.helpmenu);
	gtk_menu_shell_append(gui_menu.helpmenu, gui_menu.help.about);

	gtk_menu_shell_append(gui_menu.menubar, (GtkWidget*)gui_menu.file.self);
	gtk_menu_shell_append(gui_menu.menubar, (GtkWidget*)gui_menu.edit.self);
	gtk_menu_shell_append(gui_menu.menubar, (GtkWidget*)gui_menu.help.self);

	gtk_box_pack_start(GTK_BOX(main_window.vbox), GTK_WIDGET(gui_menu.menubar), FALSE, FALSE, 3);

	/*accelerators*/

	/*file menu*/
	gtk_widget_add_accelerator(gui_menu.file.new, "activate", main_window.accel, GDK_n,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(gui_menu.file.open, "activate", main_window.accel, GDK_o,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(gui_menu.file.save, "activate", main_window.accel, GDK_s,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(gui_menu.file.saveas, "activate", main_window.accel, GDK_s,
								GDK_CONTROL_MASK | GDK_SHIFT_MASK,
								GTK_ACCEL_VISIBLE);

	gtk_widget_add_accelerator(gui_menu.file.quit, "activate", main_window.accel, GDK_q,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);

	/*edit menu*/
	gtk_widget_add_accelerator(gui_menu.edit.cut, "activate", main_window.accel, GDK_x,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(gui_menu.edit.copy, "activate", main_window.accel, GDK_c,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(gui_menu.edit.paste, "activate", main_window.accel, GDK_v,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(gui_menu.edit.delete, "activate", main_window.accel, GDK_KEY_Delete,
								0,
								GTK_ACCEL_VISIBLE);

	gtk_widget_add_accelerator(gui_menu.edit.selectall, "activate", main_window.accel, GDK_a,
								GDK_CONTROL_MASK,
								GTK_ACCEL_VISIBLE);
	/*signals*/

	/*file menu*/
	g_signal_connect(gui_menu.file.new, "activate",
							(GCallback)gui_file_new, NULL);
	g_signal_connect(gui_menu.file.open, "activate",
							(GCallback)gui_ask_file_open, NULL);
	g_signal_connect(gui_menu.file.save, "activate",
							(GCallback)gui_file_save_check, NULL);
	g_signal_connect(gui_menu.file.saveas, "activate",
							(GCallback)gui_file_save_as, NULL);

	g_signal_connect(gui_menu.file.quit, "activate",
							(GCallback)gtk_main_quit, NULL);

	/*edit menu*/
	g_signal_connect(gui_menu.edit.cut, "activate",
							(GCallback)gui_text_cut, NULL);
	g_signal_connect(gui_menu.edit.copy, "activate",
							(GCallback)gui_text_copy, NULL);
	g_signal_connect(gui_menu.edit.paste, "activate",
							(GCallback)gui_text_paste, NULL);
	g_signal_connect(gui_menu.edit.delete, "activate",
							(GCallback)gui_text_delete, NULL);

	g_signal_connect(gui_menu.edit.selectall, "activate",
							(GCallback)gui_text_select_all, NULL);
	g_signal_connect(gui_menu.edit.wrapping, "activate",
							(GCallback)gui_text_switch_wrapping, NULL);
	g_signal_connect(gui_menu.edit.select_font, "activate",
							(GCallback)gui_select_font, NULL);

	/*help menu*/
	g_signal_connect(gui_menu.help.about, "activate",
							(GCallback)gui_show_menu_help_about, NULL);
}

void setup_textarea()
{
	gui_text.scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	gtk_scrolled_window_set_policy(gui_text.scroll,
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	gui_text.area = GTK_TEXT_VIEW(gtk_text_view_new());

	gtk_container_add(GTK_CONTAINER(gui_text.scroll), GTK_WIDGET(gui_text.area));

	gtk_box_pack_start(GTK_BOX(main_window.vbox),
					   GTK_WIDGET(gui_text.scroll),
					   TRUE, TRUE, 0);

	gui_text.buffer = gtk_text_view_get_buffer(gui_text.area);

	g_signal_connect(gui_text.buffer, "changed",
					 (GCallback) gui_text_changed, NULL);
	g_signal_connect(gui_text.buffer, "mark_set",
					 (GCallback) gui_update_lncol, NULL);

	gui_text_switch_wrapping();
}

void setup_lncol()
{
	main_window.lncol = GTK_LABEL(gtk_label_new(NULL));
	gtk_box_pack_start(GTK_BOX(main_window.vbox), GTK_WIDGET(main_window.lncol), FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(main_window.lncol), 0.0, 0.0);

	gui_update_lncol();
}

int main(int argc, char** argv)
{
	gtk_init(&argc, &argv);

	GdkAtom atom = gdk_atom_intern("CLIPBOARD", TRUE);
	clipboard = gtk_clipboard_get(atom);

	main_window.self = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

	gtk_window_set_title(main_window.self, title);
	gtk_window_set_default_size(main_window.self, 640, 480);
	gtk_window_set_position(main_window.self, GTK_WIN_POS_CENTER);

	main_window.vbox = GTK_VBOX(gtk_vbox_new(FALSE, 0));
	gtk_container_add(GTK_CONTAINER(main_window.self),
					  GTK_WIDGET(main_window.vbox));

	main_window.accel = gtk_accel_group_new();
	gtk_window_add_accel_group(main_window.self, main_window.accel);

	setup_menubar();
	setup_textarea();
	setup_lncol();

	if (argc == 2)
	{
		strcpy(name_current_file, argv[1]);
		gui_file_open(name_current_file);
	}

	g_signal_connect(main_window.self, "destroy",
					 (GCallback)gtk_main_quit, NULL);
	gtk_widget_show_all(GTK_WIDGET(main_window.self));
	gtk_main();

	return 0;
}
