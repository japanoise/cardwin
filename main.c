#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "data.h"

#define PROGNAME "cardwin"

#define ABOUT_TEXT                                                       \
	PROGNAME                                                         \
	" - cardfile clone\n"                                            \
	"Unleashed on an unsuspecting world by the Princess Japanoise\n" \
	"Based on code by Cam Menard\n"                                  \
	"\n"                                                             \
	"Copyright (C) 2023 Japanoise\n"                                 \
	"Copyright (C) 2001 Cam Menard"

#define FNAMESIZE PATH_MAX
#define TITLESIZE 300

#define SB_WIDTH 60
#define SB_HEIGHT 30

/* Color used to display on bits (white by default) */
#define ON_BIT_COLOR_R 0xFF
#define ON_BIT_COLOR_G 0xFF
#define ON_BIT_COLOR_B 0xFF

/* Color used to display off bits (black by default) */
#define OFF_BIT_COLOR_R 0x00
#define OFF_BIT_COLOR_G 0x00
#define OFF_BIT_COLOR_B 0x00

/* Boundary for an on bit when quantizing */
#define ON_BIT_BOUNDARY 0x80

/* ---=== Global state ===--- */
crd_cardfile *deck;
char filename[FNAMESIZE];
/* Used to store if the file has been edited: */
bool dirty = false;
/* Used to prevent dirty getting updated accidentally: */
bool data_loading = false;
/* Used if we need to sync the data to the widgets: */
bool sync_up = false;
gint selitem = 0;
GtkTextBuffer *card_data;
GdkPixbuf *image_data = NULL;

/* ---=== Global widgets ===--- */
GtkWidget *window;
GtkWidget *card_header;
GtkWidget *listbox;
GtkWidget *card_data_view;
GtkWidget *index_entry;
GtkWidget *image;

/* ---=== Function declarations ===--- */

/* Data functionality */
bool fexist(char *path);
void data_load();
void data_save();
void data_sync_up();
void list_select(GtkWidget *widget, guint ndata);

/* Dialog boxes */
gboolean yes_no_dialog(char *message);
void generic_dialog(char *message, GtkMessageType type);
void message_dialog(char *message);
void error_dialog(char *message);
char *file_dialog(char *title, GtkFileChooserAction action);
gboolean quit_confirm();

/* Button callbacks */
void close_dialog(GtkWidget *widget, gpointer data);
void close_show_message(GtkWidget *widget, gpointer *data);
void close_and_add(GtkWidget *widget, gpointer *data);
void click_quit(GtkWidget *widget, gpointer *data);
void click_prev(GtkWidget *widget, gpointer data);
void click_next(GtkWidget *widget, gpointer data);
void click_add(GtkWidget *widget, gpointer data);
void click_delete(GtkWidget *widget, gpointer data);

/* Menu callbacks */
void menu_save_as(gpointer data, guint action, GtkWidget *widget);
void menu_save(gpointer data, guint action, GtkWidget *widget);
void menu_about(gpointer data, guint action, GtkWidget *widget);
void menu_new(gpointer data, guint action, GtkWidget *widget);
void menu_open(gpointer data, guint action, GtkWidget *widget);

/* GUI utility */
GtkWidget *pack_new_button(GtkWidget *box, char *szlabel, int right);
void update_title();
void modified(GtkWidget *widget, gpointer *data);
gboolean window_delete_callback(GtkWidget *widget, gpointer *data);

/* ---=== Implementation ===--- */

/* ---=== Data functionality ===--- */
bool fexist(char *path)
{
	return access(path, F_OK) == 0;
}

