/* cardwin.c
	WIP: save and save as.
*/
#include <stdio.h>
#include <sys/types.h>		//27 aug.
#include <sys/stat.h>		//27 aug.
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#define RECLEN 52
#define SB_WIDTH 60
#define SB_HEIGHT 30

// globals:
struct stat s; // file structure
gint status;   // file status
gint flen, ccnt, indpos, datpos, f_op;
static gchar *fname;
gchar fl[160];
gchar wtitle[256];
GList *rdata = NULL;
GList *rindx = NULL;
gchar *ptr;
// GString *gstr;
GtkWidget *ndex, *window, *f_sel;
GtkWidget *cardata, *statdata, *htext;	 // htext header added 28/04/2001
GtkWidget *listbox;
gint count, altered, first;

//function templates:
void CloseDialog (GtkWidget *, gpointer);
void YesNo (char *, void (*Yesfunc), void (*Nofunc), int);
void CloseShowMessage (GtkWidget *, gpointer * );
void ShowMessage (char *, char * );
void exitYes ( GtkWidget *, gpointer * );
void exitNo ( GtkWidget *, gpointer * );
void callback ( GtkWidget *, gpointer );
void callnext ( GtkWidget *, gpointer );
void AddCard ( GtkWidget *, gpointer );
void CloseandAdd ( GtkWidget *, gpointer * );
void DelCard ( GtkWidget *, gpointer );
void CardOut ( GtkWidget *, gpointer );
void exitapp ( GtkWidget *, gpointer * );
void get_card_data (GtkWidget *, char * );
void parsebuf ( );
void l_select ( GtkWidget *, guint );
void wip ( gpointer, guint, GtkWidget * );
void f_openh ( GtkWidget * );
gboolean hide_win ( GtkWidget *, GdkEvent *, gpointer );
void file_back ( GtkWidget * );
void WriteCards ( GtkWidget *, gpointer * );
void men_move ( gpointer, guint, GtkWidget * );
void currstat ( );
void re_sort ( GtkWidget *, GList * );
gint eval_items ( gpointer, gpointer );
gchar *killcr ( gchar * );
gchar *addcr ( gchar * );
gchar *savbuff ( );

void CloseDialog (GtkWidget *widget, gpointer data)
{
  gtk_grab_remove (GTK_WIDGET(widget));
}

void YesNo (char *szMess, void (*Yesfunc), void (*Nofunc), int deflt)
{
  GtkWidget *label;
  GtkWidget *but1;
  GtkWidget *but2;
  GtkWidget *dialog_window;
  dialog_window = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT(dialog_window), "destroy", GTK_SIGNAL_FUNC(CloseDialog), &dialog_window);
  gtk_window_set_transient_for(GTK_WINDOW(dialog_window), GTK_WINDOW(window));
  gtk_window_set_title (GTK_WINDOW(dialog_window), "Yes/No Response");
  gtk_window_set_position (GTK_WINDOW(dialog_window), GTK_WIN_POS_CENTER);
  gtk_container_border_width (GTK_CONTAINER(dialog_window), 5);
  label = gtk_label_new (szMess);
  gtk_misc_set_padding (GTK_MISC(label), 10, 10);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog_window)->vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  but1 = gtk_button_new_with_label ("Yes");
  but2 = gtk_button_new_with_label ("No");
  gtk_widget_set_usize (but1, 100, 40);
  gtk_widget_set_usize (but2, 100, 40);
  gtk_signal_connect (GTK_OBJECT(but1), "clicked", GTK_SIGNAL_FUNC(Yesfunc), dialog_window);
  gtk_signal_connect (GTK_OBJECT(but2), "clicked", GTK_SIGNAL_FUNC(Nofunc), dialog_window);
  GTK_WIDGET_SET_FLAGS (but1, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (but2, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog_window)->action_area), but1, TRUE, TRUE, 0);
  gtk_widget_show (but1);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog_window)->action_area), but2, TRUE, TRUE, 0);
  if (deflt == 1)
    gtk_widget_grab_default (but1);
  else
    gtk_widget_grab_default (but2);

  gtk_widget_show (but2);
  gtk_widget_show (dialog_window);
  gtk_grab_add (dialog_window);
}


void CloseShowMessage (GtkWidget *widget, gpointer *data)
{
  GtkWidget *dialog_widget = (GtkWidget *) data;
  gtk_widget_destroy (dialog_widget);
}

