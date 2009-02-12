/* Where Shall We Eat is copyright David King, 2009
 *
 * This file is part of Where Shall We Eat
 *
 * Where Shall We Eat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Where Shall We Eat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Where Shall We Eat  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include "config.h"

#define WSWE_MAINWINDOW_UI "main_window_uimanager.ui"

/* Columns in model for tree view. */
enum {
  NAME_COLUMN,
  STYLE_COLUMN,
  PRICE_COLUMN,
  QUALITY_COLUMN,
  VISITS_COLUMN,
  N_COLUMNS
};

/* Variables that need to be accessed across several functions. */
typedef struct {
  GtkWidget *window;
  GtkWidget *treeview;
} MainWindowData;

/* Function prototypes. */
static gchar *get_system_file (const gchar *filename);
static gboolean init_main_window (MainWindowData *data);
static void price_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);

/* Callbacks for menu and toolbar. */
static gboolean open_file_action (GtkWidget *widget, MainWindowData *user_data);
static gboolean save_file_action (GtkWidget *widget, MainWindowData *user_data);
static void quit_file_action (GtkWidget *widget, MainWindowData *user_data);
static void add_place_action (GtkWidget *widget, MainWindowData *user_data);
static void delete_place_action (GtkWidget *widget, MainWindowData *user_data);
static void choose_place_action (GtkWidget *widget, MainWindowData *user_data);
static void about_action (GtkWidget *widget, MainWindowData *user_data);

/* A list of entries that is passed to the GtkActionGroup */
static GtkActionEntry entries[] = 
{
  { "FileMenuAction", NULL, "_File" },
  { "GoMenuAction", NULL, "_Go" },
  { "HelpMenuAction", NULL, "_Help" },

  { "OpenAction", GTK_STOCK_OPEN, "_Open", "<control>O", "Open a file", G_CALLBACK (open_file_action) },
  { "SaveAction", GTK_STOCK_SAVE, "_Save", "<control>S", "Save a file", G_CALLBACK (save_file_action) },
  { "QuitAction", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK (quit_file_action) },

  { "AddPlaceAction", GTK_STOCK_ADD, "_Add place...", "<control>A", "Add an eating place", G_CALLBACK (add_place_action) },
  { "DeletePlaceAction", GTK_STOCK_DELETE, "_Delete place", "<control>D", "Delete an eating place", G_CALLBACK (delete_place_action) },
  { "ChoosePlaceAction", GTK_STOCK_EXECUTE, "_Choose place", "<control>H", "Choose an eating place", G_CALLBACK (choose_place_action) },

  { "AboutAction", GTK_STOCK_ABOUT, "A_bout", "<control>B", "About " PACKAGE_NAME, G_CALLBACK (about_action) }
};

/* Use G_N_ELEMENTS to count the number of elements in an array. */
static guint n_entries = G_N_ELEMENTS (entries);

/* Function to get a system file, e.g. a UI description. */
static gchar *get_system_file (const gchar *filename)
{
  const gchar *directory;
  gchar *pathname;
  const gchar* const *system_data_dirs;
  gboolean file_exists = FALSE;
  gint i = 0;

  system_data_dirs = g_get_system_data_dirs ();

  /* Iterate over array of strings to find system data files. */
  for (i = 0, file_exists = FALSE; !file_exists && (directory = system_data_dirs[i]) != NULL; i++)
  {
    pathname = g_build_filename (directory, PACKAGE_TARNAME, filename, NULL);
    file_exists = g_file_test (pathname, G_FILE_TEST_EXISTS);
    g_debug ("File %s exists: %s", pathname, file_exists ? "Yes" : "No");
    if (!file_exists)
    {
      g_free (pathname);
      pathname = NULL;
    }
  }

  return pathname;
}