void load_bitmap(crd_card *card)
{
	uint16_t width, height, xcoord, ycoord, datalen;
	uint8_t *datastart;

	if (!crd_card_parse_bmp(card, &width, &height, &xcoord, &ycoord,
				&datalen, &datastart)) {
		gtk_widget_hide(image);
		return;
	}

	int rowbytes = width / 8;
	/* XXX: integer version of ceiling */
	if (width % 8) {
		rowbytes++;
	}

	/* XXX: I've seen malformed bmps with extra data - find the
	 * start with a heuristic. */
	if (datalen >= (width * height)) {
		datalen = (rowbytes + 1) * height;
		int j = datalen;
		/* 0 is black which will rarely be used as a
		 * background, unless erroneously used. */
		while (j != rowbytes * height && datastart[j] == 0) {
			j--;
		}
		if (j == rowbytes * height) {
			datalen = j;
		}
	}

	/* XXX: Sometimes there's a junk byte after each row */
	if (rowbytes * height != datalen) {
		rowbytes++;
	}

	image_data =
		gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, width, height);
	guchar *pixels = gdk_pixbuf_get_pixels(image_data);
	int rowstride = gdk_pixbuf_get_rowstride(image_data);
	int n_channels = gdk_pixbuf_get_n_channels(image_data);

	/* image bits aligned to 8 memory bits */
	for (int y = 0; y < height; y++) {
		int destx = 0;
		for (int x = 0; x < rowbytes; x++) {
			uint8_t byte = datastart[(y * rowbytes) + x];
			for (int i = 0; i < 8; i++) {
				if (destx < width) {
					guchar *p = pixels + (y * rowstride) +
						    (destx * n_channels);

					if ((byte << i) & 0x80) {
						p[0] = ON_BIT_COLOR_R;
						p[1] = ON_BIT_COLOR_G;
						p[2] = ON_BIT_COLOR_B;
					} else {
						p[0] = OFF_BIT_COLOR_R;
						p[1] = OFF_BIT_COLOR_G;
						p[2] = OFF_BIT_COLOR_B;
					}
				}
				destx++;
			}
		}
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(image), image_data);
	gtk_widget_show(image);
}

void data_load()
{
	data_loading = true;
	gtk_list_clear_items(GTK_LIST(listbox), 0, -1);
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(card_data, &start);
	gtk_text_buffer_get_end_iter(card_data, &end);
	gtk_text_buffer_delete(card_data, &start, &end);

	if (deck->ncards > 0) {
		gtk_text_buffer_set_text(card_data, deck->cards[selitem]->data,
					 deck->cards[selitem]->datalen);

		gtk_entry_set_text(GTK_ENTRY(card_header),
				   deck->cards[selitem]->title);

		load_bitmap(deck->cards[selitem]);

		for (int i = 0; i < deck->ncards; i++) {
			GtkWidget *item = gtk_list_item_new_with_label(
				deck->cards[i]->title);
			gtk_container_add(GTK_CONTAINER(listbox), item);
			gtk_widget_show(item);
		}
	} else {
		gtk_text_buffer_set_text(card_data, "", 0);

		gtk_entry_set_text(GTK_ENTRY(card_header), "");

		gtk_widget_hide(image);
	}
	gtk_text_view_set_editable(GTK_TEXT_VIEW(card_data_view),
				   deck->ncards > 0);
	data_loading = false;
}

void data_save()
{
	data_loading = true;
	if (sync_up) {
		data_sync_up();
	}
	if (!crd_cardfile_save(deck, filename)) {
		perror(PROGNAME);
	}
	gtk_text_buffer_set_modified(card_data, false);
	data_loading = false;
	dirty = false;
	update_title();
}

void data_sync_up()
{
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(card_data, &start);
	gtk_text_buffer_get_end_iter(card_data, &end);
	gchar *entry = gtk_text_buffer_get_text(card_data, &start, &end, false);
	crd_card_set_data(deck->cards[selitem], entry);
	g_free(entry);
	sync_up = false;
}

void list_select(GtkWidget *widget, guint ndata)
{
	GList *node = GTK_LIST(widget)->selection;
	if (!node) {
		return;
	}

	/* Sync up the deck if necessary */
	if (sync_up) {
		data_sync_up();
	}

	GtkWidget *list_item_data = GTK_WIDGET(node->data);
	selitem = gtk_list_child_position(GTK_LIST(widget), list_item_data);
	data_loading = true;
	gtk_text_buffer_set_text(card_data, deck->cards[selitem]->data,
				 deck->cards[selitem]->datalen);
	gtk_entry_set_text(GTK_ENTRY(card_header), deck->cards[selitem]->title);
	load_bitmap(deck->cards[selitem]);
	data_loading = false;
}