// Show message:
void ShowMessage (char *szTitle, char *szMess)
{
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *dialog_window;
  dialog_window = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT(dialog_window), "destroy", GTK_SIGNAL_FUNC (CloseDialog), NULL);

  gtk_window_set_title (GTK_WINDOW(dialog_window), szTitle);
  gtk_container_border_width (GTK_CONTAINER(dialog_window), 0);
  button = gtk_button_new_with_label("OK");
  gtk_window_set_transient_for(GTK_WINDOW(dialog_window), GTK_WINDOW(window));
  gtk_window_set_position (GTK_WINDOW(dialog_window), GTK_WIN_POS_CENTER);
  gtk_signal_connect (GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC (CloseShowMessage), dialog_window);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  label = gtk_label_new (szMess);
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog_window)->vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  gtk_widget_show (dialog_window);
  gtk_grab_add (dialog_window);
}

/* Button callback function(s) */
void exitYes( GtkWidget *widget, gpointer *data )
{
  gtk_widget_destroy (GTK_WIDGET(data));
  g_free (ptr);
  gtk_main_quit ();
}

void exitNo( GtkWidget *widget, gpointer *data )
{
  gtk_widget_destroy (GTK_WIDGET(data));
}

void AddCard( GtkWidget *widget, gpointer data ) // data is *listbox
{
  // requires a textbox for 40-character input 'index' field.
  // then call the edit card function to edit cardata blank text field.
  GtkWidget *label;
  GtkWidget *butOK, *butCancel;
  GtkWidget *d_window;
  d_window = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT(d_window), "destroy", GTK_SIGNAL_FUNC (CloseDialog), NULL);
// window and other containers
  gtk_widget_set_usize (d_window, 300, 100);
  gtk_window_set_transient_for(GTK_WINDOW(d_window), GTK_WINDOW(window));
  gtk_window_set_position (GTK_WINDOW(d_window), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW(d_window), "- Add Card Entry -");
  gtk_container_set_border_width (GTK_CONTAINER(d_window), 0);
  ndex = gtk_entry_new_with_max_length (40);
  gtk_entry_set_text(GTK_ENTRY(ndex), "");
  label = gtk_label_new ("Index Text");
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
// buttons, connections and packing:
  butOK = gtk_button_new_with_label("OK");
  butCancel = gtk_button_new_with_label("Cancel");
  gtk_widget_set_usize (butOK, SB_WIDTH, SB_HEIGHT);
  gtk_widget_set_usize (butCancel, SB_WIDTH, SB_HEIGHT);
  gtk_signal_connect (GTK_OBJECT(butOK), "clicked", GTK_SIGNAL_FUNC(CloseandAdd), d_window);
  gtk_signal_connect (GTK_OBJECT(butCancel), "clicked", GTK_SIGNAL_FUNC(CloseShowMessage), d_window);
  gtk_signal_connect (GTK_OBJECT(ndex), "activate", GTK_SIGNAL_FUNC(CloseandAdd), d_window);
  GTK_WIDGET_SET_FLAGS (butOK, GTK_CAN_DEFAULT);
// vbox is top, action_area is bottom of the dialog window
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(d_window)->vbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(d_window)->vbox), ndex, TRUE, TRUE, 0);
// put buttons below label and input - FALSE to fill areas
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(d_window)->action_area), butOK, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(d_window)->action_area), butCancel, TRUE, FALSE, 0);

  gtk_widget_grab_default (butOK);
  gtk_widget_show (butCancel);
  gtk_widget_show (butOK);
  gtk_widget_show (ndex);
  gtk_widget_show (label);
  gtk_widget_show (d_window);
  gtk_grab_add (d_window);
  gtk_widget_grab_focus(GTK_WIDGET(ndex));
}

