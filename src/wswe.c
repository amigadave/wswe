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
enum
{
  NAME_COLUMN,
  STYLE_COLUMN,
  PRICE_COLUMN,
  QUALITY_COLUMN,
  VISITS_COLUMN,
  N_COLUMNS
};

enum
{
  NAME_SORTID
};

/* Variables that need to be accessed across several functions. */
typedef struct
{
  GtkWidget *window;
  GtkWidget *treeview;
  GtkWidget *add_place_dialog;
  GtkWidget *add_place_dialog_entry;
  GtkWidget *add_place_dialog_combobox;
} MainWindowData;

/* Function prototypes. */
static gchar *get_system_file (const gchar *filename);
static gboolean init_main_window (MainWindowData *data);
static void init_add_place_dialog (MainWindowData *user_data);
static void hide_add_place_dialog(MainWindowData *user_data);
static void price_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void name_column_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_string, MainWindowData *user_data);
static gboolean only_toplevel_visible (GtkTreeModel *model, GtkTreeIter *iter, MainWindowData *user_data);
static void remove_row (GtkTreeRowReference *ref, GtkTreeModel *model);
static gint sort_place_name (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

/* Callbacks for menu and toolbar. */
static gboolean open_file_action (GtkWidget *widget, MainWindowData *user_data);
static gboolean save_file_action (GtkWidget *widget, MainWindowData *user_data);
static void quit_file_action (GtkWidget *widget, MainWindowData *user_data);
static void add_place_action (GtkWidget *widget, MainWindowData *user_data);
static void add_place_action_response (GtkWidget *widget, gint response_id, MainWindowData *user_data);
static gboolean add_place_delete_event (GtkWidget *widget, MainWindowData *user_data);
static void remove_place_action (GtkWidget *widget, MainWindowData *user_data);
static void add_visit_action (GtkWidget *widget, MainWindowData *user_data);
static void remove_visit_action (GtkWidget *widget, MainWindowData *user_data);
static void choose_place_action (GtkWidget *widget, MainWindowData *user_data);
static void about_action (GtkWidget *widget, MainWindowData *user_data);

/* A list of entries that is passed to the GtkActionGroup. */
static const GtkActionEntry entries[] = 
{
  { "FileMenuAction", NULL, "_File" },
  { "GoMenuAction", NULL, "_Go" },
  { "HelpMenuAction", NULL, "_Help" },

  { "OpenAction", GTK_STOCK_OPEN, "_Open", "<control>O", "Open a file", G_CALLBACK (open_file_action) },
  { "SaveAction", GTK_STOCK_SAVE, "_Save", "<control>S", "Save a file", G_CALLBACK (save_file_action) },
  { "QuitAction", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK (quit_file_action) },

  { "AddPlaceAction", GTK_STOCK_ADD, "_Add place...", "<control>A", "Add an eating place", G_CALLBACK (add_place_action) },
  { "RemovePlaceAction", GTK_STOCK_REMOVE, "_Remove place", "<control>R", "Remove an eating place", G_CALLBACK (remove_place_action) },
  { "AddVisitAction", GTK_STOCK_ADD, "Add _visit...", "<control>I", "Add a visit to an eating place", G_CALLBACK (add_visit_action) },
  { "RemoveVisitAction", GTK_STOCK_REMOVE, "R_emove visit", "<control>E", "Remove a visit to an eating place", G_CALLBACK (remove_visit_action) },
  { "ChoosePlaceAction", GTK_STOCK_EXECUTE, "_Choose place", "<control>H", "Choose an eating place", G_CALLBACK (choose_place_action) },

  { "AboutAction", GTK_STOCK_ABOUT, "A_bout", "<control>B", "About " PACKAGE_NAME, G_CALLBACK (about_action) }
};

/* Function to get a system file, e.g. a UI description. */
static gchar *get_system_file (const gchar *filename)
{
  gchar *pathname;
  const gchar* const *system_data_dirs;

  /* Iterate over array of strings to find system data files. */
  for (system_data_dirs = g_get_system_data_dirs (); system_data_dirs != NULL; system_data_dirs++)
  {
    pathname = g_build_filename (*system_data_dirs, PACKAGE_TARNAME, filename, NULL);
    if (g_file_test (pathname, G_FILE_TEST_EXISTS))
    {
      break;
    }
    else
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
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GError *ui_error = NULL;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkCellRenderer *text_renderer;
  GtkCellRenderer *progress_renderer;
  GtkWidget *scrollwin;
  GtkWidget *vbox;
  GtkTreeStore *treestore;
  GtkTreeSortable *sortable;
  GtkTreeViewColumn *name_column;
  GtkTreeViewColumn *style_column;
  GtkTreeViewColumn *price_column;
  GtkTreeViewColumn *quality_column;
  GtkTreeViewColumn *visits_column;
  GtkTreeSelection *selection;

  /* Create a new GtkActionGroup and add entries. */
  action_group = gtk_action_group_new ("MainWindowActions");
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), data);

  /* Add action_group to ui_manager. */
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  /* Load GtkUIManager description from file. */
  if ( !(gtk_ui_manager_add_ui_from_file (ui_manager, get_system_file (WSWE_MAINWINDOW_UI), &ui_error)))
  {
    g_printerr ("Error loading GtkUIManager file: %s", ui_error->message);
    g_error_free (ui_error);
    return FALSE;
  }

  /* Menu and toolbar setup. */
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  toolbar = gtk_ui_manager_get_widget (ui_manager, "/MainToolbar");

  /* Setup treestore and treeview. */
  treestore = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_UINT, G_TYPE_UINT);
  sortable = GTK_TREE_SORTABLE (treestore);
  gtk_tree_sortable_set_sort_func (sortable, NAME_SORTID, sort_place_name, data, NULL);
  data->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (treestore));
  g_object_unref (treestore);
  text_renderer = gtk_cell_renderer_text_new ();
  g_object_set (text_renderer, "editable", TRUE, NULL);
  name_column = gtk_tree_view_column_new_with_attributes ("Name", text_renderer, "text", NAME_COLUMN, NULL);
  g_signal_connect (text_renderer, "edited", (GCallback) name_column_edited, data);
  gtk_tree_view_column_set_sort_column_id (name_column, NAME_SORTID);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), name_column, NAME_COLUMN);
  text_renderer = gtk_cell_renderer_text_new ();
  style_column = gtk_tree_view_column_new_with_attributes ("Style", text_renderer, "text", STYLE_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), style_column, STYLE_COLUMN);
  text_renderer = gtk_cell_renderer_text_new ();
  price_column = gtk_tree_view_column_new ();
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (data->treeview), VISITS_COLUMN, "Price", text_renderer, price_cell_data_func, data, NULL);
  progress_renderer = gtk_cell_renderer_progress_new ();
  quality_column = gtk_tree_view_column_new_with_attributes ("Quality", progress_renderer, "value", QUALITY_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), quality_column, QUALITY_COLUMN);
  text_renderer = gtk_cell_renderer_text_new ();
  visits_column = gtk_tree_view_column_new_with_attributes ("Visits", text_renderer, "text", VISITS_COLUMN, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (data->treeview), visits_column, VISITS_COLUMN);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

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
  gtk_window_set_default_size (GTK_WINDOW (data->window), 350, 250);
  gtk_window_add_accel_group (GTK_WINDOW (data->window), gtk_ui_manager_get_accel_group (ui_manager));

  /* Unref ui_manager as no longer needed. */
  g_object_unref (ui_manager);

  /* Make destroy event end program. */
  g_signal_connect (G_OBJECT (data->window), "destroy", G_CALLBACK (quit_file_action), NULL);

  /* Show main window and all child widgets. */
  gtk_container_add (GTK_CONTAINER (data->window), vbox);
  gtk_widget_show_all (data->window);

  /* Initialise dialogs. */
  init_add_place_dialog (data);

  /* Successful initialisation. */
  return TRUE;
}