/* ---=== Dialog boxes ===--- */
gboolean yes_no_dialog(char *message)
{
	GtkWidget *diag = gtk_message_dialog_new(
		GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO, "%s", message);
	gint response = gtk_dialog_run(GTK_DIALOG(diag));
	gtk_widget_destroy(diag);

	return (response == GTK_RESPONSE_YES);
}

void generic_dialog(char *message, GtkMessageType type)
{
	GtkWidget *diag = gtk_message_dialog_new(GTK_WINDOW(window),
						 GTK_DIALOG_MODAL, type,
						 GTK_BUTTONS_OK, "%s", message);
	gtk_dialog_run(GTK_DIALOG(diag));
	gtk_widget_destroy(diag);
}

void message_dialog(char *message)
{
	generic_dialog(message, GTK_MESSAGE_QUESTION);
}

void error_dialog(char *message)
{
	generic_dialog(message, GTK_MESSAGE_ERROR);
}

/* Returns a relative path that needs to be g_free'd */
char *file_dialog(char *title, GtkFileChooserAction action)
{
	GtkWidget *diag = gtk_file_chooser_dialog_new(
		title, GTK_WINDOW(window), action,
		(action == GTK_FILE_CHOOSER_ACTION_OPEN ? "Open" : "Save"),
		GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(diag), false);

	if (gtk_dialog_run(GTK_DIALOG(diag)) != GTK_RESPONSE_OK) {
		gtk_widget_destroy(diag);
		return NULL;
	}

	GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(diag));
	gtk_widget_destroy(diag);

	if (file == NULL) {
		/* Shouldn't happen, but can't be too careful. */
		/* Use a unique (bad) error message */
		error_dialog("File is null: cannot be loaded");
		return NULL;
	}

	char *retval = g_file_get_path(file);

	g_object_unref(file);

	return retval;
}

/* Returns whether or not the application should keep running */
gboolean quit_confirm()
{
	if (dirty) {
		gboolean doQuit = yes_no_dialog("File modified; really quit?");

		if (!doQuit) {
			return true;
		}
	}
	crd_cardfile_destroy(deck);
	return false;
}

/* ---=== Button callbacks ===--- */
void close_dialog(GtkWidget *widget, gpointer data)
{
	gtk_grab_remove(GTK_WIDGET(widget));
}

void close_show_message(GtkWidget *widget, gpointer *data)
{
	GtkWidget *dialog_widget = (GtkWidget *)data;
	gtk_widget_destroy(dialog_widget);
}

void close_and_add(GtkWidget *widget, gpointer *data)
{
	gint y;
	gchar *buffnew;
	GtkWidget *dialog_widget = (GtkWidget *)data;
	gchar *entry =
		gtk_editable_get_chars((GtkEditable *)index_entry, 0, 40);
	gtk_widget_destroy(dialog_widget);

	crd_card *card = crd_cardfile_add_new_card(deck, entry);
	int selitem = 0;
	dirty = true;
	for (int i = 0; i < deck->ncards; i++) {
		/* This should find the card; if not, 0 is a good
		 * default */
		if (deck->cards[i] == card) {
			selitem = i;
			break;
		}
	}
	data_load();
	gtk_list_select_item(GTK_LIST(listbox), selitem);
	update_title();

	g_free(entry);
}

void click_quit(GtkWidget *widget, gpointer *data)
{
	gboolean stay_running = quit_confirm();
	if (!stay_running) {
		gtk_widget_destroy(GTK_WIDGET(window));
		gtk_main_quit();
	}
}

void click_prev(GtkWidget *widget, gpointer data)
{
	if (!deck->ncards) {
		return;
	}

	gint c1 = selitem - 1;
	if (c1 <= -1) {
		c1 = deck->ncards - 1; // circle to last item
	}
	gtk_list_select_item(GTK_LIST(listbox), c1);
}