void CloseandAdd (GtkWidget *widget, gpointer *data)
{
  // identify: multiple entries ??
  gint y;
  gchar *buffnew;
  GtkWidget *dialog_widget = (GtkWidget *) data;
  gchar *entry = gtk_editable_get_chars((GtkEditable *) ndex, 0, 40);
  gtk_widget_destroy (dialog_widget);
  first = TRUE;
  printf ("%ld: %s\n\n", strlen(entry), entry);
  buffnew = g_strdup_printf("%s", g_strstrip(entry));
  if(strlen(entry) != 0) {
	 rindx = g_list_insert_sorted (rindx, (gchar *)buffnew, (GCompareFunc) eval_items);
	 y = g_list_index (rindx, buffnew);
	 rdata = g_list_insert (rdata, "", y);
	 count = y;
	 ccnt ++;
	 re_sort (listbox, rindx);
	 altered = TRUE;
	 gtk_list_select_item (GTK_LIST(listbox), count);
	 gtk_widget_grab_focus (GTK_WIDGET(cardata));
  }
  g_free(entry);
}
void callback ( GtkWidget *widget, gpointer data )
{
  guint c1 = count - 1;
  if (c1 == -1)
    c1 = ccnt - 1;	// circle to last item
  gtk_list_select_item (GTK_LIST(data), c1);
}

void callnext( GtkWidget *widget, gpointer data )
{
  guint c1 = count + 1;
  if (c1 == ccnt) {
	 c1 = 0;		// circle to first item
  }
  gtk_list_select_item (GTK_LIST(data), c1);
}

void DelCard ( GtkWidget *widget, gpointer data )	// data: "Delete"
{
  gchar *buffnew;
  buffnew = g_strdup_printf("%s: Remove this card?\n%s", (gchar *) g_list_nth_data (rindx, count), (gchar *) g_list_nth_data(rdata, count));
  YesNo (buffnew, CardOut, exitNo, 0);
}

void CardOut ( GtkWidget *widget, gpointer data )
{
  GList *newndx = NULL;
  GList *newdat = NULL;
  gchar *s1, *s2;
  gint x;
  gint y = count;
  gtk_list_unselect_all (GTK_LIST(listbox)); // unselect
  first = TRUE;  // allow no change when l_select is called
  for ( x = 0 ; x < ccnt ; x++ ) {
	 if (x != y)
		{
		  s1 = (gchar *) g_list_nth_data (rindx, x);
		  newndx = g_list_append(newndx, s1);
		  s2 = (gchar *) g_list_nth_data (rdata, x);
		  newdat = g_list_append(newdat, s2);
		}
  }
  ccnt --;
  g_list_free (rindx);
  g_list_free (rdata);
  rindx = g_list_copy (newndx);
  rdata = g_list_copy (newdat);
  g_list_free (newndx);
  g_list_free (newdat);
  altered = TRUE;
  re_sort (listbox, rindx);
  count = (y < ccnt-1) ? y : ccnt - 1;
  gtk_list_select_item (GTK_LIST(listbox), count);
  gtk_widget_destroy (GTK_WIDGET(data));
}

/* menu callbacks */
void men_move ( gpointer data, guint action, GtkWidget *widget )
{
  // menu wrapper functions: open/new/save/save as
  switch (action) {
  case 0:
	 f_op = 0;
	 gtk_file_selection_set_filename(GTK_FILE_SELECTION(f_sel), "./*.crd");
	 f_openh(f_sel);
	 break;
  case 1:  // new
	 g_free(ptr);

	 g_snprintf (fl, 160, "%s", "");
	 ptr = g_strnfill (5, 0);
	 ptr[0] = 'M';
	 ptr[1] = 'G';
	 ptr[2] = 'C';
	 ptr[3] = ptr[4] = 0;
	 parsebuf ();  // create listbox data from GList
	 gtk_editable_delete_text (GTK_EDITABLE(cardata), 0, -1);
	 gtk_editable_delete_text (GTK_EDITABLE(htext), 0, -1);
	 break;
  case 2:   // save
	 re_sort (listbox, rindx);  // just a test
	 //	 ShowMessage ("WIP:", "- File will be saved -\nWork In Progress.");
	 ptr = savbuff();
	 YesNo ("Overwrite current file?", WriteCards, exitNo, 0);
	 //	 re_sort (listbox, rindx);
	 altered = FALSE;
	 break;
  case 3:  // save as
	 f_op = 2;
	 gtk_file_selection_set_filename(GTK_FILE_SELECTION(f_sel), "./*.crd");
	 f_openh(f_sel);
	 //	 ShowMessage ("WIP:", "-Save As- is currently Work In Progress.");
	 ptr = savbuff();
	 break;
  case 4:  // exit
	 // if (altered), ask about saving file
	 YesNo ("Exit the Cardfile Viewer?", exitYes, exitNo, 0);
	 break;
  case 10:  // call previous
	 callback (widget, data);
	 break;
  case 11:  // call next
	 callnext (widget, data);
	 break;
  case 12:  // add
	 AddCard (widget, data);
	 break;
  case 13:  // delete
	 DelCard (widget, data);
	 break;
  case 15:  // about
	 ShowMessage ("Mine!", "Windows Cardfile Viewer for Linux:\nWorking Version Gtk 0.09\nby C. Menard.");
	 break;
  default:  // for any other calls
	 ShowMessage ("CLM Software", "Incomplete software area.");
	 break;
  }
}