/* Function to initialise a dialog to add an eating place. */
static void init_add_place_dialog (MainWindowData *user_data)
{
  GtkWidget *table;
  GtkWidget *label_name;
  GtkWidget *label_style;

  /* Create a new dialog, with stock buttons ans responses. */
  user_data->add_place_dialog = gtk_dialog_new_with_buttons ("Add an eating place", GTK_WINDOW (user_data->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_OK, NULL);

  /* Setup entry field for name. */
  user_data->add_place_dialog_entry = gtk_entry_new ();
  gtk_widget_set_tooltip_text (user_data->add_place_dialog_entry, "Name of eating place");
  gtk_entry_set_activates_default (GTK_ENTRY (user_data->add_place_dialog_entry), TRUE);
  gtk_dialog_set_default_response (GTK_DIALOG (user_data->add_place_dialog), GTK_RESPONSE_OK);

  /* Setup ComboBoxEntryText with list of styles. Default to first item. */
  user_data->add_place_dialog_combobox = gtk_combo_box_new_text ();
  gtk_widget_set_tooltip_text (user_data->add_place_dialog_combobox, "Style of eating place");
  gtk_combo_box_append_text (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), "Asian");
  gtk_combo_box_append_text (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), "Indian");
  gtk_combo_box_append_text (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), "Italian");
  gtk_combo_box_append_text (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), "Turkish");
  gtk_combo_box_append_text (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), "Vietnamese");
  gtk_combo_box_set_active (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), 0);

  /* Setup labels, with mnemonics and tooltips. */
  label_name = gtk_label_new_with_mnemonic ("_Name");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_name), user_data->add_place_dialog_entry);
  gtk_misc_set_alignment (GTK_MISC (label_name), 0, 0.5);
  label_style = gtk_label_new_with_mnemonic ("_Style");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_style), user_data->add_place_dialog_combobox);
  gtk_misc_set_alignment (GTK_MISC (label_style), 0, 0.5);

  /* Setup and pack table with widgets. */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_attach_defaults (GTK_TABLE (table), label_name, 0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), label_style, 0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), user_data->add_place_dialog_entry, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), user_data->add_place_dialog_combobox, 1, 2, 1, 2);

  /* Pack table into dialog's vbox. Connect signals and show dialog. */
  gtk_box_pack_start ( GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (user_data->add_place_dialog))), table, TRUE, FALSE, 0);
  g_signal_connect (G_OBJECT (user_data->add_place_dialog), "response", G_CALLBACK (add_place_action_response), user_data);
  g_signal_connect (G_OBJECT (user_data->add_place_dialog), "delete-event", G_CALLBACK (add_place_delete_event), user_data);
}