/* Function to initialise main window and child widgets. */
static gboolean init_main_window (MainWindowData *data)
{
  GtkActionGroup *action_group = { 0, };
  GtkUIManager *ui_manager = { 0, };
  GError *ui_error = NULL;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkCellRenderer *text_renderer;
  GtkWidget *scrollwin;
  GtkWidget *vbox;
  GtkTreeStore *treestore;
  GtkTreeViewColumn *name_column;
  GtkTreeViewColumn *style_column;
  GtkTreeViewColumn *price_column;
  GtkTreeViewColumn *quality_column;
  GtkTreeViewColumn *visits_column;

  /* Create a new GtkActionGroup and add entries */
  action_group = gtk_action_group_new ("MainWindowActions");
  gtk_action_group_add_actions (action_group, entries, n_entries, data);

  /* Add action_group to ui_manager */
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  /* Load GtkUIManager description from file */
  if ( !(gtk_ui_manager_add_ui_from_file (ui_manager, get_system_file (WSWE_MAINWINDOW_UI), &ui_error)))
  {
    g_printerr ("Error loading GtkUIManager file: %s", ui_error->message);
    g_error_free (ui_error);
    return FALSE;
  }
  g_debug ("GtkUIManager file for main window loaded");

  /* Menu and toolbar setup. */
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  toolbar = gtk_ui_manager_get_widget (ui_manager, "/MainToolbar");

  /* Setup treestore and treeview. */
  treestore = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_UINT);
  data->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (treestore));
  g_object_unref (treestore);
  text_renderer = gtk_cell_renderer_text_new ();
  name_column = gtk_tree_view_column_new_with_attributes ("Name", text_renderer, "text", NAME_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), name_column, NAME_COLUMN);
  style_column = gtk_tree_view_column_new_with_attributes ("Style", text_renderer, "text", STYLE_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), style_column, STYLE_COLUMN);
  price_column = gtk_tree_view_column_new ();
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (data->treeview), VISITS_COLUMN, "Price", text_renderer, price_cell_data_func, data, NULL);
  quality_column = gtk_tree_view_column_new_with_attributes ("Quality", text_renderer, "text", QUALITY_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), quality_column, QUALITY_COLUMN);
  visits_column = gtk_tree_view_column_new_with_attributes ("Visits", text_renderer, "text", PRICE_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), visits_column, VISITS_COLUMN);

  /* Setup scrolled window for treeview. */
  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrollwin), data->treeview);
  
  /* VBox setup and packing. */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);

  /* Window setup. */
  data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (data->window), PACKAGE_STRING);
  g_set_application_name (PACKAGE_NAME);
  gtk_window_set_default_size (GTK_WINDOW (data->window), 350, 250);
  gtk_window_set_default_icon_name (PACKAGE_TARNAME);
  gtk_window_add_accel_group (GTK_WINDOW (data->window), gtk_ui_manager_get_accel_group (ui_manager));

  /* Unref ui_manager as no longer needed. */
  g_object_unref (ui_manager);

  /* Make destroy event end program. */
  g_signal_connect (G_OBJECT (data->window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  /* Show main window and all child widgets. */
  gtk_container_add (GTK_CONTAINER (data->window), vbox);
  gtk_widget_show_all (data->window);

  /* Successful initialisation. */
  return TRUE;
}

/* GtkTreeCellDataFunction to display a float as a currency. */
static void price_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
  gfloat price;
  gchar buffer[10]; /* EURxxx.xx\0 */

  gtk_tree_model_get (model, iter, PRICE_COLUMN, &price, -1);

  g_snprintf (buffer, sizeof(buffer), "EUR%.2f", price);

  g_object_set (renderer, "text", buffer, NULL);
}

/* Callback function to open a data file. */
static gboolean open_file_action (GtkWidget *widget, MainWindowData *user_data)
{
  g_debug ("Open a file");

  return TRUE;
}

/* Callback function to save a data file. */
static gboolean save_file_action (GtkWidget *widget, MainWindowData *user_data)
{
  g_debug ("Save a file");

  return TRUE;
}

/* Callback function to quit program. */
static void quit_file_action (GtkWidget *widget, MainWindowData *user_data)
{
  g_debug ("Quit program");

  gtk_main_quit ();
}