void click_next(GtkWidget *widget, gpointer data)
{
	if (!deck->ncards) {
		return;
	}
	gint c1 = selitem + 1;
	if (c1 >= deck->ncards) {
		c1 = 0; // circle to first item
	}
	gtk_list_select_item(GTK_LIST(listbox), c1);
}

void click_add(GtkWidget *widget, gpointer data)
{
	// requires a textbox for 40-character input 'index' field.
	// then call the edit card function to edit card_data blank text field.
	GtkWidget *label;
	GtkWidget *butOK, *butCancel;
	GtkWidget *d_window;
	d_window = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(d_window), "destroy",
			   GTK_SIGNAL_FUNC(close_dialog), NULL);
	// window and other containers
	gtk_widget_set_usize(d_window, 300, 100);
	gtk_window_set_transient_for(GTK_WINDOW(d_window), GTK_WINDOW(window));
	gtk_window_set_position(GTK_WINDOW(d_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(d_window), "Add Card Entry");
	gtk_container_set_border_width(GTK_CONTAINER(d_window), 0);
	index_entry = gtk_entry_new_with_max_length(40);
	gtk_entry_set_text(GTK_ENTRY(index_entry), "");
	label = gtk_label_new("Index Text");
	gtk_misc_set_padding(GTK_MISC(label), 10, 10);
	// buttons, connections and packing:
	butOK = gtk_button_new_with_label("OK");
	butCancel = gtk_button_new_with_label("Cancel");
	gtk_widget_set_usize(butOK, SB_WIDTH, SB_HEIGHT);
	gtk_widget_set_usize(butCancel, SB_WIDTH, SB_HEIGHT);
	gtk_signal_connect(GTK_OBJECT(butOK), "clicked",
			   GTK_SIGNAL_FUNC(close_and_add), d_window);
	gtk_signal_connect(GTK_OBJECT(butCancel), "clicked",
			   GTK_SIGNAL_FUNC(close_show_message), d_window);
	gtk_signal_connect(GTK_OBJECT(index_entry), "activate",
			   GTK_SIGNAL_FUNC(close_and_add), d_window);
	GTK_WIDGET_SET_FLAGS(butOK, GTK_CAN_DEFAULT);
	// vbox is top, action_area is bottom of the dialog window
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(d_window)->vbox), label, TRUE,
			   TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(d_window)->vbox), index_entry,
			   TRUE, TRUE, 0);
	// put buttons below label and input - FALSE to fill areas
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(d_window)->action_area), butOK,
			   TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(d_window)->action_area),
			   butCancel, TRUE, FALSE, 0);

	gtk_widget_grab_default(butOK);
	gtk_widget_show(butCancel);
	gtk_widget_show(butOK);
	gtk_widget_show(index_entry);
	gtk_widget_show(label);
	gtk_widget_show(d_window);
	gtk_grab_add(d_window);
	gtk_widget_grab_focus(GTK_WIDGET(index_entry));
}

void click_delete(GtkWidget *widget, gpointer data)
{
	crd_cardfile_delete_card(deck, selitem);
	if (selitem >= deck->ncards) {
		if (deck->ncards == 0) {
			selitem = 0;
		} else {
			selitem--;
		}
	}
	dirty = true;
	data_load();
	update_title();
}

/* ---=== Menu callbacks ===--- */
void menu_save_as(gpointer data, guint action, GtkWidget *widget)
{
	char *fname =
		file_dialog("Save Cardfile As", GTK_FILE_CHOOSER_ACTION_SAVE);

	if (fname == NULL) {
		return;
	}

	strncpy(filename, fname, FNAMESIZE);
	g_free(fname);

	data_save();
}

void menu_save(gpointer data, guint action, GtkWidget *widget)
{
	if (filename[0] == 0) {
		menu_save_as(data, action, widget);
		return;
	}

	data_save();
}

void menu_about(gpointer data, guint action, GtkWidget *widget)
{
	message_dialog(ABOUT_TEXT);
}