/* Hide the dialog to add an eating place. */
static void hide_add_place_dialog(MainWindowData *user_data)
{
  if (GTK_WIDGET_VISIBLE (user_data->add_place_dialog))
  {
    gtk_entry_set_text (GTK_ENTRY (user_data->add_place_dialog_entry), "");
    gtk_combo_box_set_active (GTK_COMBO_BOX (user_data->add_place_dialog_combobox), 0);
    gtk_widget_hide_all (user_data->add_place_dialog);
  }
}

/* Callback function for Name column to be edited. */
static void name_column_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_string, MainWindowData *user_data)
{
  GtkTreeIter iter = { 0, };
  GtkTreeModel *model;

  model = gtk_tree_view_get_model( GTK_TREE_VIEW (user_data->treeview));

  if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
  {
    if (new_string != NULL)
    {
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, NAME_COLUMN, new_string, -1);
    }
  }
}

/* GtkTreeModelFilterVisibleFunc to only show toplevel rows in the treestore. */
static gboolean only_toplevel_visible (GtkTreeModel *model, GtkTreeIter *iter, MainWindowData *user_data)
{
  GtkTreeIter parent = { 0, };

  if (gtk_tree_model_iter_parent (model, &parent, iter))
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

/* GtkTreeCellDataFunction to display a float as a currency. */
static void price_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
  gfloat price = 0.0;
  gchar *price_string; /* EURxxx.xx\0 */

  gtk_tree_model_get (model, iter, PRICE_COLUMN, &price, -1);

  if (price > 0)
  {
    price_string = g_strdup_printf ("EUR%.2f", price);
    g_object_set (renderer, "text", price_string, NULL);
    g_free (price_string);
  }
  else
  {
    g_object_set (renderer, "text", "", NULL);
  }

}

/* Function to remove a row from a model using a TreeRowReference. */
static void remove_row (GtkTreeRowReference *ref, GtkTreeModel *model)
{
  GtkTreeIter iter = { 0, };
  GtkTreePath *path;

  /* Convert reference to path, then to iter. Then, remove iter from store. */
  path = gtk_tree_row_reference_get_path (ref);
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
  gtk_tree_path_free (path);
}

