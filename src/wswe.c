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

enum {
  NAME_COLUMN,
  STYLE_COLUMN,
  PRICE_COLUMN,
  QUALITY_COLUMN,
  VISITS_COLUMN,
  N_COLUMNS
};

typedef struct {
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *scrollwin;
  GtkTreeStore *treestore;
  GtkTreeIter iter;
  GtkTreeIter child;
  GtkTreeViewColumn *name_column;
  GtkTreeViewColumn *style_column;
  GtkTreeViewColumn *price_column;
  GtkTreeViewColumn *quality_column;
  GtkTreeViewColumn *visits_column;
  GtkWidget *treeview;
} MainWindowData;

gchar *get_system_file (const gchar *filename);
gboolean init_main_window (MainWindowData *data);

static gboolean open_file_action (GtkWidget *widget, gpointer *user_data);
static gboolean save_file_action (GtkWidget *widget, gpointer *user_data);
static void quit_file_action (GtkWidget *widget, gpointer *user_data);
static void add_place_action (GtkWidget *widget, gpointer *user_data);
static void delete_place_action (GtkWidget *widget, gpointer *user_data);
static void choose_place_action (GtkWidget *widget, gpointer *user_data);
static void about_action (GtkWidget *widget, gpointer *user_data);

/* A list of entries that is passed to the GtkActionGroup */
static GtkActionEntry entries[] = 
{
  { "FileMenuAction", NULL, "_File" },
  { "GoMenuAction", NULL, "_Go" },
  { "HelpMenuAction", NULL, "_Help" },

  { "OpenAction", GTK_STOCK_OPEN, "_Open", "<control>L", "Open a file", G_CALLBACK (open_file_action) },
  { "SaveAction", GTK_STOCK_SAVE, "_Save", "<control>S", "Save a file", G_CALLBACK (save_file_action) },
  { "QuitAction", GTK_STOCK_QUIT, "_Quit", "<coltrol>Q", "Quit", G_CALLBACK (quit_file_action) },

  { "AddPlaceAction", GTK_STOCK_ADD, "_Add place...", "<control>A", "Add an eating place", G_CALLBACK (add_place_action) },
  { "DeletePlaceAction", GTK_STOCK_DELETE, "_Delete place", "<control>D", "Delete an eating place", G_CALLBACK (delete_place_action) },
  { "ChoosePlaceAction", GTK_STOCK_EXECUTE, "_Choose place", "<control>H", "Choose an eating place", G_CALLBACK (choose_place_action) },

  { "AboutAction", GTK_STOCK_ABOUT, "A_bout", "<control>B", "About " PACKAGE_NAME, G_CALLBACK (about_action) }
};

static guint n_entries = G_N_ELEMENTS (entries);

/* Function to get a system file, e.g. a UI description. */
gchar *get_system_file (const gchar *filename)
{
  const gchar *directory;
  gchar *pathname;
  const gchar* const *system_data_dirs;
  gboolean file_exists = FALSE;
  gint i = 0;

  system_data_dirs = g_get_system_data_dirs ();

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
gboolean init_main_window (MainWindowData *data)
{
  GtkActionGroup *action_group = { 0, };
  GtkUIManager *ui_manager = { 0, };
  GError *ui_error = NULL;
  GtkWidget *menubar;
  GtkWidget *toolbar;

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

  data->treestore = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_INT);
  data->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (data->treestore));
  
  /* VBox setup. */
  data->vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (data->vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (data->vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (data->vbox), data->treeview, TRUE, TRUE, 0);

  /* Window setup. */
  data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (data->window), PACKAGE_STRING);
  g_set_application_name (PACKAGE_NAME);
  gtk_container_set_border_width (GTK_CONTAINER (data->window), 12);
  gtk_window_set_default_size (GTK_WINDOW (data->window), 350, 250);
  gtk_window_set_default_icon_name (PACKAGE_TARNAME);
  gtk_window_add_accel_group (GTK_WINDOW (data->window), gtk_ui_manager_get_accel_group (ui_manager));
  g_object_unref (ui_manager);

  /* Make destroy event end program. */
  g_signal_connect (G_OBJECT (data->window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  /* Show main window and all child widgets. */
  gtk_container_add (GTK_CONTAINER (data->window), data->vbox);
  gtk_widget_show_all (data->window);

  return TRUE;
}

/* Callback function to open a data file. */
static gboolean open_file_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("Open a file");

  return TRUE;
}

static gboolean save_file_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("Save a file");

  return TRUE;
}

static void quit_file_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("Quit program");

  gtk_main_quit ();
}

static void add_place_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("Add an eating place");
}

static void delete_place_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("Delete an eating place");
}

static void choose_place_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("Choose an eating place");
}

static void about_action (GtkWidget *widget, gpointer *user_data)
{
  g_debug ("About WSWE");
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