void menu_new(gpointer data, guint action, GtkWidget *widget)
{
	if (dirty && !yes_no_dialog("You have unsaved changes, "
				    "really create a new cardfile?")) {
		return;
	}

	filename[0] = 0;
	selitem = 0;
	crd_cardfile_clear(deck);

	data_load();

	update_title();
}

void menu_open(gpointer data, guint action, GtkWidget *widget)
{
	if (dirty &&
	    !yes_no_dialog(
		    "You have unsaved changes, really open a new file?")) {
		return;
	}

	char *fname =
		file_dialog("Open Cardfile", GTK_FILE_CHOOSER_ACTION_OPEN);

	if (fname == NULL) {
		return;
	}

	strncpy(filename, fname, FNAMESIZE);
	g_free(fname);

	if (!fexist(filename)) {
		error_dialog("File does not exist");
		return;
	}

	crd_cardfile_destroy(deck);
	deck = crd_cardfile_new();

	if (!crd_cardfile_load(deck, filename)) {
		error_dialog("Error loading file");
	}
	selitem = 0;
	data_load();

	update_title();
	if (deck->ncards > 0) {
		gtk_list_select_item(GTK_LIST(listbox), selitem);
	}
}

void menu_image_export(gpointer data, guint action, GtkWidget *widget)
{
	if (deck->ncards == 0 || !deck->cards[selitem]->bmpsize) {
		return;
	}

	char *fname =
		file_dialog("Export Image As", GTK_FILE_CHOOSER_ACTION_SAVE);

	if (fname == NULL) {
		return;
	}

	GError *err = NULL;
	char *fmt = "png"; /* Seems a sensible default */
	char png[] = "png";
	char jpeg[] = "jpeg";
	char ico[] = "ico";
	char bmp[] = "bmp";
	int len = strlen(fname);

	char *ext = fname + len - 4;

	/* I know, I know this is stupid. Patches welcome for a better
	 * version. */
	if (strcmp(ext, ".bmp") == 0 || strcmp(ext, ".BMP") == 0) {
		fmt = bmp;
	} else if (strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0) {
		fmt = png;
	} else if (strcmp(ext, ".ico") == 0 || strcmp(ext, ".ICO") == 0) {
		fmt = ico;
	} else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".JPG") == 0 ||
		   strcmp(ext, "jpeg") == 0 || strcmp(ext, "JPEG") == 0) {
		fmt = jpeg;
	}

	if (!gdk_pixbuf_save(image_data, fname, fmt, &err, NULL)) {
		error_dialog(err->message);
		g_error_free(err);
	}

	g_free(fname);
}