/* Function to sort columns in the treeview. */
static gint sort_place_name (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
  gchar *name1;
  gchar *name2;
  gint sortval = 0;

  gtk_tree_model_get (model, a, NAME_COLUMN, &name1, -1);
  gtk_tree_model_get (model, b, NAME_COLUMN, &name2, -1);

  if (name1 == NULL || name2 == NULL)
  {
    if (name1 == NULL && name2 == NULL)
    {
      g_free (name1);
      g_free (name2);
      return sortval;
    }

    sortval = (name1 == NULL) ? -1 : 1;
  }
  else
  {
    sortval = g_utf8_collate (name1, name2);
  }

  g_free (name1);
  g_free (name2);

  return sortval;
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
  gtk_main_quit ();
}

/* Callback function to show the add_place dialog, but only once. */
static void add_place_action (GtkWidget *widget, MainWindowData *user_data)
{
  if (!GTK_WIDGET_VISIBLE (user_data->add_place_dialog))
  {
    gtk_dialog_set_default_response (GTK_DIALOG (user_data->add_place_dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all (user_data->add_place_dialog);
  }
}

/* Callback function to check response id of add_place dialog. */
static void add_place_action_response (GtkWidget *widget, gint response_id, MainWindowData *user_data)
{
  const gchar *name;
  gchar *style;
  gchar *name_model;
  GtkTreeModel *model;
  GtkTreeIter iter = { 0, };

  switch (response_id)
  {
    case GTK_RESPONSE_OK:
      name = gtk_entry_get_text (GTK_ENTRY (user_data->add_place_dialog_entry));
      style = gtk_combo_box_get_active_text (GTK_COMBO_BOX (user_data->add_place_dialog_combobox));

      /* Don't add an empty place name to the treestore. */
      if (name[0] == '\0')
      {
        g_debug ("Empty place name");
        g_free (style);

        hide_add_place_dialog (user_data);
        return;
      }

      /* Check model for eating place that already exists. */
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data->treeview));
      if (gtk_tree_model_get_iter_first (model, &iter))
      {
        do
        {
          gtk_tree_model_get (model, &iter, NAME_COLUMN, &name_model, -1);

          if (g_ascii_strcasecmp (name, name_model) == 0)
          {
            g_free (name_model);
            hide_add_place_dialog (user_data);
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
      break;
    case GTK_RESPONSE_CANCEL: case GTK_RESPONSE_DELETE_EVENT:
      hide_add_place_dialog (user_data);
      break;
    default:
      g_return_if_reached();
      break;
  }
  hide_add_place_dialog (user_data);
}

/* Callback function to prevent dialog from being deleted. */
static gboolean add_place_delete_event(GtkWidget *widget, MainWindowData *user_data)
{
  /* Don't let GTK+ destroy the dialog. */
  return TRUE;
}

/* Callback function to delete an eating place. */
static void remove_place_action (GtkWidget *widget, MainWindowData *user_data)
{
  GtkTreeSelection *selection;
  GtkTreeRowReference *ref;
  GtkTreeModel *model;
  GList *rows = NULL;
  GList *no_children = NULL;
  GList *path_to_ref = NULL;
  GList *treerowref = NULL;

  /* Get GList of selected rows from treeview. */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data->treeview));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data->treeview));
  rows = gtk_tree_selection_get_selected_rows (selection, &model);

  /* Remove child paths (visits) from list. */
  no_children = rows;
  while (no_children != NULL)
  {
    /* Only remove if depth > 1. */
    if (gtk_tree_path_get_depth ( (GtkTreePath*) no_children->data) > 1)
    {
      gtk_tree_path_free ((GtkTreePath*) no_children->data);
      no_children = g_list_remove (no_children, no_children->data);
      rows = no_children;
      if (no_children != NULL)
      {
        no_children = no_children->next;
      }
    }
    else
    {
      no_children = no_children->next;
    }
  }

  /* Create GtkTreeRowReference for each of the selected rows. */
  path_to_ref = rows;
  while (path_to_ref != NULL)
  {
    ref = gtk_tree_row_reference_new (model, (GtkTreePath*) path_to_ref->data);
    treerowref = g_list_prepend (treerowref, gtk_tree_row_reference_copy (ref));
    gtk_tree_row_reference_free (ref);
    path_to_ref = path_to_ref->next;
  }

  /* Remove selected rows from model. */
  g_list_foreach (treerowref, (GFunc) remove_row, model);

  /* Free treerowref, paths and lists. */
  g_list_foreach (treerowref, (GFunc) gtk_tree_row_reference_free, NULL);
  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (treerowref);
  g_list_free (rows);
}