void exitapp (GtkWidget *widget, gpointer *data)
{
  // if (altered), ask about saving file
  YesNo ("Exit the Cardfile Viewer?", exitYes, exitNo, 0);
}

void f_openh (GtkWidget *f_sel)
{
  gtk_widget_show(GTK_WIDGET(f_sel));
}

gboolean hide_win (GtkWidget *widget, GdkEvent *evnt, gpointer data)
{
  gtk_widget_hide(f_sel);
  return (TRUE); // don't destroy, just hide.
}

void file_back (GtkWidget *widget)
{
  gtk_widget_hide(f_sel);
  fname=gtk_file_selection_get_filename(GTK_FILE_SELECTION(f_sel));
  if (eval_items(fl, fname)) {
	 g_snprintf (fl, 160, "%s", (gchar *)fname);
  }
  switch (f_op) {
  case 0:    // File Open
	 get_card_data (listbox, fl);
	 first = TRUE;
	 altered = FALSE;
	 gtk_list_select_item (GTK_LIST(listbox), 0);
	 break;
  case 2:
	 YesNo ("Overwrite current file?", WriteCards, exitNo, 0);
	 //	 ShowMessage (fl, "- Save As is currently WIP -");
	 break;
  }
  return;
  // f_op: 0=open, 2=Save as
}

void WriteCards (GtkWidget *widget, gpointer *data)
{
  FILE *fp;
  gtk_widget_destroy(GTK_WIDGET(data));
  if ((fp = fopen (fl, "wb")) == NULL)
	 {
		ShowMessage ("I/O Error", "Cannot create file.");
		flen = 0;
		return;
	 }
  fwrite (ptr, datpos + 1, 1, fp); // set flen somewhere?
  fclose (fp);
}

GtkWidget *PackNewButton (GtkWidget *box, char *szlabel, int right)
{
   GtkWidget *button;
	button = gtk_button_new_with_label (szlabel);
	gtk_widget_set_usize (button, SB_WIDTH, SB_HEIGHT);
	if (right)
	  gtk_box_pack_start (GTK_BOX(box), button, FALSE, FALSE, 0);
	else
	  gtk_box_pack_end (GTK_BOX(box), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
	return (button);
}

GtkWidget *Toggle_Button (GtkWidget *box, char *szlabel, int right)
{
   GtkWidget *button;
	button = gtk_toggle_button_new_with_label (szlabel);
	gtk_widget_set_usize (button, SB_WIDTH + 10, SB_HEIGHT);
	if (right)
	  gtk_box_pack_start (GTK_BOX(box), button, FALSE, FALSE, 0);
	else
	  gtk_box_pack_end (GTK_BOX(box), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
	return (button);
}

int main (int argc, char *argv[])
{

	GtkWidget *butP, *butN, *butA, *butD, *butQ;
	GtkWidget *box1, *box2, *hbox;
	GtkWidget *vbox;
	GtkWidget *div1, *div2, *div3;
	GtkWidget *scrwin;

	GtkWidget *menubar;
	GtkAccelGroup *accel;
	GtkItemFactory *ifac;
//	ItemFactory for Menu Creation - Mar 2001
//	widget to pass is (gpointer) listbox
	GtkItemFactoryEntry items[] = { {"/_File", NULL, NULL, 0, "<Branch>"},
		{"/File/_Open...", "<Ctrl>o", men_move, 0, "<Item>"},
		{"/File/_New", "<Ctrl>n", men_move, 1, "<Item>"},
		{"/File/_Save", "<Ctrl>s", men_move, 2, "<Item>"},
		{"/File/Save _As...", NULL, men_move, 3, "<Item>"},
		{"/File/Separator", NULL, NULL, 0, "<Separator>"},
		{"/File/E_xit", "<Ctrl>q", men_move, 4, "<Item>"},
		{"/_Edit", NULL, NULL, 5, "<Branch>"},
		{"/Edit/Cut", "<Ctrl>x", men_move, 6, "<Item>"},
		{"/Edit/Copy", "<Ctrl>c", men_move, 7, "<Item>"},
		{"/Edit/Paste", "<Ctrl>v", men_move, 8, "<Item>"},
		{"/_Record", NULL, NULL, 9, "<Branch>"},
		{"/Record/_Previous", "<Alt>p", men_move, 10, "<Item>"},
		{"/Record/_Next", "<Alt>n", men_move, 11, "<Item>"},
		{"/Record/_Add", NULL, men_move, 12, "<Item>"},
		{"/Record/_Delete", NULL, men_move, 13, "<Item>"},
		{"/_Help", NULL, NULL, 14, "<LastBranch>"},
		{"/Help/_About...", NULL, men_move, 15, "<Item>"} };

	altered=FALSE;
	first = TRUE;
	count = 0;

	gtk_init (&argc, &argv);	// use default.crd or 1st param.
// added file name to title - 29/04/2001:
// moved window creation to before menu - core dump stop:01/05/2001.
	g_snprintf (wtitle, 255, "- GTK Windows Cardfile Viewer - %s", fl);
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(window), wtitle);
	gtk_widget_set_usize (window, 700, 500);
// hook signals:
	gtk_signal_connect (GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(exitapp), NULL);
	gtk_signal_connect (GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(exitapp), NULL);
	gtk_container_set_border_width (GTK_CONTAINER(window), 10);
	gtk_widget_realize (window);

	if (argc > 1)
		strcpy (fl, argv[1]);
	else
		strcpy (fl, "default.crd");
// create and initialize items as needed:
	listbox = gtk_list_new ();
	statdata = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(statdata), FALSE);
	htext = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(htext), FALSE);
	cardata = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(cardata), TRUE);
	// gtk_text_set_word_wrap (GTK_TEXT (cardata), TRUE);
	gtk_list_set_selection_mode (GTK_LIST (listbox), GTK_SELECTION_SINGLE);