void menu_image_import(gpointer data, guint action, GtkWidget *widget)
{
	if (deck->ncards == 0) {
		return;
	}

	if (deck->cards[selitem]->bmpsize && !yes_no_dialog(
		    "This card already has an image, are you sure you"
		    " want to import a new one?"
		    " The old one will be overwritten.")) {
		return;
	}

	char *fname =
		file_dialog("Import image", GTK_FILE_CHOOSER_ACTION_OPEN);

	GError *err = NULL;
	GdkPixbuf* buf = gdk_pixbuf_new_from_file(fname, &err);
	g_free(fname);

	if (buf == NULL) {
		error_dialog(err->message);
		g_error_free(err);
		return;
	}

	uint8_t *pixels = gdk_pixbuf_get_pixels(buf);
	int rowstride = gdk_pixbuf_get_rowstride(buf);
	int n_channels = gdk_pixbuf_get_n_channels(buf);
	int width = gdk_pixbuf_get_width(buf);
	int height = gdk_pixbuf_get_height(buf);
	int rowbytes = width / 8;
	if (width % 8) {
		rowbytes++;
	}

	if (width > 0xFFFF) {
		error_dialog(
			"Image is too wide - must be at most 65535 pixels.");
		g_free(buf);
		return;
	}

	if (height > 0xFFFF) {
		error_dialog(
			"Image is too tall - must be at most 65535 pixels.");
		g_free(buf);
		return;
	}

	uint8_t *datastart = crd_card_new_bmp(
		deck->cards[selitem], width, height, rowbytes);

	if (datastart == NULL) {
		error_dialog("Image data would be too large");
		g_free(buf);
		return;
	}

#ifdef QUANT_DUMB
	for (int y = 0; y < height; y++) {
		int destbyte = y*rowbytes;
		int destbit = 0;
		for (int x = 0; x < width; x++) {
			guchar *p = pixels + (y * rowstride) + (x * n_channels);

			if (p[0] >= ON_BIT_BOUNDARY ||
			    p[1] >= ON_BIT_BOUNDARY ||
				p[2] >= ON_BIT_BOUNDARY) {
				datastart[destbyte] |= 1<<(7-destbit);
			}
			destbit++;
			if (destbit >= 8) {
				destbyte++;
				destbit = 0;
			}
		}
	}

	g_free(buf);
#else  /* Floyd-Steinberg dithering */
	int16_t *inter = calloc(width*height, sizeof(int16_t));

	/* Pass 1 - convert to grayscale */
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			guchar *p = pixels + (y * rowstride) + (x * n_channels);
			inter[(y*width)+x] = (p[0] + p[1] + p[3])/3;
		}
	}
	g_free(buf);

	/* Pass 2 - F-S on the grayscale buffer */
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int16_t oldpixel = inter[(y*width)+x];
			int16_t newpixel =
				oldpixel >= ON_BIT_BOUNDARY ? 0xFF : 0;
			inter[(y*width)+x] = newpixel;
			int16_t quantError = oldpixel - newpixel;
			if (x < width-1) {
				inter[(y*width)+x+1] = inter[(y*width)+x+1] +
					((quantError*7)/16);
			}
			if (y < height-1) {
				int nextrow = ((y+1)*width)+x;

				if (0 < x) {
					inter[nextrow-1] = inter[nextrow-1] +
						((quantError*3)/16);
				}

				inter[nextrow] = inter[nextrow] +
					((quantError*5)/16);

				if (x < width-1) {
					inter[nextrow+1] = inter[nextrow+1] +
						(quantError/16);
				}
			}
		}
	}

	/* Pass 3 - copy into packed array */
	for (int y = 0; y < height; y++) {
		int destbyte = y*rowbytes;
		int destbit = 0;
		for (int x = 0; x < width; x++) {
			if (inter[(y*width)+x] == 0xFF) {
				datastart[destbyte] |= 1<<(7-destbit);
			}
			destbit++;
			if (destbit >= 8) {
				destbyte++;
				destbit = 0;
			}
		}
	}

	free(inter);
#endif

	load_bitmap(deck->cards[selitem]);
	dirty = true;
	update_title();
}

/* ---=== GUI utility ===--- */
GtkWidget *pack_new_button(GtkWidget *box, char *szlabel, int right)
{
	GtkWidget *button;
	button = gtk_button_new_with_label(szlabel);
	gtk_widget_set_usize(button, SB_WIDTH, SB_HEIGHT);
	if (right)
		gtk_box_pack_start(GTK_BOX(box), button, false, false, 0);
	else
		gtk_box_pack_end(GTK_BOX(box), button, false, false, 0);
	gtk_widget_show(button);
	return (button);
}

void update_title()
{
	char wtitle[TITLESIZE];
	char untitled[] = "(untitled)";

	g_snprintf(wtitle, TITLESIZE - 1, "%s -%c- " PROGNAME,
		   filename[0] ? filename : untitled, dirty ? '*' : '-');

	gtk_window_set_title(GTK_WINDOW(window), wtitle);
}

void modified(GtkWidget *widget, gpointer *data)
{
	/* This is to prevent it firing while we're loading data in. */
	if (data_loading) {
		return;
	}

	bool old_dirty = dirty;
	/* Always keep the buffer dirty if it already was */
	dirty = dirty || gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(widget));
	if (dirty != old_dirty) {
		update_title();
	}
	sync_up = dirty;
}

gboolean window_delete_callback(GtkWidget *widget, gpointer *data)
{
	gboolean stay_running = quit_confirm();
	if (!stay_running) {
		gtk_main_quit();
	}
	return stay_running;
}