/* Callback function to add a visit to an eating place to the treestore. */
static void add_visit_action (GtkWidget *widget, MainWindowData *user_data)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *combo_name;
  GtkCellRenderer *text_renderer;
  GtkWidget *spinbutton_price;
  GtkWidget *spinbutton_quality;
  GtkWidget *label_name;
  GtkWidget *label_price;
  GtkWidget *label_quality;
  gdouble price = 0.0;
  guint quality = 0;
  GtkTreeModel *model;
  GtkTreeModel *model_only_toplevel;
  GtkTreeIter iter = { 0, };
  GtkTreeIter child = { 0, };
  GtkTreePath *path = NULL;
  GtkTreePath *child_path = NULL;

  /* Create a new dialog, with stock buttons and responses. */
  dialog = gtk_dialog_new_with_buttons ("Add a visit to an eating place", GTK_WINDOW (user_data->window), GTK_DIALOG_MODAL, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  /* Setup new treemodel to filter out child rows. */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data->treeview));
  model_only_toplevel = gtk_tree_model_filter_new (model, NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (model_only_toplevel), (GtkTreeModelFilterVisibleFunc) only_toplevel_visible, user_data, NULL);

  /* Setup ComboBox to select names from model. */
  combo_name = gtk_combo_box_new_with_model (model_only_toplevel);
  text_renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_name), text_renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_name), text_renderer, "text", NAME_COLUMN, NULL);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_name), 0);
  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_name), &iter))
  {
    g_debug ("No places to add visit to");
    return;
  }

  /* Setup spinbuttons for price and quality. */
  spinbutton_price = gtk_spin_button_new_with_range (0.0, 100.0, 0.01);
  gtk_widget_set_tooltip_text (spinbutton_price, "Price of meal at eating place");
  spinbutton_quality = gtk_spin_button_new_with_range (0.0, 100.0, 1.0);
  gtk_widget_set_tooltip_text (spinbutton_quality, "Quality of visit to eating place, as a percentage");

  /* Setup labels, with mnemonics and tooltips. */
  label_name = gtk_label_new_with_mnemonic ("_Name");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_name), combo_name);
  gtk_misc_set_alignment (GTK_MISC (label_name), 0, 0.5);
  label_price = gtk_label_new_with_mnemonic ("_Price");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_price), spinbutton_price);
  gtk_misc_set_alignment (GTK_MISC (label_price), 0, 0.5);
  label_quality = gtk_label_new_with_mnemonic ("_Quality");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label_quality), spinbutton_quality);
  gtk_misc_set_alignment (GTK_MISC (label_quality), 0, 0.5);

  /* Setup and pack table with widgets. */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_attach_defaults (GTK_TABLE (table), label_name, 0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), label_price, 0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), label_quality, 0, 1, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), combo_name, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton_price, 1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton_quality, 1, 2, 2, 3);

  /* Pack table into dialog's vbox. GTK_DIALOG cast needed as GtkWidget does
   * not have a member "vbox". */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, FALSE, 0);
  gtk_widget_show_all (dialog);

  /* Run dialog and check for valid response. */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    /* Combo box uses same model as treeview, so we can get the iter to the
     * item that is currently selected and use it to create a new visit as
     * a child. */
    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_name), &iter);
    price = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spinbutton_price));
    quality = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spinbutton_quality));

    /* Must have paid money for our meal. */
    if (price == 0.0)
    {
      g_debug ("Price of zero");
      gtk_widget_destroy (dialog);
      return;
    }

    /* Convert eating place iter into a path so that it remains valid, even
     * after addition of a new visit. */
    path = gtk_tree_model_get_path (model_only_toplevel, &iter);
    child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (model_only_toplevel), path);
    gtk_tree_model_get_iter (model, &iter, child_path);
    gtk_tree_store_append (GTK_TREE_STORE (model), &child, &iter);
    gtk_tree_store_set (GTK_TREE_STORE (model), &child, PRICE_COLUMN, price, QUALITY_COLUMN, quality, -1);

    /* TODO: Average quality and price, and add to place. */
    gtk_tree_path_free (path);
  }

  gtk_widget_destroy (dialog);
}