/* Callback function to add an eating place. */
static void add_place_action (GtkWidget *widget, MainWindowData *user_data)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *combobox;
  GtkWidget *label_name;
  GtkWidget *label_style;
  const gchar *name;
  gchar *style;
  gchar *name_model;
  GtkTreeModel *model;
  GtkTreeIter iter = { 0, };

  /* Create a new dialog, with stock buttons ans responses. */
  dialog = gtk_dialog_new_with_buttons ("Add an eating place", GTK_WINDOW (user_data->window), GTK_DIALOG_MODAL, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_OK, NULL);

  /* Setup entry field for name. */
  entry = gtk_entry_new ();
  gtk_widget_set_tooltip_text (entry, "Name of eating place");

  /* Setup ComboBoxEntryText with list of styles. Default to first item. */
  combobox = gtk_combo_box_new_text ();
  gtk_widget_set_tooltip_text (combobox, "Style of eating place");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), "Asian");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), "Indian");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), "Italian");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), "Turkish");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), "Vietnamese");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);

  /* Setup labels, with mnemonics and tooltips. */
  label_name = gtk_label_new_with_mnemonic ("_Name");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_name), entry);
  gtk_misc_set_alignment (GTK_MISC (label_name), 0, 0.5);
  label_style = gtk_label_new_with_mnemonic ("_Style");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_style), combobox);
  gtk_misc_set_alignment (GTK_MISC (label_style), 0, 0.5);

  /* Setup and pack table with widgets. */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_attach_defaults (GTK_TABLE (table), label_name, 0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), label_style, 0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), combobox, 1, 2, 1, 2);

  /* Pack table into dialog's vbox. GTK_DIALOG cast needed as GtkWidget does
   * not have a member "vbox". */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, FALSE, 0);
  gtk_widget_show_all (dialog);

  /* Run dialog and check for valid response */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    name = gtk_entry_get_text (GTK_ENTRY (entry));
    style = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combobox));

    /* g_ascii_strcasecmp returns 0 (FALSE) on strings matching. */
    if (g_ascii_strcasecmp (name, "") == 0)
    {
      g_debug ("Empty place name");
      gtk_widget_destroy (dialog);

      g_free (style);
      return;
    }

    /* Check model for eating place that already exists. */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data->treeview));
    if (gtk_tree_model_get_iter_from_string (model, &iter, "0"))
    {

      do
      {
        gtk_tree_model_get (model, &iter, NAME_COLUMN, &name_model, -1);
      
        if (g_ascii_strcasecmp (name, name_model) == 0)
        {
          g_free (name_model);
          gtk_widget_destroy (dialog);
          g_debug ("Name already exists in model.");
          return;
        }

        g_free (name_model);
      } while (gtk_tree_model_iter_next (model, &iter));
    }

    /* Append new place to end of treestore. */
    gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, NAME_COLUMN, name, STYLE_COLUMN, style, -1);
    g_free (style);
  }

  gtk_widget_destroy (dialog);
}

/* Callback function to delete an eating place. */
static void delete_place_action (GtkWidget *widget, MainWindowData *user_data)
{
  g_debug ("Delete an eating place");
}

/* Callback function to randomly select an eating place */
static void choose_place_action (GtkWidget *widget, MainWindowData *user_data)
{
  g_debug ("Choose an eating place");
}

/* Show about dialog. Convenience function hides dialog when the close button
 * is clicked, keeping it in memory for future invocation. */
static void about_action (GtkWidget *widget, MainWindowData *user_data)
{
  gtk_show_about_dialog (GTK_WINDOW (user_data->window), "program-name", PACKAGE_NAME, "version", PACKAGE_VERSION, "comments", "A GTK+ application to randomly select a place to eat.", "license", "Where Shall We Eat is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\nWhere Shall We Eat is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License along with Where Shall We Eat.  If not, see <http://www.gnu.org/licenses/>.", "wrap-license", TRUE, "copyright", "Copyright 2009 David King", NULL);
}

/* Main function to allocate memory for typedef'd struct and start GTK+ main
 * loop */
int main (int argc, char *argv[])
{
  MainWindowData *data;

  gtk_init (&argc, &argv);

  data = g_slice_new (MainWindowData);
  if (! (init_main_window (data)))
  {
    g_slice_free (MainWindowData, data);
    return 1;
  }

  gtk_main ();

  g_slice_free (MainWindowData, data);

  return 0;
}