/* ---=== Main function (entrypoint) ===--- */
int main(int argc, char *argv[])
{
	deck = crd_cardfile_new();

	/* Menubar */
	GtkWidget *menubar;
	GtkAccelGroup *accel;
	GtkItemFactory *ifac;
	GtkItemFactoryEntry items[] = {
		{ "/_File", NULL, NULL, 0, "<Branch>" },
		{ "/File/_New", "<Ctrl>n", menu_new, 1, "<Item>" },
		{ "/File/_Open...", "<Ctrl>o", menu_open, 0, "<Item>" },
		{ "/File/_Save", "<Ctrl>s", menu_save, 2, "<Item>" },
		{ "/File/Save _As...", NULL, menu_save_as, 3, "<Item>" },
		{ "/File/Separator", NULL, NULL, 0, "<Separator>" },
		{ "/File/E_xit", "<Ctrl>q", click_quit, 4, "<Item>" },
		/* This code breaks clipboard and does nothing useful. */
		/* I'll add it back in/implement it if someone complains. */
		/* { "/_Edit", NULL, NULL, 5, "<Branch>" }, */
		/* { "/Edit/Cut", "<Ctrl>x", men_move, 6, "<Item>" }, */
		/* { "/Edit/Copy", "<Ctrl>c", men_move, 7, "<Item>" }, */
		/* { "/Edit/Paste", "<Ctrl>v", men_move, 8, "<Item>" }, */
		{ "/_Record", NULL, NULL, 9, "<Branch>" },
		{ "/Record/_Previous", "<Alt>p", click_prev, 10, "<Item>" },
		{ "/Record/_Next", "<Alt>n", click_next, 11, "<Item>" },
		{ "/Record/_Add", NULL, click_add, 12, "<Item>" },
		{ "/Record/_Delete", NULL, click_delete, 13, "<Item>" },
		{ "/File/Separator", NULL, NULL, 0, "<Separator>" },
		{ "/Record/_Image", NULL, NULL, 13, "<Branch>" },
		{ "/Record/Image/_Export", NULL, menu_image_export, 13,
		  "<Item>" },
		{ "/Record/Image/_Import", NULL, menu_image_import, 13,
		  "<Item>" },
		{ "/_Help", NULL, NULL, 14, "<LastBranch>" },
		{ "/Help/_About...", NULL, menu_about, 15, "<Item>" }
	};

	gtk_init(&argc, &argv);

	/* Create window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize(window, 700, 500);
	/* Wire up window */
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(window_delete_callback), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_widget_realize(window);

	/* Create large gui items */
	listbox = gtk_list_new();
	card_header = gtk_entry_new();
	card_data = gtk_text_buffer_new(NULL);
	card_data_view = gtk_text_view_new_with_buffer(card_data);
	image = gtk_image_new();
	/* Wire up large gui items */
	gtk_entry_set_editable(GTK_ENTRY(card_header), false);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(card_data_view),
				    GTK_WRAP_WORD);
	gtk_list_set_selection_mode(GTK_LIST(listbox), GTK_SELECTION_SINGLE);

	/* Wire up menubar */
	accel = gtk_accel_group_new();
	ifac = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel);
	gtk_item_factory_create_items(ifac, sizeof(items) / sizeof(items[0]),
				      items, (gpointer)listbox);
	menubar = gtk_item_factory_get_widget(ifac, "<main>");

	/* Load in initial data, if applicable, and set filename */
	if (argc > 1) {
		strcpy(filename, argv[1]);
		crd_cardfile_load(deck, filename);
		data_load();
	} else {
		filename[0] = 0;
	}
	gtk_text_view_set_editable(GTK_TEXT_VIEW(card_data_view),
				   deck->ncards > 0);

	/* Now that we have a filename we can set the window title */
	update_title();

	/* Wire up watcher for modified state */
	gtk_text_buffer_set_modified(card_data, false);

	g_signal_connect(G_OBJECT(card_data), "changed",
			 GTK_SIGNAL_FUNC(modified), NULL);

	/* Create toolbar and buttons */
	GtkWidget *toolbar_box = gtk_hbox_new(false, 15);
	GtkWidget *button_prev = pack_new_button(toolbar_box, "Prev", true);
	GtkWidget *button_next = pack_new_button(toolbar_box, "Next", true);
	GtkWidget *button_add = pack_new_button(toolbar_box, "Add", true);
	GtkWidget *button_delete = pack_new_button(toolbar_box, "Delete", true);
	GtkWidget *button_quit = pack_new_button(toolbar_box, "Quit", false);

	/* Wire up buttons */
	gtk_signal_connect(GTK_OBJECT(button_prev), "clicked",
			   GTK_SIGNAL_FUNC(click_prev), NULL);
	gtk_signal_connect(GTK_OBJECT(button_next), "clicked",
			   GTK_SIGNAL_FUNC(click_next), NULL);
	gtk_signal_connect(GTK_OBJECT(button_add), "clicked",
			   GTK_SIGNAL_FUNC(click_add), NULL);
	gtk_signal_connect(GTK_OBJECT(button_delete), "clicked",
			   GTK_SIGNAL_FUNC(click_delete), NULL);
	gtk_signal_connect(GTK_OBJECT(button_quit), "clicked",
			   GTK_SIGNAL_FUNC(click_quit), NULL);

	/* Wire up listbox */
	gtk_signal_connect(GTK_OBJECT(listbox), "select_child",
			   GTK_SIGNAL_FUNC(list_select), NULL);
	gtk_list_select_item(GTK_LIST(listbox), selitem);

	/* Lay out GUI */
	/*  _____ toplevel container ___
	 * |  menubar                   |
	 * |  toolbar                   |
	 * | _____ split container ____ |
	 * |             |card header   |
	 * |             |              |
	 * |   scroll    |              |
	 * |  container  |card data view|
	 * |             |              |
	 * |             |              |
	 * |             |(todo) image) |
	 * |             |              |
	 * |             |              |
	 * | -------------------------- |
	 * |                            |
	 * `----------------------------'
	 */
	GtkWidget *toplevel_container = gtk_vbox_new(false, 0);
	GtkWidget *split_container = gtk_hpaned_new();
	GtkWidget *listbox_scroll_container =
		gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(listbox_scroll_container),
		GTK_POLICY_AUTOMATIC, false);
	GtkWidget *card_container = gtk_vbox_new(false, 0);

	GtkWidget *divider_1 = gtk_hseparator_new();
	GtkWidget *divider_2 = gtk_hseparator_new();
	GtkWidget *divider_3 = gtk_hseparator_new();

	gtk_paned_pack1(GTK_PANED(split_container), listbox_scroll_container,
			false, false);
	gtk_paned_pack2(GTK_PANED(split_container), card_container, true,
			false);
	gtk_widget_set_size_request(listbox_scroll_container, 300, -1);

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(listbox_scroll_container), listbox);

	gtk_box_pack_start(GTK_BOX(card_container), card_header, false, false,
			   0);
	gtk_box_pack_start(GTK_BOX(card_container), card_data_view, true, true,
			   0);
	gtk_box_pack_start(GTK_BOX(card_container), divider_3, false, false, 0);
	gtk_box_pack_start(GTK_BOX(card_container), image, false, false, 0);

	gtk_box_pack_start(GTK_BOX(toplevel_container), menubar, false, false,
			   0);
	gtk_box_pack_start(GTK_BOX(toplevel_container), toolbar_box, false,
			   false, 0);
	gtk_box_pack_start(GTK_BOX(toplevel_container), divider_1, false, true,
			   0);
	gtk_box_pack_start(GTK_BOX(toplevel_container), split_container, true,
			   true, 0);
	gtk_box_pack_start(GTK_BOX(toplevel_container), divider_2, false, true,
			   0);

	gtk_window_add_accel_group(GTK_WINDOW(window), accel);
	gtk_container_add(GTK_CONTAINER(window), toplevel_container);
	gtk_widget_show_all(window);
	gtk_widget_set_visible(image, deck->ncards > 0 &&
					      deck->cards[selitem]->bmpsize);

	/* Hand off to gtk */
	gtk_main();
	return 0;
}