/* Callback function to remove a visit to an eating place from the treestore. */
static void remove_visit_action (GtkWidget *widget, MainWindowData *user_data)
{
  GtkTreeSelection *selection;
  GtkTreeRowReference *ref;
  GtkTreeModel *model;
  GList *rows = NULL;
  GList *no_toplevel = NULL;
  GList *path_to_ref = NULL;
  GList *treerowref = NULL;

  /* Get GList of selected rows from treeview. */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_data->treeview));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data->treeview));
  rows = gtk_tree_selection_get_selected_rows (selection, &model);

  /* Remove child paths (visits) from list. */
  no_toplevel = rows;
  while (no_toplevel != NULL)
  {
    /* Only remove if depth is 1, i.e. a toplevel item (eating place). */
    if (gtk_tree_path_get_depth ( (GtkTreePath*) no_toplevel->data) == 1)
    {
      gtk_tree_path_free ((GtkTreePath*) no_toplevel->data);
      no_toplevel = g_list_remove (no_toplevel, no_toplevel->data);
      /* Set rows pointer to new start of list. */
      rows = no_toplevel;
      if (no_toplevel != NULL)
      {
        no_toplevel = no_toplevel->next;
      }
    }
    else
    {
      no_toplevel = no_toplevel->next;
    }
  }

  /* Create GtkTreeRowReference for each of the selected rows. */
  path_to_ref = rows;
  while (path_to_ref != NULL)
  {
    ref = gtk_tree_row_reference_new (model, (GtkTreePath*) path_to_ref->data);
    treerowref = g_list_prepend (treerowref, gtk_tree_row_reference_copy (ref));
    gtk_tree_row_reference_free (ref);
    path_to_ref = path_to_ref->next;
  }

  /* Remove selected rows from model. */
  g_list_foreach (treerowref, (GFunc) remove_row, model);

  /* Free treerowref, paths and lists. */
  g_list_foreach (treerowref, (GFunc) gtk_tree_row_reference_free, NULL);
  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (treerowref);
  g_list_free (rows);
}

/* Callback function to randomly select an eating place. */
static void choose_place_action (GtkWidget *widget, MainWindowData *user_data)
{
  g_debug ("Choose an eating place");
}

/* Show about dialog. Convenience function hides dialog when the close button
 * is clicked, keeping it in memory for future invocation. */
static void about_action (GtkWidget *widget, MainWindowData *user_data)
{
  gtk_show_about_dialog (GTK_WINDOW (user_data->window),
      "program-name", PACKAGE_NAME,
      "version", PACKAGE_VERSION,
      "comments", "A GTK+ application to randomly select a place to eat.",
      "license", "Where Shall We Eat is free software: "
      "you can redistribute it and/or modify it under the terms of the "
      "GNU General Public License as published by the Free Software "
      "Foundation, either version 3 of the License, or (at your option) "
      "any later version.\n\nWhere Shall We Eat is distributed in the hope "
      "that it will be useful, but WITHOUT ANY WARRANTY; without even the "
      "implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR "
      "PURPOSE. See the GNU General Public License for more details.\n\n"
      "You should have received a copy of the GNU General Public License "
      "along with Where Shall We Eat. If not, see "
      "<http://www.gnu.org/licenses/>.",
      "wrap-license", TRUE, "copyright", "Copyright © 2009 David King", NULL);
}

/* Main function to allocate memory for typedef'd struct and start GTK+ main
 * loop. */
int main (int argc, char *argv[])
{
  MainWindowData *data;

  /* GTK+ initialisation. */
  g_set_application_name (PACKAGE_NAME);
  gtk_init (&argc, &argv);
  gtk_window_set_default_icon_name (PACKAGE_TARNAME);

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