// note: all listbox parameters had to be set before menu factory - core dump otherwise
// menu factory:
	accel = gtk_accel_group_new ();
	ifac = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel);
	gtk_item_factory_create_items (ifac, sizeof(items)/sizeof(items[0]), items, (gpointer) listbox);
	menubar = gtk_item_factory_get_widget (ifac, "<main>");
	// hidden file selection window:
	f_sel = gtk_file_selection_new("Select card file");
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(f_sel));
	//	gtk_file_selection_set_filename(GTK_FILE_SELECTION(f_sel), "./*.crd");
	gtk_signal_connect (GTK_OBJECT(f_sel), "delete_event", GTK_SIGNAL_FUNC(hide_win), NULL);
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(f_sel)->ok_button), "clicked", file_back, NULL);
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(f_sel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(hide_win), NULL);

	get_card_data (listbox, fl);
	box1 = gtk_hbox_new (FALSE, 15);
	butP = PackNewButton (box1, "Prev", TRUE);
	butN = PackNewButton (box1, "Next", TRUE);
	butA = PackNewButton (box1, "Add", TRUE);
	butD = PackNewButton (box1, "Delete", TRUE);
	butQ = PackNewButton (box1, "Quit", FALSE);
	gtk_signal_connect (GTK_OBJECT(butP), "clicked", GTK_SIGNAL_FUNC(callback), (gpointer) listbox);
	gtk_signal_connect (GTK_OBJECT(butN), "clicked", GTK_SIGNAL_FUNC(callnext), (gpointer) listbox);
	gtk_signal_connect (GTK_OBJECT(butA), "clicked", GTK_SIGNAL_FUNC(AddCard), (gpointer) listbox);
	gtk_signal_connect (GTK_OBJECT(butD), "clicked", GTK_SIGNAL_FUNC(DelCard), (gpointer) "Delete");
	gtk_signal_connect (GTK_OBJECT(butQ), "clicked", GTK_SIGNAL_FUNC(exitapp), NULL);
	gtk_signal_connect (GTK_OBJECT(listbox), "select_child", GTK_SIGNAL_FUNC(l_select), NULL);
	box2 = gtk_vbox_new (FALSE, 0);
	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	scrwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, FALSE);

	div1 = gtk_hseparator_new ();
	div2 = gtk_hseparator_new ();
	div3 = gtk_hseparator_new ();
	gtk_widget_set_usize (scrwin, 320, 400);
// usize: widget, width, height.
	gtk_widget_set_usize (htext, 350, 25); // 28/04/2001
	gtk_widget_set_usize (cardata, 350, 225);
	gtk_widget_set_usize (statdata, 350, 150);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrwin), listbox);
	gtk_box_pack_start (GTK_BOX(hbox), scrwin, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(vbox), htext, FALSE, FALSE, 0); // 28/04/2001
	gtk_box_pack_start (GTK_BOX(vbox), cardata, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(vbox), div3, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(vbox), statdata, FALSE, FALSE, 0);
// from item factory
	gtk_box_pack_start (GTK_BOX(box2), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box2), box1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box2), div1, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(box2), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box2), div2, FALSE, TRUE, 0);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel);
	gtk_container_add (GTK_CONTAINER(window), box2);
	gtk_widget_show_all (window);
// listbox, cardata, statdata, scrwin, box1, box2, hbox, vbox, div1, div2, div3
	gtk_list_select_item (GTK_LIST(listbox), count);
	gtk_main ();
	return 0;
}

// file IO for address book:
void get_card_data (GtkWidget *listbox, char *fl)
{
  gint nchar;
  FILE *fp;
  status = stat (fl, &s);		// status of file to be used
  if (!(status == 0 && S_ISREG(s.st_mode))) {
	 ShowMessage (fl, "File Doesn't Exist.");
	 return;
  }
  flen = s.st_size;		      // stat file length
  if ((fp = fopen (fl, "rb")) == NULL) {
	 ShowMessage ("I/O Error", "Cannot open file.");
	 flen = 0;
	 return;
  }
  ptr = g_new (gchar, flen);
  nchar = fread (ptr, 1, flen, fp);
  if (nchar < 5) {
	 ShowMessage ("I/O Error", "Cannot open file.");
	 flen = 0;
	 return;
  }
  fclose (fp);
  if (strncmp(ptr, "MGC", 3) != 0) {
	 ShowMessage ("File Error", "Invalid Signature String (not MGC)");
	 flen = 0;
	 return;
  }
  parsebuf ();
}

void parsebuf ()
{
  gint ofs, x;
  guint32 cdata;
  gchar *p1, *rec; // p1 is index, rec is data
  ccnt = ptr[3] + (ptr[4] << 8);
  g_list_free (rindx);
  g_list_free (rdata);
  rindx = NULL;
  rdata = NULL;
  for (x = 0 ; x < ccnt ; x++) {
	 ofs = x * RECLEN;    // single value for loop
	 p1 = &ptr[ofs+16];   // use address of data in ptr
	 rindx = g_list_append(rindx, p1);
	 cdata = ptr[ofs + 11];  // 4 byte offset value, lsb
	 cdata += ptr[ofs + 12] << 8;
	 cdata += ptr[ofs + 13] << 16;
	 cdata += ptr[ofs + 14] << 24; // msb
	 rec = &ptr[cdata + 4];
	 rdata = g_list_append(rdata, killcr((gchar *) rec));
  }
  re_sort (listbox, rindx);  // create listbox data from GList
}

void l_select (GtkWidget *widget, guint ndata)	// widget is callback's listbox
{
	gint position;
	gchar *ptr1;
	GList *node;
	GtkWidget *ditem;
	if (!first) {
	  gchar *entry = gtk_editable_get_chars(GTK_EDITABLE(cardata), 0, -1);
	  node = g_list_nth(rdata, count);
	  if (g_strcasecmp((gchar *) entry, (gchar *) node->data) != 0) {
		 node->data = (gpointer) entry;  // on removing old data: memory leak?
		 altered = TRUE;
		 g_print("%s\n", entry); // buffnew
	  }
	  else
		 g_free(entry); // buffnew
	}
	node = GTK_LIST(widget)->selection;
	if (!node)
	  return;
	ditem = GTK_WIDGET(node->data);		// list item data
	count = gtk_list_child_position (GTK_LIST(widget), ditem);
	ptr1 = g_list_nth_data (rindx, count);
	//gtk_text_freeze (GTK_TEXT(cardata));
	gtk_editable_delete_text (GTK_EDITABLE(cardata), 0, -1);
// not editable, but works.
	gtk_editable_delete_text (GTK_EDITABLE(htext), 0, -1);
	position = gtk_editable_get_position(GTK_EDITABLE(htext));
	gtk_editable_insert_text(GTK_EDITABLE(htext), ptr1, strlen(ptr1), &position);
	ptr1 = g_list_nth_data (rdata, count);  // record data
	position = gtk_editable_get_position(GTK_EDITABLE(cardata));
	gtk_editable_insert_text(GTK_EDITABLE(cardata), ptr1, strlen(ptr1), &position);
	//gtk_text_thaw (GTK_TEXT(cardata));
	first = FALSE;
	currstat ();
}

void currstat ()
{
	gint position;
	gchar *ptr1, *ptr2;
	gchar *head;
	ptr1 = g_list_nth_data (rindx, count);
	ptr2 = g_list_nth_data (rdata, count);
	//gtk_text_freeze (GTK_TEXT(statdata));
	gtk_editable_delete_text (GTK_EDITABLE(statdata), 0, -1);
	head = g_strdup_printf ("Entries: %d   Count: %d\nIndex: %d (data length: %d)\n%s", ccnt, count, strlen(ptr1), strlen(ptr2), ptr1);
	position = gtk_editable_get_position(GTK_EDITABLE(statdata));
	gtk_editable_insert_text(GTK_EDITABLE(statdata), head, strlen(head), &position);
	ptr1 = g_list_nth_data (rindx, 0);
	ptr2 = g_list_nth_data (rdata, 0);
	head = g_strdup_printf ("\n\nMGC length: %d   End:  %d (cards): %d", indpos, datpos, (guint16) ptr[3]);
	gtk_editable_insert_text(GTK_EDITABLE(statdata), head, strlen(head), &position);
	head = g_strdup_printf ("\n\nData start %d  length: %d\nFile: %s  [%d]\n", indpos + 1, datpos + 1, fl, flen);
	gtk_editable_insert_text(GTK_EDITABLE(statdata), head, strlen(head), &position);
        head = g_strdup_printf ("\nFile Status: %s", (altered ? "** Altered **":"** Unchanged **"));
	gtk_editable_insert_text(GTK_EDITABLE(statdata), head, strlen(head), &position);
	//gtk_text_thaw (GTK_TEXT(statdata));
	g_free(head);
}
// re_sort: called from CloseandAdd, men_move->save, CardOut and parsebuf
void re_sort (GtkWidget *list, GList *array1)
{
  GList *node, *a_go;
  gchar *ptr1;
  GtkWidget *item;
  indpos = 4;
  gtk_list_clear_items(GTK_LIST(list), 0, -1);
  a_go = g_list_nth(array1, 0);
  for (node = a_go ; node ; node = node->next) {
	 indpos += 52;
	 ptr1 = (gchar *) node->data;
	 item = gtk_list_item_new_with_label (ptr1);
	 gtk_container_add (GTK_CONTAINER(list), item);
	 gtk_widget_show (item);
  }
  a_go=g_list_nth(rdata, 0);
  datpos = indpos;
  for (node = a_go ; node ; node = node->next) {
	 datpos += strlen(addcr(node->data));
	 datpos += 4;
  }
}

gint eval_items (gpointer dat1, gpointer dat2)
{
  return(g_strcasecmp((gchar *) dat1, (gchar *) dat2));
}

gchar *killcr (gchar *str) // called from parsebuf
{
  gint x = 0;
  GString *gstr = g_string_new(str);
  while (x < gstr->len) {
	 if (gstr->str[x] == '\r')
		g_string_erase (gstr, x, 1);
	 else
		x++;
  }
  return gstr->str;
}

gchar *addcr (gchar *str) // called from re_sort, savbuff
{
  gint x;
  GString *gstr = g_string_new(str);
  if (gstr->str[0] == '\n') {
	 g_string_insert_c(gstr, 0, '\r');
  }
  for (x = 1 ; x < gstr->len ; x ++) {
	  if (gstr->str[x - 1] != '\r' && gstr->str[x] == '\n') {
		g_string_insert_c(gstr, x, '\r');
	  }
  }
  return gstr->str;
}

gchar *savbuff ()
{
  guint x, ofs, dat, slen;
  gchar *txt;
  gchar *bufn = g_strnfill (datpos + 1, 0);
  GList *node, *node1;
  bufn[0]='M';
  bufn[1]='G';
  bufn[2]='C';
  bufn[3]=(guchar) ccnt % 256;
  bufn[4]=(guchar) ccnt / 256;
  // start of card data calculated as indpos+1
  dat = indpos+1;
  for (x = 0 ; x < ccnt ; x++) {
	 ofs = x * RECLEN;
	 txt = &bufn[ofs + 16]; // rindx text start
	 // location of card data: offset 11-14, LSB low
	 bufn[ofs + 11] = (guchar) dat % 256;  // 4 byte value, lsb
	 bufn[ofs + 12] = (guchar) (dat >> 8) % 256;
	 bufn[ofs + 13] = (guchar) (dat >> 16) % 256;
	 bufn[ofs + 14] = (guchar) (dat >> 24)% 256; // msb
	 node = g_list_nth(rindx, x);
	 g_snprintf (txt, 40, "%s", (gchar *)node->data);
	 txt = &bufn[dat + 4]; // rdata text start
	 node1=g_list_nth(rdata, x);
	 slen = strlen(addcr(node1->data));
	 bufn[dat] = 0;
	 bufn[dat + 1] = 0;
	 bufn[dat + 2] = slen % 256;
	 bufn[dat + 3] = slen / 256;
	 g_snprintf (txt, slen+1, "%s", (gchar *) addcr(node1->data));
	 // slen+1: if slen, last character was being cut off.
	 dat += slen;
	 dat += 4;
  }
  g_free(ptr);
  ptr = &bufn[0];
  parsebuf();
  return ptr;
}

/*
Struct GList {
gpointer data;
GList prev;
GList next;
}

25 jun/2K - sub to calculate buffer length in Bytes
30 jun/2K - int altered var to indicate if changes made
26 Aug/2K - sub for file stat (Sam's text)
27 Aug/2K - move stat to main from file_exists sub. flen var
28 Apr/2K1 - Callbacks for menu segfaulting: parm error?
30 Apr/2K1 - Record section of menu works properly (gpointer) is OK
 8 May 2K1 - rewrite menubar entries to call single sub, calling same Fn's as buttons
 9 May 2K1 - AddCard sub to take index field. (req'd: chop length to 37 characters)
10 May 2K1 - CloseandAdd sub: set up dummy call from AddCard
18 May 2K1 - Rewrite some code to use list pointers for index, data
21 May 2K1 - And to get ready for AddCard's CloseandAdd to alter cardlist
21 May 2K1 - currstat sub for statdata  info created
21 May 2K1 - can't copy ptr list for appending lists. use address instead
23 May 2K1 - eval_items sub for insertion of array elements
23 May 2K1 - re_sort sub to repopulate listbox with sized array items
23 May 2K1 - re_sort calculates index length, data start, end file (datpos+1)
           - indpos+1: start of file data
24 May 2K1 - ToggleButton sub to create Edit button
24 May 2K1 - make listbox a global for easier population
24 May 2K1 - entries not added properly?? re_sort gets garbage from new list entries
26 May 2K1 - corrected - g_strdup_printf allocates memory for strings properly
26 May 2K1 - UpdateData sub for editing of rdata array
27 May 2K1 - remove Edit button: not used
28 May 2K1 - AddCard focus to ndex object: gtk_widget_grab_focus()
 1 Jun 2K1 - Checking array manipulation for node->data edit
 3 Jun 2K1 - Card edit directed through l_select(). UpdateData sub unused
 3 Jun 2K1 - Transients allow better window control for YesNo, ShowMessage subs
10 Jun 2K1 - separate parsebuf from get_card_data to allow parses from modification subs
15 Jun 2K1 - Identify AddCard as transient window for better placement.
16 Jun 2K1 - addcr and killcr subs to convert between DOS and Linux format strings
16 Jun 2K1 - start on savbuff, sub creating and filling new buffer for file storage
17 Jun 2K1 - savbuff works: parsebuf called at end, proper data adjustment
20 Jun 2K1 - DelCard calls CardOut for removal: removal problem caused by selected unit
                fixed by setting first=TRUE to prevent reading in of current card.
20 Jun 2K1 - new buffers using g_list_free to clear data in CardOut, parsebuff
20 Jun 2K1 - change: get_card_data sub returns instead of exits on bad file
20 Jun 2K1 - New File in menu set to create pointer to 5 bytes, clear entries
24 Jun 2K1 - load & parse files properly
24 Jun 2K1 - remove flen from get_card_data parameters - sub finds length
28 Jun 2K1 - Add 'Altered' display to statdata window, multi-use statdata's head string
*/
