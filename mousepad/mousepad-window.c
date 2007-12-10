/* $Id$ */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib/gstdio.h>

#include <mousepad/mousepad-private.h>
#include <mousepad/mousepad-application.h>
#include <mousepad/mousepad-marshal.h>
#include <mousepad/mousepad-document.h>
#include <mousepad/mousepad-dialogs.h>
#include <mousepad/mousepad-preferences.h>
#include <mousepad/mousepad-replace-dialog.h>
#include <mousepad/mousepad-encoding-dialog.h>
#include <mousepad/mousepad-search-bar.h>
#include <mousepad/mousepad-statusbar.h>
#include <mousepad/mousepad-print.h>
#include <mousepad/mousepad-window.h>
#include <mousepad/mousepad-window-ui.h>



#define PADDING                   (2)
#define PASTE_HISTORY_MENU_LENGTH (30)

#if GTK_CHECK_VERSION (2,12,0)
static gpointer NOTEBOOK_GROUP = "Mousepad";
#endif



enum
{
  NEW_WINDOW,
  NEW_WINDOW_WITH_DOCUMENT,
  HAS_DOCUMENTS,
  LAST_SIGNAL,
};



/* class functions */
static void              mousepad_window_class_init                   (MousepadWindowClass    *klass);
static void              mousepad_window_init                         (MousepadWindow         *window);
static void              mousepad_window_dispose                      (GObject                *object);
static void              mousepad_window_finalize                     (GObject                *object);
static gboolean          mousepad_window_configure_event              (GtkWidget              *widget,
                                                                       GdkEventConfigure      *event);

/* statusbar tooltips */
static void              mousepad_window_connect_proxy                (GtkUIManager           *manager,
                                                                       GtkAction              *action,
                                                                       GtkWidget              *proxy,
                                                                       MousepadWindow         *window);
static void              mousepad_window_disconnect_proxy             (GtkUIManager           *manager,
                                                                       GtkAction              *action,
                                                                       GtkWidget              *proxy,
                                                                       MousepadWindow         *window);
static void              mousepad_window_menu_item_selected           (GtkWidget              *menu_item,
                                                                       MousepadWindow         *window);
static void              mousepad_window_menu_item_deselected         (GtkWidget              *menu_item,
                                                                       MousepadWindow         *window);

/* save windows geometry */
static gboolean          mousepad_window_save_geometry_timer          (gpointer                user_data);
static void              mousepad_window_save_geometry_timer_destroy  (gpointer                user_data);

/* window functions */
static gboolean          mousepad_window_open_file                    (MousepadWindow         *window,
                                                                       const gchar            *filename,
                                                                       const gchar            *encoding);
static gboolean          mousepad_window_close_document               (MousepadWindow         *window,
                                                                       MousepadDocument       *document);
static void              mousepad_window_set_title                    (MousepadWindow         *window);

/* notebook signals */
static void              mousepad_window_notebook_switch_page         (GtkNotebook            *notebook,
                                                                       GtkNotebookPage        *page,
                                                                       guint                   page_num,
                                                                       MousepadWindow         *window);
static void              mousepad_window_notebook_reordered           (GtkNotebook            *notebook,
                                                                       GtkWidget              *page,
                                                                       guint                   page_num,
                                                                       MousepadWindow         *window);
static void              mousepad_window_notebook_added               (GtkNotebook            *notebook,
                                                                       GtkWidget              *page,
                                                                       guint                   page_num,
                                                                       MousepadWindow         *window);
static void              mousepad_window_notebook_removed             (GtkNotebook            *notebook,
                                                                       GtkWidget              *page,
                                                                       guint                   page_num,
                                                                       MousepadWindow         *window);
static void              mousepad_window_notebook_menu_position       (GtkMenu                *menu,
                                                                       gint                   *x,
                                                                       gint                   *y,
                                                                       gboolean               *push_in,
                                                                       gpointer                user_data);
static gboolean          mousepad_window_notebook_button_release_event (GtkNotebook           *notebook,
                                                                        GdkEventButton        *event,
                                                                        MousepadWindow        *window);
static gboolean          mousepad_window_notebook_button_press_event  (GtkNotebook            *notebook,
                                                                       GdkEventButton         *event,
                                                                       MousepadWindow         *window);
#if GTK_CHECK_VERSION (2,12,0)
static GtkNotebook      *mousepad_window_notebook_create_window       (GtkNotebook            *notebook,
                                                                       GtkWidget              *page,
                                                                       gint                    x,
                                                                       gint                    y,
                                                                       MousepadWindow         *window);
#endif

/* document signals */
static void              mousepad_window_modified_changed             (MousepadWindow         *window);
static void              mousepad_window_cursor_changed               (MousepadDocument       *document,
                                                                       gint                    line,
                                                                       gint                    column,
                                                                       gint                    selection,
                                                                       MousepadWindow         *window);
static void              mousepad_window_selection_changed            (MousepadDocument       *document,
                                                                       gint                    selection,
                                                                       MousepadWindow         *window);
static void              mousepad_window_overwrite_changed            (MousepadDocument       *document,
                                                                       gboolean                overwrite,
                                                                       MousepadWindow         *window);
static void              mousepad_window_can_undo                     (MousepadWindow         *window,
                                                                       gboolean                can_undo);
static void              mousepad_window_can_redo                     (MousepadWindow         *window,
                                                                       gboolean                can_redo);

/* menu functions */
static void              mousepad_window_menu_templates_fill          (MousepadWindow         *window,
                                                                       GtkWidget              *menu,
                                                                       const gchar            *path);
static void              mousepad_window_menu_templates               (GtkWidget              *item,
                                                                       MousepadWindow         *window);
static void              mousepad_window_menu_tab_sizes               (MousepadWindow         *window);
static void              mousepad_window_menu_tab_sizes_update        (MousepadWindow         *window);
static void              mousepad_window_menu_textview_deactivate     (GtkWidget              *menu,
                                                                       GtkTextView            *textview);
static void              mousepad_window_menu_textview_popup          (GtkTextView            *textview,
                                                                       GtkMenu                *old_menu,
                                                                       MousepadWindow         *window);
static void              mousepad_window_update_actions               (MousepadWindow         *window);
static gboolean          mousepad_window_update_gomenu_idle           (gpointer                user_data);
static void              mousepad_window_update_gomenu_idle_destroy   (gpointer                user_data);
static void              mousepad_window_update_gomenu                (MousepadWindow         *window);

/* recent functions */
static void              mousepad_window_recent_add                   (MousepadWindow         *window,
                                                                       MousepadFile           *file);
static gint              mousepad_window_recent_sort                  (GtkRecentInfo          *a,
                                                                       GtkRecentInfo          *b);
static void              mousepad_window_recent_manager_init          (MousepadWindow         *window);
static gboolean          mousepad_window_recent_menu_idle             (gpointer                user_data);
static void              mousepad_window_recent_menu_idle_destroy     (gpointer                user_data);
static void              mousepad_window_recent_menu                  (MousepadWindow         *window);
static void              mousepad_window_recent_clear                 (MousepadWindow         *window);

/* dnd */
static void              mousepad_window_drag_data_received           (GtkWidget              *widget,
                                                                       GdkDragContext         *context,
                                                                       gint                    x,
                                                                       gint                    y,
                                                                       GtkSelectionData       *selection_data,
                                                                       guint                   info,
                                                                       guint                   time,
                                                                       MousepadWindow         *window);

/* search bar */
static void              mousepad_window_hide_search_bar              (MousepadWindow         *window);

/* history clipboard functions */
static void              mousepad_window_paste_history_add            (MousepadWindow         *window);
static void              mousepad_window_paste_history_menu_position  (GtkMenu                *menu,
                                                                       gint                   *x,
                                                                       gint                   *y,
                                                                       gboolean               *push_in,
                                                                       gpointer                user_data);
static void              mousepad_window_paste_history_activate       (GtkMenuItem            *item,
                                                                       MousepadWindow         *window);
static GtkWidget        *mousepad_window_paste_history_menu_item      (const gchar            *text,
                                                                       const gchar            *mnemonic);
static GtkWidget        *mousepad_window_paste_history_menu           (MousepadWindow         *window);

/* miscellaneous actions */
static void              mousepad_window_button_close_tab             (MousepadDocument       *document,
                                                                       MousepadWindow         *window);
static gboolean          mousepad_window_delete_event                 (MousepadWindow         *window,
                                                                       GdkEvent               *event);

/* actions */
static void              mousepad_window_action_new                   (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_new_window            (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_new_from_template     (GtkMenuItem            *item,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_open                  (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_open_recent           (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_clear_recent          (GtkAction              *action,
                                                                       MousepadWindow         *window);
static gboolean          mousepad_window_action_save                  (GtkAction              *action,
                                                                       MousepadWindow         *window);
static gboolean          mousepad_window_action_save_as               (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_save_all              (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_revert                (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_print                 (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_detach                (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_close                 (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_close_window          (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_undo                  (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_redo                  (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_cut                   (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_copy                  (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_paste                 (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_paste_history         (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_paste_column          (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_delete                (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_select_all            (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_change_selection      (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_find                  (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_find_next             (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_find_previous         (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_replace_destroy       (MousepadWindow         *window);
static void              mousepad_window_action_replace               (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_select_font           (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_statusbar_overwrite   (MousepadWindow         *window,
                                                                       gboolean                overwrite);
static void              mousepad_window_action_statusbar             (GtkToggleAction        *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_lowercase             (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_uppercase             (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_titlecase             (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_opposite_case         (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_tabs_to_spaces        (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_spaces_to_tabs        (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_strip_trailing_spaces (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_transpose             (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_move_line_up          (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_move_line_down        (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_duplicate             (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_increase_indent       (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_decrease_indent       (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_line_numbers          (GtkToggleAction        *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_word_wrap             (GtkToggleAction        *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_auto_indent           (GtkToggleAction        *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_tab_size              (GtkToggleAction        *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_insert_spaces         (GtkToggleAction        *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_line_ending           (GtkRadioAction         *action,
                                                                       GtkRadioAction         *current,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_prev_tab              (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_next_tab              (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_go_to_tab             (GtkRadioAction         *action,
                                                                       GtkNotebook            *notebook);
static void              mousepad_window_action_go_to_position        (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_contents              (GtkAction              *action,
                                                                       MousepadWindow         *window);
static void              mousepad_window_action_about                 (GtkAction              *action,
                                                                       MousepadWindow         *window);



struct _MousepadWindowClass
{
  GtkWindowClass __parent__;
};

struct _MousepadWindow
{
  GtkWindow __parent__;

  /* mousepad preferences */
  MousepadPreferences *preferences;

  /* the current active document */
  MousepadDocument    *active;

  /* closures for the menu callbacks */
  GClosure            *menu_item_selected_closure;
  GClosure            *menu_item_deselected_closure;

  /* action group */
  GtkActionGroup      *action_group;

  /* recent manager */
  GtkRecentManager    *recent_manager;

  /* UI manager */
  GtkUIManager        *ui_manager;
  guint                gomenu_merge_id;
  guint                recent_merge_id;

  /* main window widgets */
  GtkWidget           *box;
  GtkWidget           *notebook;
  GtkWidget           *search_bar;
  GtkWidget           *statusbar;
  GtkWidget           *replace_dialog;

  /* support to remember window geometry */
  guint                save_geometry_timer_id;

  /* idle update functions for the recent and go menu */
  guint                update_recent_menu_id;
  guint                update_go_menu_id;
};



static const GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_("_File"), NULL, NULL, NULL, },
    { "new", GTK_STOCK_NEW, N_("_New"), "<control>N", N_("Create a new document"), G_CALLBACK (mousepad_window_action_new), },
    { "new-window", NULL, N_("New _Window"), "<shift><control>N", N_("Create a new document in a new window"), G_CALLBACK (mousepad_window_action_new_window), },
    { "template-menu", NULL, N_("New From Te_mplate"), NULL, NULL, NULL, },
    { "open", GTK_STOCK_OPEN, N_("_Open..."), NULL, N_("Open a file"), G_CALLBACK (mousepad_window_action_open), },
    { "recent-menu", NULL, N_("Op_en Recent"), NULL, NULL, NULL, },
      { "no-recent-items", NULL, N_("No items found"), NULL, NULL, NULL, },
      { "clear-recent", GTK_STOCK_CLEAR, N_("Clear _History"), NULL, N_("Clear the recently used files history"), G_CALLBACK (mousepad_window_action_clear_recent), },
    { "save", GTK_STOCK_SAVE, NULL, "<control>S", N_("Save the current document"), G_CALLBACK (mousepad_window_action_save), },
    { "save-as", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<shift><control>S", N_("Save current document as another file"), G_CALLBACK (mousepad_window_action_save_as), },
    { "save-all", NULL, N_("Save A_ll"), NULL, N_("Save all document in this window"), G_CALLBACK (mousepad_window_action_save_all), },
    { "revert", GTK_STOCK_REVERT_TO_SAVED, N_("Re_vert"), NULL, N_("Revert to the saved version of the file"), G_CALLBACK (mousepad_window_action_revert), },
    { "print", GTK_STOCK_PRINT, N_("_Print..."), "<control>P", N_("Prin the current document"), G_CALLBACK (mousepad_window_action_print), },
    { "detach", NULL, N_("_Detach Tab"), "<control>D", N_("Move the current document to a new window"), G_CALLBACK (mousepad_window_action_detach), },
    { "close", GTK_STOCK_CLOSE, N_("Close _Tab"), "<control>W", N_("Close the current document"), G_CALLBACK (mousepad_window_action_close), },
    { "close-window", GTK_STOCK_QUIT, N_("_Close Window"), "<control>Q", N_("Close this window"), G_CALLBACK (mousepad_window_action_close_window), },

  { "edit-menu", NULL, N_("_Edit"), NULL, NULL, NULL, },
    { "undo", GTK_STOCK_UNDO, NULL, "<control>Z", N_("Undo the last action"), G_CALLBACK (mousepad_window_action_undo), },
    { "redo", GTK_STOCK_REDO, NULL, "<control>Y", N_("Redo the last undone action"), G_CALLBACK (mousepad_window_action_redo), },
    { "cut", GTK_STOCK_CUT, NULL, NULL, N_("Cut the selection"), G_CALLBACK (mousepad_window_action_cut), },
    { "copy", GTK_STOCK_COPY, NULL, NULL, N_("Copy the selection"), G_CALLBACK (mousepad_window_action_copy), },
    { "paste", GTK_STOCK_PASTE, NULL, NULL, N_("Paste the clipboard"), G_CALLBACK (mousepad_window_action_paste), },
    { "paste-menu", NULL, N_("Paste _Special"), NULL, NULL, NULL, },
      { "paste-history", NULL, N_("Paste from _History"), NULL, N_("Paste from the clipboard history"), G_CALLBACK (mousepad_window_action_paste_history), },
      { "paste-column", NULL, N_("Paste as _Column"), NULL, N_("Paste the clipboard text into a column"), G_CALLBACK (mousepad_window_action_paste_column), },
    { "delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete the current selection"), G_CALLBACK (mousepad_window_action_delete), },
    { "select-all", GTK_STOCK_SELECT_ALL, NULL, NULL, N_("Select the text in the entire document"), G_CALLBACK (mousepad_window_action_select_all), },
    { "change-selection", NULL, N_("Change the selection"), NULL, N_("Change a normal selection into a column selection and vice versa"), G_CALLBACK (mousepad_window_action_change_selection), },
    { "find", GTK_STOCK_FIND, NULL, NULL, N_("Search for text"), G_CALLBACK (mousepad_window_action_find), },
    { "find-next", NULL, N_("Find _Next"), NULL, N_("Search forwards for the same text"), G_CALLBACK (mousepad_window_action_find_next), },
    { "find-previous", NULL, N_("Find _Previous"), NULL, N_("Search backwards for the same text"), G_CALLBACK (mousepad_window_action_find_previous), },
    { "replace", GTK_STOCK_FIND_AND_REPLACE, N_("Find and Rep_lace..."), NULL, N_("Search for and replace text"), G_CALLBACK (mousepad_window_action_replace), },

  { "view-menu", NULL, N_("_View"), NULL, NULL, NULL, },
    { "font", GTK_STOCK_SELECT_FONT, N_("Select F_ont..."), NULL, N_("Change the editor font"), G_CALLBACK (mousepad_window_action_select_font), },

  { "text-menu", NULL, N_("_Text"), NULL, NULL, NULL, },
    { "convert-menu", NULL, N_("_Convert"), NULL, NULL, NULL, },
      { "uppercase", NULL, N_("to _Uppercase"), NULL, N_("Change the case of the selection to uppercase"), G_CALLBACK (mousepad_window_action_uppercase), },
      { "lowercase", NULL, N_("to _Lowercase"), NULL, N_("Change the case of the selection to lowercase"), G_CALLBACK (mousepad_window_action_lowercase), },
      { "titlecase", NULL, N_("to _Title Case"), NULL, N_("Change the case of the selection to title case"), G_CALLBACK (mousepad_window_action_titlecase), },
      { "opposite-case", NULL, N_("to _Opposite Case"), NULL, N_("Change the case of the selection opposite case"), G_CALLBACK (mousepad_window_action_opposite_case), },
      { "tabs-to-spaces", NULL, N_("_Tabs to Spaces"), NULL, N_("Convert all tabs to spaces in the selection or document"), G_CALLBACK (mousepad_window_action_tabs_to_spaces), },
      { "spaces-to-tabs", NULL, N_("_Spaces to Tabs"), NULL, N_("Convert all the leading spaces to tabs in the selected line(s) or document"), G_CALLBACK (mousepad_window_action_spaces_to_tabs), },
      { "strip-trailing", NULL, N_("St_rip Trailing Spaces"), NULL, N_("Remove all the trailing spaces from the selected line(s) or document"), G_CALLBACK (mousepad_window_action_strip_trailing_spaces), },
      { "transpose", NULL, N_("_Transpose"), "<control>T", N_("Reverse the order of something"), G_CALLBACK (mousepad_window_action_transpose), },
    { "move-menu", NULL, N_("_Move Selection"), NULL, NULL, NULL, },
      { "line-up", NULL, N_("Line _Up"), NULL, N_("Move the selection one line up"), G_CALLBACK (mousepad_window_action_move_line_up), },
      { "line-down", NULL, N_("Line _Down"), NULL, N_("Move the selection one line down"), G_CALLBACK (mousepad_window_action_move_line_down), },
    { "duplicate", NULL, N_("D_uplicate Line / Selection"), NULL, N_("Duplicate the current line or selection"), G_CALLBACK (mousepad_window_action_duplicate), },
    { "increase-indent", GTK_STOCK_INDENT, N_("_Increase Indent"), NULL, N_("Increase indent of selection or line"), G_CALLBACK (mousepad_window_action_increase_indent), },
    { "decrease-indent", GTK_STOCK_UNINDENT, N_("_Decrease Indent"), NULL, N_("Decrease indent of selection or line"), G_CALLBACK (mousepad_window_action_decrease_indent), },

  { "document-menu", NULL, N_("_Document"), NULL, NULL, NULL, },
    { "tab-size-menu", NULL, N_("Tab _Size"), NULL, NULL, NULL, },
    { "eol-menu", NULL, N_("Line E_nding"), NULL, NULL, NULL, },

  { "navigation-menu", NULL, N_("_Navigation"), NULL, },
    { "back", GTK_STOCK_GO_BACK, N_("_Previous Tab"), "<control>Page_Up", N_("Select the previous tab"), G_CALLBACK (mousepad_window_action_prev_tab), },
    { "forward", GTK_STOCK_GO_FORWARD, N_("_Next Tab"), "<control>Page_Down", N_("Select the next tab"), G_CALLBACK (mousepad_window_action_next_tab), },
    { "go-to", GTK_STOCK_JUMP_TO, N_("_Go to..."), "<control>G", N_("Go to a specific location in the document"), G_CALLBACK (mousepad_window_action_go_to_position), },

  { "help-menu", NULL, N_("_Help"), NULL, },
    { "contents", GTK_STOCK_HELP, N_ ("_Contents"), "F1", N_("Display the Mousepad user manual"), G_CALLBACK (mousepad_window_action_contents), },
    { "about", GTK_STOCK_ABOUT, NULL, NULL, N_("About this application"), G_CALLBACK (mousepad_window_action_about), },
};

static const GtkToggleActionEntry toggle_action_entries[] =
{
  { "statusbar", NULL, N_("St_atusbar"), NULL, N_("Change the visibility of the statusbar"), G_CALLBACK (mousepad_window_action_statusbar), FALSE, },
  { "line-numbers", NULL, N_("Line N_umbers"), NULL, N_("Show line numbers"), G_CALLBACK (mousepad_window_action_line_numbers), FALSE, },
  { "auto-indent", NULL, N_("_Auto Indent"), NULL, N_("Auto indent a new line"), G_CALLBACK (mousepad_window_action_auto_indent), FALSE, },
  { "word-wrap", NULL, N_("_Word Wrap"), NULL, N_("Toggle breaking lines in between words"), G_CALLBACK (mousepad_window_action_word_wrap), FALSE, },
  { "insert-spaces", NULL, N_("Insert _Spaces"), NULL, N_("Insert spaces when the tab button is pressed"), G_CALLBACK (mousepad_window_action_insert_spaces), FALSE, },
};

static const GtkRadioActionEntry radio_action_entries[] =
{
  { "unix", NULL, N_("Unix (_LF)"), NULL, N_("Set the line ending of the document to Unix (LF)"), MOUSEPAD_LINE_END_UNIX, },
  { "mac", NULL, N_("Mac (_CR)"), NULL, N_("Set the line ending of the document to Mac (CR)"), MOUSEPAD_LINE_END_MAC, },
  { "dos", NULL, N_("DOS / Windows (C_R LF)"), NULL, N_("Set the line ending of the document to DOS / Windows (CR LF)"), MOUSEPAD_LINE_END_DOS, },
};



/* global variables */
static GObjectClass *mousepad_window_parent_class;
static guint         window_signals[LAST_SIGNAL];
static gint          lock_menu_updates = 0;
static GSList       *clipboard_history = NULL;
static guint         clipboard_history_ref_count = 0;



GtkWidget *
mousepad_window_new (void)
{
  return g_object_new (MOUSEPAD_TYPE_WINDOW, NULL);
}



GType
mousepad_window_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (GTK_TYPE_WINDOW,
                                            I_("MousepadWindow"),
                                            sizeof (MousepadWindowClass),
                                            (GClassInitFunc) mousepad_window_class_init,
                                            sizeof (MousepadWindow),
                                            (GInstanceInitFunc) mousepad_window_init,
                                            0);
    }

  return type;
}



static void
mousepad_window_class_init (MousepadWindowClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  mousepad_window_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = mousepad_window_dispose;
  gobject_class->finalize = mousepad_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->configure_event = mousepad_window_configure_event;

  window_signals[NEW_WINDOW] =
    g_signal_new (I_("new-window"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_NO_HOOKS,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  window_signals[NEW_WINDOW_WITH_DOCUMENT] =
    g_signal_new (I_("new-window-with-document"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_NO_HOOKS,
                  0, NULL, NULL,
                  _mousepad_marshal_VOID__OBJECT_INT_INT,
                  G_TYPE_NONE, 3,
                  G_TYPE_OBJECT,
                  G_TYPE_INT, G_TYPE_INT);

  window_signals[HAS_DOCUMENTS] =
    g_signal_new (I_("has-documents"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_NO_HOOKS,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);
}



static void
mousepad_window_init (MousepadWindow *window)
{
  GtkAccelGroup *accel_group;
  GtkWidget     *menubar;
  GtkWidget     *label;
  GtkWidget     *separator;
  GtkWidget     *ebox;
  GtkWidget     *item;
  GtkAction     *action;
  gint           width, height;
  gboolean       statusbar_visible;

  /* initialize stuff */
  window->save_geometry_timer_id = 0;
  window->update_recent_menu_id = 0;
  window->update_go_menu_id = 0;
  window->gomenu_merge_id = 0;
  window->recent_merge_id = 0;
  window->search_bar = NULL;
  window->statusbar = NULL;
  window->replace_dialog = NULL;
  window->active = NULL;
  window->recent_manager = NULL;

  /* increase clipboard history ref count */
  clipboard_history_ref_count++;

  /* add the preferences to the window */
  window->preferences = mousepad_preferences_get ();

  /* allocate a closure for the menu_item_selected() callback */
  window->menu_item_selected_closure = g_cclosure_new_object (G_CALLBACK (mousepad_window_menu_item_selected), G_OBJECT (window));
  g_closure_ref (window->menu_item_selected_closure);
  g_closure_sink (window->menu_item_selected_closure);

  /* signal for handling the window delete event */
  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (mousepad_window_delete_event), NULL);

  /* allocate a closure for the menu_item_deselected() callback */
  window->menu_item_deselected_closure = g_cclosure_new_object (G_CALLBACK (mousepad_window_menu_item_deselected), G_OBJECT (window));
  g_closure_ref (window->menu_item_deselected_closure);
  g_closure_sink (window->menu_item_deselected_closure);

  /* read settings from the preferences */
  g_object_get (G_OBJECT (window->preferences),
                "window-width", &width,
                "window-height", &height,
                "window-statusbar-visible", &statusbar_visible,
                NULL);

  /* set the default window size */
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  /* the action group for this window */
  window->action_group = gtk_action_group_new ("MousepadWindow");
  gtk_action_group_set_translation_domain (window->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (window->action_group, action_entries, G_N_ELEMENTS (action_entries), GTK_WIDGET (window));
  gtk_action_group_add_toggle_actions (window->action_group, toggle_action_entries, G_N_ELEMENTS (toggle_action_entries), GTK_WIDGET (window));
  gtk_action_group_add_radio_actions (window->action_group, radio_action_entries, G_N_ELEMENTS (radio_action_entries), -1, G_CALLBACK (mousepad_window_action_line_ending), GTK_WIDGET (window));

  /* create the ui manager and connect proxy signals for the statusbar */
  window->ui_manager = gtk_ui_manager_new ();
  g_signal_connect (G_OBJECT (window->ui_manager), "connect-proxy", G_CALLBACK (mousepad_window_connect_proxy), window);
  g_signal_connect (G_OBJECT (window->ui_manager), "disconnect-proxy", G_CALLBACK (mousepad_window_disconnect_proxy), window);
  gtk_ui_manager_insert_action_group (window->ui_manager, window->action_group, 0);
  gtk_ui_manager_add_ui_from_string (window->ui_manager, mousepad_window_ui, mousepad_window_ui_length, NULL);

  /* build the templates menu when the item is shown for the first time */
  /* from here we also trigger the idle build of the recent menu */
  item = gtk_ui_manager_get_widget (window->ui_manager, "/main-menu/file-menu/template-menu");
  g_signal_connect (G_OBJECT (item), "map", G_CALLBACK (mousepad_window_menu_templates), window);

  /* add tab size menu */
  mousepad_window_menu_tab_sizes (window);

  /* set accel group for the window */
  accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  /* create the main table */
  window->box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), window->box);
  gtk_widget_show (window->box);

  menubar = gtk_ui_manager_get_widget (window->ui_manager, "/main-menu");
  gtk_box_pack_start (GTK_BOX (window->box), menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);

  /* check if we need to add the root warning */
  if (G_UNLIKELY (geteuid () == 0))
    {
      /* install default settings for the root warning text box */
      gtk_rc_parse_string ("style\"mousepad-window-root-style\"\n"
                             "{\n"
                               "bg[NORMAL]=\"#b4254b\"\n"
                               "fg[NORMAL]=\"#fefefe\"\n"
                             "}\n"
                           "widget\"MousepadWindow.*.root-warning\"style\"mousepad-window-root-style\"\n"
                           "widget\"MousepadWindow.*.root-warning.GtkLabel\"style\"mousepad-window-root-style\"\n");

      /* add the box for the root warning */
      ebox = gtk_event_box_new ();
      gtk_widget_set_name (ebox, "root-warning");
      gtk_box_pack_start (GTK_BOX (window->box), ebox, FALSE, FALSE, 0);
      gtk_widget_show (ebox);

      /* add the label with the root warning */
      label = gtk_label_new (_("Warning, you are using the root account, you may harm your system."));
      gtk_misc_set_padding (GTK_MISC (label), 6, 3);
      gtk_container_add (GTK_CONTAINER (ebox), label);
      gtk_widget_show (label);

      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (window->box), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
    }

  /* create the notebook */
  window->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                   "homogeneous", FALSE,
                                   "scrollable", TRUE,
                                   "show-border", FALSE,
                                   "show-tabs", FALSE,
                                   "tab-hborder", 0,
                                   "tab-vborder", 0,
                                   NULL);

  /* set the group id */
#if GTK_CHECK_VERSION (2,12,0)
  gtk_notebook_set_group (GTK_NOTEBOOK (window->notebook), NOTEBOOK_GROUP);
#else
  gtk_notebook_set_group_id (GTK_NOTEBOOK (window->notebook), 1337);
#endif

  /* connect signals to the notebooks */
  g_signal_connect (G_OBJECT (window->notebook), "switch-page", G_CALLBACK (mousepad_window_notebook_switch_page), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-reordered", G_CALLBACK (mousepad_window_notebook_reordered), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-added", G_CALLBACK (mousepad_window_notebook_added), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-removed", G_CALLBACK (mousepad_window_notebook_removed), window);
  g_signal_connect (G_OBJECT (window->notebook), "button-press-event", G_CALLBACK (mousepad_window_notebook_button_press_event), window);
  g_signal_connect (G_OBJECT (window->notebook), "button-release-event", G_CALLBACK (mousepad_window_notebook_button_release_event), window);
#if GTK_CHECK_VERSION (2,12,0)
  g_signal_connect (G_OBJECT (window->notebook), "create-window", G_CALLBACK (mousepad_window_notebook_create_window), window);
#endif

  /* append and show the notebook */
  gtk_box_pack_start (GTK_BOX (window->box), window->notebook, TRUE, TRUE, PADDING);
  gtk_widget_show (window->notebook);

  /* check if we should display the statusbar by default */
  action = gtk_action_group_get_action (window->action_group, "statusbar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), statusbar_visible);

  /* allow drops in the window */
  gtk_drag_dest_set (GTK_WIDGET (window), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, drop_targets, G_N_ELEMENTS (drop_targets), GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (window), "drag-data-received", G_CALLBACK (mousepad_window_drag_data_received), window);

}



static void
mousepad_window_dispose (GObject *object)
{
  MousepadWindow *window = MOUSEPAD_WINDOW (object);

  /* disconnect recent manager signal */
  if (G_LIKELY (window->recent_manager))
    g_signal_handlers_disconnect_by_func (G_OBJECT (window->recent_manager), mousepad_window_recent_menu, window);

  /* destroy the save geometry timer source */
  if (G_UNLIKELY (window->save_geometry_timer_id != 0))
    g_source_remove (window->save_geometry_timer_id);

  (*G_OBJECT_CLASS (mousepad_window_parent_class)->dispose) (object);
}



static void
mousepad_window_finalize (GObject *object)
{
  MousepadWindow *window = MOUSEPAD_WINDOW (object);

  /* decrease history clipboard ref count */
  clipboard_history_ref_count--;

  /* cancel a scheduled recent menu update */
  if (G_UNLIKELY (window->update_recent_menu_id != 0))
    g_source_remove (window->update_recent_menu_id);

  /* cancel a scheduled go menu update */
  if (G_UNLIKELY (window->update_go_menu_id != 0))
    g_source_remove (window->update_go_menu_id);

  /* drop our references on the menu_item_selected()/menu_item_deselected() closures */
  g_closure_unref (window->menu_item_deselected_closure);
  g_closure_unref (window->menu_item_selected_closure);

  /* release the ui manager */
  g_signal_handlers_disconnect_matched (G_OBJECT (window->ui_manager), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, window);
  g_object_unref (G_OBJECT (window->ui_manager));

  /* release the action group */
  g_object_unref (G_OBJECT (window->action_group));

  /* release the preferences reference */
  g_object_unref (G_OBJECT (window->preferences));

  /* free clipboard history if needed */
  if (clipboard_history_ref_count == 0 && clipboard_history != NULL)
    {
      g_slist_foreach (clipboard_history, (GFunc) g_free, NULL);
      g_slist_free (clipboard_history);
    }

  (*G_OBJECT_CLASS (mousepad_window_parent_class)->finalize) (object);
}



static gboolean
mousepad_window_configure_event (GtkWidget         *widget,
                                 GdkEventConfigure *event)
{
  MousepadWindow *window = MOUSEPAD_WINDOW (widget);

  /* check if we have a new dimension here */
  if (widget->allocation.width != event->width || widget->allocation.height != event->height)
    {
      /* drop any previous timer source */
      if (window->save_geometry_timer_id > 0)
        g_source_remove (window->save_geometry_timer_id);

      /* check if we should schedule another save timer */
      if (GTK_WIDGET_VISIBLE (widget))
        {
          /* save the geometry one second after the last configure event */
          window->save_geometry_timer_id = g_timeout_add_full (G_PRIORITY_LOW, 1000, mousepad_window_save_geometry_timer,
                                                               window, mousepad_window_save_geometry_timer_destroy);
        }
    }

  /* let gtk+ handle the configure event */
  return (*GTK_WIDGET_CLASS (mousepad_window_parent_class)->configure_event) (widget, event);
}



/**
 * Statusbar Tooltip Functions
 **/
static void
mousepad_window_connect_proxy (GtkUIManager   *manager,
                               GtkAction      *action,
                               GtkWidget      *proxy,
                               MousepadWindow *window)
{
  _mousepad_return_if_fail (GTK_IS_ACTION (action));
  _mousepad_return_if_fail (GTK_IS_MENU_ITEM (proxy));
  _mousepad_return_if_fail (GTK_IS_UI_MANAGER (manager));

  /* we want to get informed when the user hovers a menu item */
  g_signal_connect_closure (G_OBJECT (proxy), "select", window->menu_item_selected_closure, FALSE);
  g_signal_connect_closure (G_OBJECT (proxy), "deselect", window->menu_item_deselected_closure, FALSE);
}



static void
mousepad_window_disconnect_proxy (GtkUIManager   *manager,
                                  GtkAction      *action,
                                  GtkWidget      *proxy,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (GTK_IS_ACTION (action));
  _mousepad_return_if_fail (GTK_IS_MENU_ITEM (proxy));
  _mousepad_return_if_fail (GTK_IS_UI_MANAGER (manager));

  /* disconnect the signal from mousepad_window_connect_proxy() */
  g_signal_handlers_disconnect_matched (G_OBJECT (proxy), G_SIGNAL_MATCH_CLOSURE, 0, 0, window->menu_item_selected_closure, NULL, NULL);
  g_signal_handlers_disconnect_matched (G_OBJECT (proxy), G_SIGNAL_MATCH_CLOSURE, 0, 0, window->menu_item_deselected_closure, NULL, NULL);
}



static void
mousepad_window_menu_item_selected (GtkWidget      *menu_item,
                                    MousepadWindow *window)
{
  GtkAction *action;
  gchar     *tooltip;
  gint       id;

  /* we can only display tooltips if we have a statusbar */
  if (G_LIKELY (window->statusbar != NULL))
    {
      /* get the action from the menu item */
      action = gtk_widget_get_action (menu_item);
      if (G_LIKELY (action))
        {
          /* read the tooltip from the action, if there is one */
          g_object_get (G_OBJECT (action), "tooltip", &tooltip, NULL);

          if (G_LIKELY (tooltip != NULL))
            {
              /* show the tooltip */
              id = gtk_statusbar_get_context_id (GTK_STATUSBAR (window->statusbar), "tooltip");
              gtk_statusbar_push (GTK_STATUSBAR (window->statusbar), id, tooltip);

              /* cleanup */
              g_free (tooltip);
            }
        }
    }
}



static void
mousepad_window_menu_item_deselected (GtkWidget      *menu_item,
                                      MousepadWindow *window)
{
  gint id;

  /* we can only undisplay tooltips if we have a statusbar */
  if (G_LIKELY (window->statusbar != NULL))
    {
      /* drop the last tooltip from the statusbar */
      id = gtk_statusbar_get_context_id (GTK_STATUSBAR (window->statusbar), "tooltip");
      gtk_statusbar_pop (GTK_STATUSBAR (window->statusbar), id);
    }
}



/**
 * Save Geometry Functions
 **/
static gboolean
mousepad_window_save_geometry_timer (gpointer user_data)
{
  GdkWindowState   state;
  MousepadWindow  *window = MOUSEPAD_WINDOW (user_data);
  gboolean         remember_geometry;
  gint             width;
  gint             height;

  GDK_THREADS_ENTER ();

  /* check if we should remember the window geometry */
  g_object_get (G_OBJECT (window->preferences), "misc-remember-geometry", &remember_geometry, NULL);
  if (G_LIKELY (remember_geometry))
    {
      /* check if the window is still visible */
      if (GTK_WIDGET_VISIBLE (window))
        {
          /* determine the current state of the window */
          state = gdk_window_get_state (GTK_WIDGET (window)->window);

          /* don't save geometry for maximized or fullscreen windows */
          if ((state & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)) == 0)
            {
              /* determine the current width/height of the window... */
              gtk_window_get_size (GTK_WINDOW (window), &width, &height);

              /* ...and remember them as default for new windows */
              g_object_set (G_OBJECT (window->preferences),
                            "window-width", width,
                            "window-height", height, NULL);
            }
        }
    }

  GDK_THREADS_LEAVE ();

  return FALSE;
}



static void
mousepad_window_save_geometry_timer_destroy (gpointer user_data)
{
  MOUSEPAD_WINDOW (user_data)->save_geometry_timer_id = 0;
}



/**
 * Mousepad Window Functions
 **/
static gboolean
mousepad_window_open_file (MousepadWindow *window,
                           const gchar    *filename,
                           const gchar    *encoding)
{
  MousepadDocument *document;
  GError           *error = NULL;
  gboolean          succeed = FALSE;
  gint              npages = 0, i;
  gint              response;
  const gchar      *new_encoding;
  const gchar      *opened_filename;
  GtkWidget        *dialog;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);
  _mousepad_return_val_if_fail (filename != NULL && *filename != '\0', FALSE);

  /* check if the file is already openend */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  for (i = 0; i < npages; i++)
    {
      document = MOUSEPAD_DOCUMENT (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i));

      /* debug check */
      _mousepad_return_val_if_fail (MOUSEPAD_IS_DOCUMENT (document), FALSE);

      if (G_LIKELY (document))
        {
          /* get the filename */
          opened_filename = mousepad_file_get_filename (MOUSEPAD_DOCUMENT (document)->file);

          /* see if the file is already opened */
          if (opened_filename && strcmp (filename, opened_filename) == 0)
            {
              /* switch to the tab */
              gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), i);

              /* and we're done */
              return TRUE;
            }
        }
    }

  /* new document */
  document = mousepad_document_new ();

  /* set the filename */
  mousepad_file_set_filename (document->file, filename);

  /* set the passed encoding */
  mousepad_file_set_encoding (document->file, encoding);

  try_open_again:

  /* lock the undo manager */
  mousepad_undo_lock (document->undo);

  /* read the content into the buffer */
  succeed = mousepad_file_open (document->file, &error);

  /* release the lock */
  mousepad_undo_unlock (document->undo);

  if (G_LIKELY (succeed))
    {
      /* add the document to the notebook and connect some signals */
      mousepad_window_add (window, document);

      /* add to the recent history */
      mousepad_window_recent_add (window, document->file);
    }
  else if (error->domain == G_CONVERT_ERROR)
    {
      /* clear the error */
      g_clear_error (&error);

      /* run the encoding dialog */
      dialog = mousepad_encoding_dialog_new (GTK_WINDOW (window), document->file);

      /* run the dialog */
      response = gtk_dialog_run (GTK_DIALOG (dialog));

      if (response == GTK_RESPONSE_OK)
        {
          /* get the selected encoding */
          new_encoding = mousepad_encoding_dialog_get_encoding (MOUSEPAD_ENCODING_DIALOG (dialog));

          /* set the document encoding */
          mousepad_file_set_encoding (document->file, new_encoding);
        }

      /* destroy the dialog */
      gtk_widget_destroy (dialog);

      /* handle */
      if (response == GTK_RESPONSE_OK)
        goto try_open_again;
      else
        goto opening_failed;
    }
  else
    {
      opening_failed:

      /* something went wrong, release the document */
      g_object_unref (G_OBJECT (document));

      if (G_LIKELY (error))
        {
          /* show the warning */
          mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to open file"));
          g_error_free (error);
        }
    }

  return succeed;
}



gboolean
mousepad_window_open_files (MousepadWindow  *window,
                            const gchar     *working_directory,
                            gchar          **filenames)
{
  guint  n;
  gchar *filename;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);
  _mousepad_return_val_if_fail (working_directory != NULL, FALSE);
  _mousepad_return_val_if_fail (filenames != NULL, FALSE);
  _mousepad_return_val_if_fail (*filenames != NULL, FALSE);

  /* block menu updates */
  lock_menu_updates++;

  /* walk through all the filenames */
  for (n = 0; filenames[n] != NULL; ++n)
    {
      /* check if the filename looks like an uri */
      if (strncmp (filenames[n], "file:", 5) == 0)
        {
          /* convert the uri to an absolute filename */
          filename = g_filename_from_uri (filenames[n], NULL, NULL);
        }
      else if (g_path_is_absolute (filenames[n]) == FALSE)
        {
          /* create an absolute file */
          filename = g_build_filename (working_directory, filenames[n], NULL);
        }
      else
        {
          /* looks like a valid filename */
          filename = NULL;
        }

      /* open a new tab with the file */
      mousepad_window_open_file (window, filename ? filename : filenames[n], NULL);

      /* cleanup */
      g_free (filename);
    }

  /* allow menu updates again */
  lock_menu_updates--;

  /* check if the window contains tabs */
  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)) == 0)
    return FALSE;

  /* update the menus */
  mousepad_window_recent_menu (window);
  mousepad_window_update_gomenu (window);

  return TRUE;
}



void
mousepad_window_add (MousepadWindow   *window,
                     MousepadDocument *document)
{
  GtkWidget        *label;
  gint              page;
  MousepadDocument *prev_active;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));
  _mousepad_return_if_fail (GTK_IS_NOTEBOOK (window->notebook));

  /* get the active tab before we switch to the new one */
  prev_active = window->active;

  /* create the tab label */
  label = mousepad_document_get_tab_label (document);

  /* get active page */
  page = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));

  /* insert the page right of the active tab */
  page = gtk_notebook_insert_page (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (document), label, page + 1);

  /* allow tab reordering and detaching */
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (document), TRUE);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (document), TRUE);

  /* show the document */
  gtk_widget_show (GTK_WIDGET (document));

  /* don't bother about this when there was no previous active page (startup) */
  if (G_LIKELY (prev_active != NULL))
    {
      /* switch to the new tab */
      gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page);

      /* destroy the previous tab if it was not modified, untitled and the new tab is not untitled */
      if (gtk_text_buffer_get_modified (prev_active->buffer) == FALSE
          && mousepad_file_get_filename (prev_active->file) == NULL
          && mousepad_file_get_filename (document->file) != NULL)
        gtk_widget_destroy (GTK_WIDGET (prev_active));
    }

  /* make sure the textview is focused in the new document */
  mousepad_document_focus_textview (document);
}



static gboolean
mousepad_window_close_document (MousepadWindow   *window,
                                MousepadDocument *document)
{
  gboolean succeed = FALSE;
  gint     response;
  gboolean readonly;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);
  _mousepad_return_val_if_fail (MOUSEPAD_IS_DOCUMENT (document), FALSE);

  /* check if the document has been modified */
  if (gtk_text_buffer_get_modified (document->buffer))
    {
      /* whether the file is readonly */
      readonly = mousepad_file_get_read_only (document->file);

      /* run save changes dialog */
      response = mousepad_dialogs_save_changes (GTK_WINDOW (window), readonly);

      switch (response)
        {
          case MOUSEPAD_RESPONSE_DONT_SAVE:
            /* don't save, only destroy the document */
            succeed = TRUE;
            break;

          case MOUSEPAD_RESPONSE_CANCEL:
            /* do nothing */
            break;

          case MOUSEPAD_RESPONSE_SAVE:
            succeed = mousepad_window_action_save (NULL, window);
            break;

          case MOUSEPAD_RESPONSE_SAVE_AS:
            succeed = mousepad_window_action_save_as (NULL, window);
            break;
        }
    }
  else
    {
      /* no changes in the document, safe to destroy it */
      succeed = TRUE;
    }

  /* destroy the document */
  if (succeed)
    gtk_widget_destroy (GTK_WIDGET (document));

  return succeed;
}



static void
mousepad_window_set_title (MousepadWindow *window)
{
  gchar            *string;
  const gchar      *title;
  gboolean          show_full_path;
  MousepadDocument *document = window->active;

  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* whether to show the full path */
  g_object_get (G_OBJECT (window->preferences), "misc-path-in-title", &show_full_path, NULL);

  /* name we display in the title */
  if (G_UNLIKELY (show_full_path && mousepad_document_get_filename (document)))
    title = mousepad_document_get_filename (document);
  else
    title = mousepad_document_get_basename (document);

  /* build the title */
  if (G_UNLIKELY (mousepad_file_get_read_only (document->file)))
    string = g_strdup_printf ("%s [%s] - %s", title, _("Read Only"), PACKAGE_NAME);
  else
    string = g_strdup_printf ("%s%s - %s", gtk_text_buffer_get_modified (document->buffer) ? "*" : "", title, PACKAGE_NAME);

  /* set the window title */
  gtk_window_set_title (GTK_WINDOW (window), string);

  /* cleanup */
  g_free (string);
}



/**
 * Notebook Signal Functions
 **/
static void
mousepad_window_notebook_switch_page (GtkNotebook     *notebook,
                                      GtkNotebookPage *page,
                                      guint            page_num,
                                      MousepadWindow  *window)
{
  MousepadDocument *document;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (GTK_IS_NOTEBOOK (notebook));

  /* get the new active document */
  document = MOUSEPAD_DOCUMENT (gtk_notebook_get_nth_page (notebook, page_num));

  /* only update when really changed */
  if (G_LIKELY (window->active != document))
    {
      /* set new active document */
      window->active = document;

      /* set the window title */
      mousepad_window_set_title (window);

      /* update the menu actions */
      mousepad_window_update_actions (window);

      /* update the statusbar */
      mousepad_document_send_signals (window->active);
    }
}



static void
mousepad_window_notebook_reordered (GtkNotebook     *notebook,
                                    GtkWidget       *page,
                                    guint            page_num,
                                    MousepadWindow  *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (page));

  /* update the go menu */
  mousepad_window_update_gomenu (window);
}



static void
mousepad_window_notebook_added (GtkNotebook     *notebook,
                                GtkWidget       *page,
                                guint            page_num,
                                MousepadWindow  *window)
{
  MousepadDocument *document = MOUSEPAD_DOCUMENT (page);
  gboolean          always_show_tabs;
  gint              npages;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));

  /* connect signals to the document for this window */
  g_signal_connect (G_OBJECT (page), "close-tab", G_CALLBACK (mousepad_window_button_close_tab), window);
  g_signal_connect (G_OBJECT (page), "cursor-changed", G_CALLBACK (mousepad_window_cursor_changed), window);
  g_signal_connect (G_OBJECT (page), "selection-changed", G_CALLBACK (mousepad_window_selection_changed), window);
  g_signal_connect (G_OBJECT (page), "overwrite-changed", G_CALLBACK (mousepad_window_overwrite_changed), window);
  g_signal_connect (G_OBJECT (page), "drag-data-received", G_CALLBACK (mousepad_window_drag_data_received), window);
  g_signal_connect_swapped (G_OBJECT (document->undo), "can-undo", G_CALLBACK (mousepad_window_can_undo), window);
  g_signal_connect_swapped (G_OBJECT (document->undo), "can-redo", G_CALLBACK (mousepad_window_can_redo), window);
  g_signal_connect_swapped (G_OBJECT (document->buffer), "modified-changed", G_CALLBACK (mousepad_window_modified_changed), window);
  g_signal_connect (G_OBJECT (document->textview), "populate-popup", G_CALLBACK (mousepad_window_menu_textview_popup), window);

  /* get the number of pages */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

  /* check tabs should always be visible */
  g_object_get (G_OBJECT (window->preferences), "misc-always-show-tabs", &always_show_tabs, NULL);

  /* change the visibility of the tabs accordingly */
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), always_show_tabs || (npages > 1));

  /* update the go menu */
  mousepad_window_update_gomenu (window);
}



static void
mousepad_window_notebook_removed (GtkNotebook     *notebook,
                                  GtkWidget       *page,
                                  guint            page_num,
                                  MousepadWindow  *window)
{
  gboolean          always_show_tabs;
  gint              npages;
  MousepadDocument *document = MOUSEPAD_DOCUMENT (page);

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));
  _mousepad_return_if_fail (GTK_IS_NOTEBOOK (notebook));

  /* disconnect the old document signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (page), mousepad_window_button_close_tab, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (page), mousepad_window_cursor_changed, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (page), mousepad_window_selection_changed, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (page), mousepad_window_overwrite_changed, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (page), mousepad_window_drag_data_received, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (document->undo), mousepad_window_can_undo, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (document->undo), mousepad_window_can_redo, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (document->buffer), mousepad_window_modified_changed, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (document->textview), mousepad_window_menu_textview_popup, window);

  /* unset the go menu item (part of the old window) */
  mousepad_object_set_data (G_OBJECT (page), "navigation-menu-action", NULL);

  /* get the number of pages in this notebook */
  npages = gtk_notebook_get_n_pages (notebook);

  /* update the window */
  if (npages == 0)
    {
      /* window contains no tabs, destroy it */
      gtk_widget_destroy (GTK_WIDGET (window));
    }
  else
    {
      /* check tabs should always be visible */
      g_object_get (G_OBJECT (window->preferences), "misc-always-show-tabs", &always_show_tabs, NULL);

      /* change the visibility of the tabs accordingly */
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), always_show_tabs || (npages > 1));

      /* update the go menu */
      mousepad_window_update_gomenu (window);
    }
}



static void
mousepad_window_notebook_menu_position (GtkMenu  *menu,
                                        gint     *x,
                                        gint     *y,
                                        gboolean *push_in,
                                        gpointer  user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);

  gdk_window_get_origin (widget->window, x, y);

  *x += widget->allocation.x;
  *y += widget->allocation.y + widget->allocation.height;

  *push_in = TRUE;
}



static gboolean
mousepad_window_notebook_button_press_event (GtkNotebook    *notebook,
                                             GdkEventButton *event,
                                             MousepadWindow *window)
{
  GtkWidget *page, *label;
  GtkWidget *menu;
  guint      page_num = 0;
  gint       x_root;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);

  if (event->type == GDK_BUTTON_PRESS && (event->button == 3 || event->button == 2))
    {
      /* walk through the tabs and look for the tab under the cursor */
      while ((page = gtk_notebook_get_nth_page (notebook, page_num)) != NULL)
        {
          label = gtk_notebook_get_tab_label (notebook, page);

          /* get the origin of the label */
          gdk_window_get_origin (label->window, &x_root, NULL);
          x_root = x_root + label->allocation.x;

          /* check if the cursor is inside this label */
          if (event->x_root >= x_root && event->x_root <= (x_root + label->allocation.width))
            {
              /* switch to this tab */
              gtk_notebook_set_current_page (notebook, page_num);

              /* handle the button action */
              if (event->button == 3)
                {
                  /* get the menu */
                  menu = gtk_ui_manager_get_widget (window->ui_manager, "/tab-menu");

                  /* show it */
                  gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                                  mousepad_window_notebook_menu_position, label,
                                  event->button, event->time);
                }
              else if (event->button == 2)
                {
                  /* close the document */
                  mousepad_window_action_close (NULL, window);
                }

              /* we succeed */
              return TRUE;
            }

          /* try the next tab */
          ++page_num;
        }
    }
  else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      /* check if the event window is the notebook event window (not a tab) */
      if (event->window == notebook->event_window)
        {
          /* create new document */
          mousepad_window_action_new (NULL, window);

          /* we succeed */
          return TRUE;
        }
    }

  return FALSE;
}



static gboolean
mousepad_window_notebook_button_release_event (GtkNotebook    *notebook,
                                               GdkEventButton *event,
                                               MousepadWindow *window)
{
  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);
  _mousepad_return_val_if_fail (MOUSEPAD_IS_DOCUMENT (window->active), FALSE);

  /* focus the active textview */
  mousepad_document_focus_textview (window->active);

  return FALSE;
}



#if GTK_CHECK_VERSION (2,12,0)
static GtkNotebook *
mousepad_window_notebook_create_window (GtkNotebook    *notebook,
                                        GtkWidget      *page,
                                        gint            x,
                                        gint            y,
                                        MousepadWindow *window)
{
  MousepadDocument *document;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), NULL);
  _mousepad_return_val_if_fail (MOUSEPAD_IS_DOCUMENT (page), NULL);

  /* only create new window when there are more then 2 tabs */
  if (gtk_notebook_get_n_pages (notebook) >= 2)
    {
      /* get the document */
      document = MOUSEPAD_DOCUMENT (page);

      /* take a reference */
      g_object_ref (G_OBJECT (document));

      /* remove the document from the active window */
      gtk_container_remove (GTK_CONTAINER (window->notebook), page);

      /* emit the new window with document signal */
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW_WITH_DOCUMENT], 0, document, x, y);

      /* release our reference */
      g_object_unref (G_OBJECT (document));
    }

  return NULL;
}
#endif



/**
 * Document Signals Functions
 **/
static void
mousepad_window_modified_changed (MousepadWindow   *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  mousepad_window_set_title (window);
}



static void
mousepad_window_cursor_changed (MousepadDocument *document,
                                gint              line,
                                gint              column,
                                gint              selection,
                                MousepadWindow   *window)
{


  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));

  if (window->statusbar)
    {
      /* set the new statusbar cursor position and selection length */
      mousepad_statusbar_set_cursor_position (MOUSEPAD_STATUSBAR (window->statusbar), line, column, selection);
    }
}



static void
mousepad_window_selection_changed (MousepadDocument *document,
                                   gint              selection,
                                   MousepadWindow   *window)
{
  GtkAction *action;
  gint       i;

  /* sensitivity of the change selection action */
  action = gtk_action_group_get_action (window->action_group, "change-selection");
  gtk_action_set_sensitive (action, selection != 0);

  /* actions that are unsensitive during a column selection */
  const gchar *action_names1[] = { "tabs-to-spaces", "spaces-to-tabs", "duplicate", "strip-trailing" };
  for (i = 0; i < G_N_ELEMENTS (action_names1); i++)
    {
      action = gtk_action_group_get_action (window->action_group, action_names1[i]);
      gtk_action_set_sensitive (action, selection == 0 || selection == 1);
    }

  /* action that are only sensitive for normal selections */
  const gchar *action_names2[] = { "line-up", "line-down" };
  for (i = 0; i < G_N_ELEMENTS (action_names2); i++)
    {
      action = gtk_action_group_get_action (window->action_group, action_names2[i]);
      gtk_action_set_sensitive (action, selection == 1);
    }

  /* actions that are sensitive for all selections with content */
  const gchar *action_names3[] = { "cut", "copy", "delete", "lowercase", "uppercase", "titlecase", "opposite-case" };
  for (i = 0; i < G_N_ELEMENTS (action_names3); i++)
    {
      action = gtk_action_group_get_action (window->action_group, action_names3[i]);
      gtk_action_set_sensitive (action, selection > 0);
    }
}



static void
mousepad_window_overwrite_changed (MousepadDocument *document,
                                   gboolean          overwrite,
                                   MousepadWindow   *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));

  /* set the new overwrite mode in the statusbar */
  if (window->statusbar)
    mousepad_statusbar_set_overwrite (MOUSEPAD_STATUSBAR (window->statusbar), overwrite);
}



static void
mousepad_window_can_undo (MousepadWindow *window,
                          gboolean        can_undo)
{
  GtkAction *action;

  action = gtk_action_group_get_action (window->action_group, "undo");
  gtk_action_set_sensitive (action, can_undo);
}



static void
mousepad_window_can_redo (MousepadWindow *window,
                          gboolean        can_redo)
{
  GtkAction *action;

  action = gtk_action_group_get_action (window->action_group, "redo");
  gtk_action_set_sensitive (action, can_redo);
}



/**
 * Menu Functions
 **/
static void
mousepad_window_menu_templates_fill (MousepadWindow *window,
                                     GtkWidget      *menu,
                                     const gchar    *path)
{
  GDir        *dir;
  GSList      *files_list = NULL;
  GSList      *dirs_list = NULL;
  GSList      *li;
  gchar       *absolute_path;
  gchar       *label, *dot;
  const gchar *name;
  GtkWidget   *item, *image, *submenu;

  /* open the directory */
  dir = g_dir_open (path, 0, NULL);

  /* read the directory */
  if (G_LIKELY (dir))
    {
      /* walk the directory */
      for (;;)
        {
          /* read the filename of the next file */
          name = g_dir_read_name (dir);

          /* break when we reached the last file */
          if (G_UNLIKELY (name == NULL))
            break;

          /* skip hidden files */
          if (name[0] == '.')
            continue;

          /* build absolute path */
          absolute_path = g_build_path (G_DIR_SEPARATOR_S, path, name, NULL);

          /* check if the file is a regular file or directory */
          if (g_file_test (absolute_path, G_FILE_TEST_IS_DIR))
            dirs_list = g_slist_insert_sorted (dirs_list, absolute_path, (GCompareFunc) strcmp);
          else if (g_file_test (absolute_path, G_FILE_TEST_IS_REGULAR))
            files_list = g_slist_insert_sorted (files_list, absolute_path, (GCompareFunc) strcmp);
        }

      /* close the directory */
      g_dir_close (dir);
    }

  /* append the directories */
  for (li = dirs_list; li != NULL; li = li->next)
    {
      /* create a newsub menu for the directory */
      submenu = gtk_menu_new ();
      g_object_ref_sink (G_OBJECT (submenu));
      gtk_menu_set_screen (GTK_MENU (submenu), gtk_widget_get_screen (menu));

      /* fill the menu */
      mousepad_window_menu_templates_fill (window, submenu, li->data);

      /* check if the sub menu contains items */
      if (G_LIKELY (GTK_MENU_SHELL (submenu)->children != NULL))
        {
          /* create directory label */
          label = g_filename_display_basename (li->data);

          /* append the menu */
          item = gtk_image_menu_item_new_with_label (label);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_show (item);

          /* cleanup */
          g_free (label);

          /* set menu image */
          image = gtk_image_new_from_icon_name (GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
          gtk_widget_show (image);
        }

      /* cleanup */
      g_free (li->data);
      g_object_unref (G_OBJECT (submenu));
    }

  /* append the files */
  for (li = files_list; li != NULL; li = li->next)
    {
      /* create directory label */
      label = g_filename_display_basename (li->data);

      /* strip the extension from the label */
      dot = g_utf8_strrchr (label, -1, '.');
      if (dot != NULL)
        *dot = '\0';

      /* create menu item */
      item = gtk_image_menu_item_new_with_label (label);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      mousepad_object_set_data_full (G_OBJECT (item), "filename", li->data, g_free);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (mousepad_window_action_new_from_template), window);
      gtk_widget_show (item);

      /* set menu image */
      image = gtk_image_new_from_icon_name (GTK_STOCK_FILE, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      /* cleanup */
      g_free (label);
    }

  /* cleanup */
  g_slist_free (dirs_list);
  g_slist_free (files_list);
}



static void
mousepad_window_menu_templates (GtkWidget      *item,
                                MousepadWindow *window)
{
  GtkWidget *submenu;
  gchar     *templates_path;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (GTK_IS_MENU_ITEM (item));
  _mousepad_return_if_fail (gtk_menu_item_get_submenu (GTK_MENU_ITEM (item)) == NULL);

  /* schedule the idle build of the recent menu */
  mousepad_window_recent_menu (window);

  /* get the templates path */
  templates_path = xfce_get_homefile ("Templates", NULL);

  /* check if the directory exists */
  if (g_file_test (templates_path, G_FILE_TEST_IS_DIR))
    {
      /* create submenu */
      submenu = gtk_menu_new ();
      g_object_ref_sink (G_OBJECT (submenu));
      gtk_menu_set_screen (GTK_MENU (submenu), gtk_widget_get_screen (item));

      /* fill the menu */
      mousepad_window_menu_templates_fill (window, submenu, templates_path);

      /* set the submenu if it contains items, else hide the item */
      if (G_LIKELY (GTK_MENU_SHELL (submenu)->children != NULL))
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
      else
        gtk_widget_hide (item);

      /* release */
      g_object_unref (G_OBJECT (submenu));
    }
  else
    {
      /* hide the templates menu item */
      gtk_widget_hide (item);
    }

  /* cleanup */
  g_free (templates_path);
}



static void
mousepad_window_menu_tab_sizes (MousepadWindow *window)
{
  GtkRadioAction   *action;
  GSList           *group = NULL;
  gint              i, size, merge_id;
  gchar            *name, *tmp;
  gchar           **tab_sizes;

  /* lock menu updates */
  lock_menu_updates++;

  /* get the default tab sizes and active tab size */
  g_object_get (G_OBJECT (window->preferences), "misc-default-tab-sizes", &tmp, NULL);

  /* get sizes array and free the temp string */
  tab_sizes = g_strsplit (tmp, ",", -1);
  g_free (tmp);

  /* create merge id */
  merge_id = gtk_ui_manager_new_merge_id (window->ui_manager);

  /* add the default sizes to the menu */
  for (i = 0; tab_sizes[i] != NULL; i++)
    {
      /* convert the string to a number */
      size = strtol (tab_sizes[i], NULL, 10);

      /* keep this in sync with the property limits */
      if (G_LIKELY (size > 0))
        {
          /* keep this in sync with the properties */
          size = CLAMP (size, 1, 32);

          /* create action name */
          name = g_strdup_printf ("tab-size_%d", size);

          action = gtk_radio_action_new (name, name + 8, NULL, NULL, size);
          gtk_radio_action_set_group (action, group);
          group = gtk_radio_action_get_group (action);
          g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (mousepad_window_action_tab_size), window);
          gtk_action_group_add_action_with_accel (window->action_group, GTK_ACTION (action), "");

          /* release the action */
          g_object_unref (G_OBJECT (action));

          /* add the action to the go menu */
          gtk_ui_manager_add_ui (window->ui_manager, merge_id,
                                 "/main-menu/document-menu/tab-size-menu/placeholder-tab-items",
                                 name, name, GTK_UI_MANAGER_MENUITEM, FALSE);

          /* cleanup */
          g_free (name);
        }
    }

  /* cleanup the array */
  g_strfreev (tab_sizes);

  /* create other action */
  action = gtk_radio_action_new ("tab-size-other", "", _("Set custom tab size"), NULL, 0);
  gtk_radio_action_set_group (action, group);
  g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (mousepad_window_action_tab_size), window);
  gtk_action_group_add_action_with_accel (window->action_group, GTK_ACTION (action), "");

  /* release the action */
  g_object_unref (G_OBJECT (action));

  /* add the action to the go menu */
  gtk_ui_manager_add_ui (window->ui_manager, merge_id,
                         "/main-menu/document-menu/tab-size-menu/placeholder-tab-items",
                         "tab-size-other", "tab-size-other", GTK_UI_MANAGER_MENUITEM, FALSE);

  /* unlock */
  lock_menu_updates--;
}



static void
mousepad_window_menu_tab_sizes_update (MousepadWindow *window)
{
  gint       tab_size;
  gchar     *name, *label = NULL;
  GtkAction *action;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* avoid menu actions */
  lock_menu_updates++;

  /* get tab size of active document */
  tab_size = mousepad_view_get_tab_size (window->active->textview);

  /* check if there is a default item with this number */
  name = g_strdup_printf ("tab-size_%d", tab_size);
  action = gtk_action_group_get_action (window->action_group, name);
  g_free (name);

  if (action)
    {
      /* toggle the default action */
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
    }
  else
    {
      /* create suitable label for the other menu */
      label = g_strdup_printf (_("Ot_her (%d)..."), tab_size);
    }

  /* get other action */
  action = gtk_action_group_get_action (window->action_group, "tab-size-other");

  /* toggle other action if needed */
  if (label)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

  /* set action label */
  g_object_set (G_OBJECT (action), "label", label ? label : _("Ot_her..."), NULL);

  /* cleanup */
  g_free (label);

  /* allow menu actions again */
  lock_menu_updates--;
}



static void
mousepad_window_menu_textview_deactivate (GtkWidget   *menu,
                                          GtkTextView *textview)
{
  _mousepad_return_if_fail (GTK_IS_TEXT_VIEW (textview));
  _mousepad_return_if_fail (textview->popup_menu == menu);

  /* disconnect this signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (menu), mousepad_window_menu_textview_deactivate, textview);

  /* unset the popup menu since your menu is owned by the ui manager */
  GTK_TEXT_VIEW (textview)->popup_menu = NULL;
}



static void
mousepad_window_menu_textview_popup (GtkTextView    *textview,
                                     GtkMenu        *old_menu,
                                     MousepadWindow *window)
{
  GtkWidget *menu;

  _mousepad_return_if_fail (GTK_WIDGET (old_menu) == textview->popup_menu);
  _mousepad_return_if_fail (GTK_IS_TEXT_VIEW (textview));
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));
  _mousepad_return_if_fail (GTK_IS_MENU (textview->popup_menu));

  /* destroy origional menu */
  gtk_widget_destroy (textview->popup_menu);

  /* get the textview menu */
  menu = gtk_ui_manager_get_widget (window->ui_manager, "/textview-menu");

  /* connect signal */
  g_signal_connect (G_OBJECT (menu), "deactivate", G_CALLBACK (mousepad_window_menu_textview_deactivate), textview);

  /* set screen */
  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (GTK_WIDGET (textview)));

  /* set ours */
  textview->popup_menu = menu;
}



static void
mousepad_window_update_actions (MousepadWindow *window)
{
  GtkAction          *action;
  GtkNotebook        *notebook = GTK_NOTEBOOK (window->notebook);
  MousepadDocument   *document;
  gboolean            cycle_tabs;
  gint                n_pages;
  gint                page_num;
  gboolean            active;
  MousepadLineEnding  line_ending;
  const gchar        *action_name;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* active document */
  document = window->active;

  /* update the actions for the active document */
  if (G_LIKELY (document))
    {
      /* avoid menu actions */
      lock_menu_updates++;

      /* determine the number of pages and the current page number */
      n_pages = gtk_notebook_get_n_pages (notebook);
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (document));

      /* whether we cycle tabs */
      g_object_get (G_OBJECT (window->preferences), "misc-cycle-tabs", &cycle_tabs, NULL);

      /* set the sensitivity of the back and forward buttons in the go menu */
      action = gtk_action_group_get_action (window->action_group, "back");
      gtk_action_set_sensitive (action, (cycle_tabs && n_pages > 1) || (page_num > 0));

      action = gtk_action_group_get_action (window->action_group, "forward");
      gtk_action_set_sensitive (action, (cycle_tabs && n_pages > 1 ) || (page_num < n_pages - 1));

      /* set the reload, detach and save sensitivity */
      action = gtk_action_group_get_action (window->action_group, "save");
      gtk_action_set_sensitive (action, !mousepad_file_get_read_only (document->file));

      action = gtk_action_group_get_action (window->action_group, "detach");
      gtk_action_set_sensitive (action, (n_pages > 1));

      action = gtk_action_group_get_action (window->action_group, "revert");
      gtk_action_set_sensitive (action, mousepad_file_get_filename (document->file) != NULL);

      /* line ending type */
      line_ending = mousepad_file_get_line_ending (document->file);
      if (G_UNLIKELY (line_ending == MOUSEPAD_LINE_END_MAC))
        action_name = "mac";
      else if (G_UNLIKELY (line_ending == MOUSEPAD_LINE_END_DOS))
        action_name = "dos";
      else
        action_name = "unix";

      /* set the corrent line ending type */
      action = gtk_action_group_get_action (window->action_group, action_name);
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

      /* toggle the document settings */
      active = mousepad_document_get_word_wrap (document);
      action = gtk_action_group_get_action (window->action_group, "word-wrap");
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);

      active = mousepad_view_get_line_numbers (document->textview);
      action = gtk_action_group_get_action (window->action_group, "line-numbers");
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);

      active = mousepad_view_get_auto_indent (document->textview);
      action = gtk_action_group_get_action (window->action_group, "auto-indent");
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);

      /* update the tabs size menu */
      mousepad_window_menu_tab_sizes_update (window);

      active = mousepad_view_get_insert_spaces (document->textview);
      action = gtk_action_group_get_action (window->action_group, "insert-spaces");
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);

      /* set the sensitivity of the undo and redo actions */
      mousepad_window_can_undo (window, mousepad_undo_can_undo (document->undo));
      mousepad_window_can_redo (window, mousepad_undo_can_redo (document->undo));

      /* active this tab in the go menu */
      action = mousepad_object_get_data (G_OBJECT (document), "navigation-menu-action");
      if (G_LIKELY (action != NULL))
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

      /* allow menu actions again */
      lock_menu_updates--;
    }
}



static gboolean
mousepad_window_update_gomenu_idle (gpointer user_data)
{
  MousepadDocument *document;
  MousepadWindow   *window;
  gint              npages;
  gint              n;
  gchar            *name;
  const gchar      *title;
  const gchar      *tooltip;
  gchar             accelerator[7];
  GtkRadioAction   *action;
  GSList           *group = NULL;
  GList            *actions, *li;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (user_data), FALSE);

  GDK_THREADS_ENTER ();

  /* get the window */
  window = MOUSEPAD_WINDOW (user_data);

  /* prevent menu updates */
  lock_menu_updates++;

  /* remove the old merge */
  if (window->gomenu_merge_id != 0)
    {
      gtk_ui_manager_remove_ui (window->ui_manager, window->gomenu_merge_id);

      /* drop all the previous actions from the action group */
      actions = gtk_action_group_list_actions (window->action_group);
      for (li = actions; li != NULL; li = li->next)
        if (strncmp (gtk_action_get_name (li->data), "mousepad-tab-", 13) == 0)
          gtk_action_group_remove_action (window->action_group, GTK_ACTION (li->data));
      g_list_free (actions);
    }

  /* create a new merge id */
  window->gomenu_merge_id = gtk_ui_manager_new_merge_id (window->ui_manager);

  /* walk through the notebook pages */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

  for (n = 0; n < npages; ++n)
    {
      document = MOUSEPAD_DOCUMENT (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), n));

      /* create a new action name */
      name = g_strdup_printf ("mousepad-tab-%d", n);

      /* get the name and file name */
      title = mousepad_document_get_basename (document);
      tooltip = mousepad_document_get_filename (document);

      /* create the radio action */
      action = gtk_radio_action_new (name, title, tooltip, NULL, n);
      gtk_radio_action_set_group (action, group);
      group = gtk_radio_action_get_group (action);
      g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (mousepad_window_action_go_to_tab), window->notebook);

      /* connect the action to the document to we can easily active it when the user switched from tab */
      mousepad_object_set_data (G_OBJECT (document), "navigation-menu-action", action);

      if (G_LIKELY (n < 9))
        {
          /* create an accelerator and add it to the menu */
          g_snprintf (accelerator, sizeof (accelerator), "<Alt>%d", n + 1);
          gtk_action_group_add_action_with_accel (window->action_group, GTK_ACTION (action), accelerator);
        }
      else
        /* add a menu item without accelerator */
        gtk_action_group_add_action (window->action_group, GTK_ACTION (action));

      /* select the active entry */
      if (gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook)) == n)
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

      /* release the action */
      g_object_unref (G_OBJECT (action));

      /* add the action to the go menu */
      gtk_ui_manager_add_ui (window->ui_manager, window->gomenu_merge_id,
                             "/main-menu/navigation-menu/placeholder-file-items",
                             name, name, GTK_UI_MANAGER_MENUITEM, FALSE);

      /* cleanup */
      g_free (name);
    }

  /* make sure the ui is up2date to avoid flickering */
  gtk_ui_manager_ensure_update (window->ui_manager);

  /* release our lock */
  lock_menu_updates--;

  GDK_THREADS_LEAVE ();

  return FALSE;
}



static void
mousepad_window_update_gomenu_idle_destroy (gpointer user_data)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (user_data));

  MOUSEPAD_WINDOW (user_data)->update_go_menu_id = 0;
}



static void
mousepad_window_update_gomenu (MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* leave when we're updating multiple files or there is this an idle function pending */
  if (lock_menu_updates && window->update_go_menu_id != 0)
    return;

  /* schedule a go menu update */
  window->update_go_menu_id = g_idle_add_full (G_PRIORITY_LOW, mousepad_window_update_gomenu_idle,
                                               window, mousepad_window_update_gomenu_idle_destroy);
}



/**
 * Funtions for managing the recent files
 **/
static void
mousepad_window_recent_add (MousepadWindow *window,
                            MousepadFile   *file)
{
  GtkRecentData  info;
  gchar         *uri;
  static gchar  *groups[] = { PACKAGE_NAME, NULL };
  gchar         *description;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_FILE (file));

  /* build description */
  description = g_strdup_printf ("%s: %s", _("Encoding"), mousepad_file_get_encoding (file));

  /* create the recent data */
  info.display_name = NULL;
  info.description  = description;
  info.mime_type    = "text/plain";
  info.app_name     = PACKAGE_NAME;
  info.app_exec     = PACKAGE " %u";
  info.groups       = groups;
  info.is_private   = FALSE;

  /* create an uri from the filename */
  uri = mousepad_file_get_uri (file);

  if (G_LIKELY (uri != NULL))
    {
      /* make sure the recent manager is initialized */
      mousepad_window_recent_manager_init (window);

      /* add the new recent info to the recent manager */
      gtk_recent_manager_add_full (window->recent_manager, uri, &info);

      /* cleanup */
      g_free (uri);
    }

  /* cleanup */
  g_free (description);
}



static gint
mousepad_window_recent_sort (GtkRecentInfo *a,
                             GtkRecentInfo *b)
{
  return (gtk_recent_info_get_modified (a) < gtk_recent_info_get_modified (b));
}



static void
mousepad_window_recent_manager_init (MousepadWindow *window)
{
  /* set recent manager if not already done */
  if (G_UNLIKELY (window->recent_manager == NULL))
    {
      /* get the default manager */
      window->recent_manager = gtk_recent_manager_get_default ();

      /* connect changed signal */
      g_signal_connect_swapped (G_OBJECT (window->recent_manager), "changed", G_CALLBACK (mousepad_window_recent_menu), window);
    }
}



static gboolean
mousepad_window_recent_menu_idle (gpointer user_data)
{
  MousepadWindow *window = MOUSEPAD_WINDOW (user_data);
  GList          *items, *li, *actions;
  GList          *filtered = NULL;
  GtkRecentInfo  *info;
  const gchar    *uri;
  const gchar    *display_name;
  gchar          *tooltip, *name, *label;
  gchar          *filename, *filename_utf8;
  GtkAction      *action;
  gint            n;

  GDK_THREADS_ENTER ();

  if (window->recent_merge_id != 0)
    {
      /* unmerge the ui controls from the previous update */
      gtk_ui_manager_remove_ui (window->ui_manager, window->recent_merge_id);

      /* drop all the previous actions from the action group */
      actions = gtk_action_group_list_actions (window->action_group);
      for (li = actions; li != NULL; li = li->next)
        if (strncmp (gtk_action_get_name (li->data), "recent-info-", 12) == 0)
          gtk_action_group_remove_action (window->action_group, li->data);
      g_list_free (actions);
    }

  /* create a new merge id */
  window->recent_merge_id = gtk_ui_manager_new_merge_id (window->ui_manager);

  /* make sure the recent manager is initialized */
  mousepad_window_recent_manager_init (window);

  /* get all the items in the manager */
  items = gtk_recent_manager_get_items (window->recent_manager);

  /* walk through the items in the manager and pick the ones that or in the mousepad group */
  for (li = items; li != NULL; li = li->next)
    {
      /* check if the item is in the Mousepad group */
      if (!gtk_recent_info_has_group (li->data, PACKAGE_NAME))
        continue;

      /* insert the the list, sorted by date */
      filtered = g_list_insert_sorted (filtered, li->data, (GCompareFunc) mousepad_window_recent_sort);
    }

  /* get the recent menu limit number */
  g_object_get (G_OBJECT (window->preferences), "misc-recent-menu-items", &n, NULL);

  /* append the items to the menu */
  for (li = filtered; n > 0 && li != NULL; li = li->next)
    {
      info = li->data;

      /* get the filename */
      uri = gtk_recent_info_get_uri (info);
      filename = g_filename_from_uri (uri, NULL, NULL);

      /* append to the menu if the file exists, else remove it from the history */
      if (filename && g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          /* create the action name */
          name = g_strdup_printf ("recent-info-%d", n);

          /* get the name of the item and escape the underscores */
          display_name = gtk_recent_info_get_display_name (info);
          label = mousepad_util_escape_underscores (display_name);

          /* create and utf-8 valid version of the filename */
          filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
          tooltip = g_strdup_printf (_("Open '%s'"), filename_utf8);
          g_free (filename_utf8);

          /* create the action */
          action = gtk_action_new (name, label, tooltip, NULL);

          /* cleanup */
          g_free (tooltip);
          g_free (label);

          /* add the info data and connect a menu signal */
          mousepad_object_set_data_full (G_OBJECT (action), "gtk-recent-info", gtk_recent_info_ref (info), gtk_recent_info_unref);
          g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (mousepad_window_action_open_recent), window);

          /* add the action to the recent actions group */
          gtk_action_group_add_action (window->action_group, action);

          /* release the action */
          g_object_unref (G_OBJECT (action));

          /* add the action to the menu */
          gtk_ui_manager_add_ui (window->ui_manager, window->recent_merge_id,
                                 "/main-menu/file-menu/recent-menu/placeholder-recent-items",
                                 name, name, GTK_UI_MANAGER_MENUITEM, FALSE);

          /* cleanup */
          g_free (name);

          /* decrease counter */
          n--;
        }
      else
        {
          /* remove the item. don't both the user if this fails */
          gtk_recent_manager_remove_item (window->recent_manager, uri, NULL);
        }

      /* cleanup */
      g_free (filename);
    }

  /* set the visibility of the 'no items found' action */
  action = gtk_action_group_get_action (window->action_group, "no-recent-items");
  gtk_action_set_visible (action, (filtered == NULL));
  gtk_action_set_sensitive (action, FALSE);

  /* set the sensitivity of the clear button */
  action = gtk_action_group_get_action (window->action_group, "clear-recent");
  gtk_action_set_sensitive (action, (filtered != NULL));

  /* cleanup */
  g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
  g_list_free (items);
  g_list_free (filtered);

  /* make sure the ui is up2date to avoid flickering */
  gtk_ui_manager_ensure_update (window->ui_manager);

  GDK_THREADS_LEAVE ();

  /* stop the idle function */
  return FALSE;
}



static void
mousepad_window_recent_menu_idle_destroy (gpointer user_data)
{
  MOUSEPAD_WINDOW (user_data)->update_recent_menu_id = 0;
}



static void
mousepad_window_recent_menu (MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* leave when we're updating multiple files or there is this an idle function pending */
  if (lock_menu_updates > 0 || window->update_recent_menu_id != 0)
    return;

  /* schedule a recent menu update */
  window->update_recent_menu_id = g_idle_add_full (G_PRIORITY_LOW, mousepad_window_recent_menu_idle,
                                                   window, mousepad_window_recent_menu_idle_destroy);
}



static void
mousepad_window_recent_clear (MousepadWindow *window)
{
  GList         *items, *li;
  const gchar   *uri;
  GError        *error = NULL;
  GtkRecentInfo *info;

  /* make sure the recent manager is initialized */
  mousepad_window_recent_manager_init (window);

  /* get all the items in the manager */
  items = gtk_recent_manager_get_items (window->recent_manager);

  /* walk through the items */
  for (li = items; li != NULL; li = li->next)
    {
      info = li->data;

      /* check if the item is in the Mousepad group */
      if (!gtk_recent_info_has_group (info, PACKAGE_NAME))
        continue;

      /* get the uri of the recent item */
      uri = gtk_recent_info_get_uri (info);

      /* try to remove it, if it fails, break the loop to avoid multiple errors */
      if (G_UNLIKELY (gtk_recent_manager_remove_item (window->recent_manager, uri, &error) == FALSE))
        break;
     }

  /* cleanup */
  g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
  g_list_free (items);

  /* print a warning is there is one */
  if (G_UNLIKELY (error != NULL))
    {
      mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to clear the recent history"));
      g_error_free (error);
    }
}



/**
 * Drag and drop functions
 **/
static void
mousepad_window_drag_data_received (GtkWidget        *widget,
                                    GdkDragContext   *context,
                                    gint              x,
                                    gint              y,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             time,
                                    MousepadWindow   *window)
{
  gchar     **uris;
  gchar      *working_directory;
  GtkWidget  *notebook, **document;
  GtkWidget  *child, *label;
  gint        i, n_pages;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  /* we only accept text/uri-list drops with format 8 and atleast one byte of data */
  if (info == TARGET_TEXT_URI_LIST && selection_data->format == 8 && selection_data->length > 0)
    {
      /* extract the uris from the data */
      uris = g_uri_list_extract_uris ((const gchar *)selection_data->data);

      /* get working directory */
      working_directory = g_get_current_dir ();

      /* open the files */
      mousepad_window_open_files (window, NULL, uris);

      /* cleanup */
      g_free (working_directory);
      g_strfreev (uris);

      /* finish the drag (copy) */
      gtk_drag_finish (context, TRUE, FALSE, time);
    }
  else if (info == TARGET_GTK_NOTEBOOK_TAB)
    {
      /* get the source notebook */
      notebook = gtk_drag_get_source_widget (context);

      /* get the document that has been dragged */
      document = (GtkWidget **) selection_data->data;

      /* check */
      _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (*document));

      /* take a reference on the document before we remove it */
      g_object_ref (G_OBJECT (*document));

      /* remove the document from the source window */
      gtk_container_remove (GTK_CONTAINER (notebook), *document);

      /* get the number of pages in the notebook */
      n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

      /* figure out where to insert the tab in the notebook */
      for (i = 0; i < n_pages; i++)
        {
          /* get the child label */
          child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i);
          label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (window->notebook), child);

          /* break if we have a matching drop position */
          if (x < (label->allocation.x + label->allocation.width / 2))
            break;
        }

      /* add the document to the new window */
      mousepad_window_add (window, MOUSEPAD_DOCUMENT (*document));

      /* move the tab to the correct position */
      gtk_notebook_reorder_child (GTK_NOTEBOOK (window->notebook), *document, i);

      /* release our reference on the document */
      g_object_unref (G_OBJECT (*document));

      /* finish the drag (move) */
      gtk_drag_finish (context, TRUE, TRUE, time);
    }
}



/**
 * Find and replace
 **/
static gint
mousepad_window_search (MousepadWindow      *window,
                        MousepadSearchFlags  flags,
                        const gchar         *string,
                        const gchar         *replacement)
{
  gint       nmatches = 0;
  gint       npages, i;
  GtkWidget *document;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), -1);

  if (flags & MOUSEPAD_SEARCH_FLAGS_ACTION_HIGHTLIGHT)
    {
      /* highlight all the matches */
      nmatches = mousepad_util_highlight (window->active->buffer, window->active->tag, string, flags);
    }
  else if (flags & MOUSEPAD_SEARCH_FLAGS_ALL_DOCUMENTS)
    {
      /* get the number of documents in this window */
      npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

      /* walk the pages */
      for (i = 0; i < npages; i++)
        {
          /* get the document */
          document = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i);

          /* replace the matches in the document */
          nmatches += mousepad_util_search (MOUSEPAD_DOCUMENT (document)->buffer, string, replacement, flags);
        }
    }
  else if (window->active != NULL)
    {
      /* search or replace in the active document */
      nmatches = mousepad_util_search (window->active->buffer, string, replacement, flags);

      /* make sure the selection is visible */
      if (flags & (MOUSEPAD_SEARCH_FLAGS_ACTION_SELECT | MOUSEPAD_SEARCH_FLAGS_ACTION_REPLACE) && nmatches > 0)
        mousepad_view_scroll_to_cursor (window->active->textview);
    }
  else
    {
      /* should never be reaches */
      _mousepad_assert_not_reached ();
    }

  return nmatches;
}



/**
 * Search Bar
 **/
static void
mousepad_window_hide_search_bar (MousepadWindow *window)
{
  MousepadSearchFlags flags;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));
  _mousepad_return_if_fail (MOUSEPAD_IS_SEARCH_BAR (window->search_bar));

  /* setup flags */
  flags = MOUSEPAD_SEARCH_FLAGS_ACTION_HIGHTLIGHT
          | MOUSEPAD_SEARCH_FLAGS_ACTION_CLEANUP;

  /* remove the highlight */
  mousepad_window_search (window, flags, NULL, NULL);

  /* hide the search bar */
  gtk_widget_hide (window->search_bar);

  /* focus the active document's text view */
  mousepad_document_focus_textview (window->active);
}



/**
 * Paste from History
 **/
static void
mousepad_window_paste_history_add (MousepadWindow *window)
{
  GtkClipboard *clipboard;
  gchar        *text;
  GSList       *li;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* get the current clipboard text */
  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (window), GDK_SELECTION_CLIPBOARD);
  text = gtk_clipboard_wait_for_text (clipboard);

  /* leave when there is no text */
  if (G_UNLIKELY (text == NULL))
    return;

  /* check if the item is already in the history */
  for (li = clipboard_history; li != NULL; li = li->next)
    if (strcmp (li->data, text) == 0)
      break;

  /* append the item or remove it */
  if (G_LIKELY (li == NULL))
    {
      /* add to the list */
      clipboard_history = g_slist_prepend (clipboard_history, text);

      /* get the 9th item from the list and remove it if it exists */
      li = g_slist_nth (clipboard_history, 9);
      if (li != NULL)
        {
          /* cleanup */
          g_free (li->data);
          clipboard_history = g_slist_delete_link (clipboard_history, li);
        }
    }
  else
    {
      /* already in the history, remove it */
      g_free (text);
    }
}



static void
mousepad_window_paste_history_menu_position (GtkMenu  *menu,
                                             gint     *x,
                                             gint     *y,
                                             gboolean *push_in,
                                             gpointer  user_data)
{
  MousepadWindow   *window = MOUSEPAD_WINDOW (user_data);
  MousepadDocument *document = window->active;
  GtkTextIter       iter;
  GtkTextMark      *mark;
  GdkRectangle      location;
  gint              iter_x, iter_y;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));
  _mousepad_return_if_fail (GTK_IS_TEXT_VIEW (document->textview));
  _mousepad_return_if_fail (GTK_IS_TEXT_BUFFER (document->buffer));

  /* get the root coordinates of the texview widget */
  gdk_window_get_origin (gtk_text_view_get_window (GTK_TEXT_VIEW (document->textview), GTK_TEXT_WINDOW_TEXT), x, y);

  /* get the cursor iter */
  mark = gtk_text_buffer_get_insert (document->buffer);
  gtk_text_buffer_get_iter_at_mark (document->buffer, &iter, mark);

  /* get iter location */
  gtk_text_view_get_iter_location (GTK_TEXT_VIEW (document->textview), &iter, &location);

  /* convert to textview coordinates */
  gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (document->textview), GTK_TEXT_WINDOW_TEXT,
                                         location.x, location.y, &iter_x, &iter_y);

  /* add the iter coordinates to the menu popup position */
  *x += iter_x;
  *y += iter_y + location.height;
}



static void
mousepad_window_paste_history_activate (GtkMenuItem    *item,
                                        MousepadWindow *window)
{
  const gchar *text;

  _mousepad_return_if_fail (GTK_IS_MENU_ITEM (item));
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));
  _mousepad_return_if_fail (MOUSEPAD_IS_VIEW (window->active->textview));

  /* get the menu item text */
  text = mousepad_object_get_data (G_OBJECT (item), "history-pointer");

  /* paste the text */
  if (G_LIKELY (text))
    mousepad_view_clipboard_paste (window->active->textview, text, FALSE);
}



static GtkWidget *
mousepad_window_paste_history_menu_item (const gchar *text,
                                         const gchar *mnemonic)
{
  GtkWidget   *item;
  GtkWidget   *label;
  GtkWidget   *hbox;
  const gchar *s;
  gchar       *label_str;
  GString     *string;

  /* create new label string */
  string = g_string_sized_new (PASTE_HISTORY_MENU_LENGTH);

  /* get the first 30 chars of the clipboard text */
  if (g_utf8_strlen (text, -1) > PASTE_HISTORY_MENU_LENGTH)
    {
      /* append the first 30 chars */
      s = g_utf8_offset_to_pointer (text, PASTE_HISTORY_MENU_LENGTH);
      string = g_string_append_len (string, text, s - text);

      /* make it look like a ellipsized string */
      string = g_string_append (string, "...");
    }
  else
    {
      /* append the entire string */
      string = g_string_append (string, text);
    }

  /* get the string */
  label_str = g_string_free (string, FALSE);

  /* replace tab and new lines with spaces */
  label_str = g_strdelimit (label_str, "\n\t\r", ' ');

  /* create a new item */
  item = gtk_menu_item_new ();

  /* create a hbox */
  hbox = gtk_hbox_new (FALSE, 14);
  gtk_container_add (GTK_CONTAINER (item), hbox);
  gtk_widget_show (hbox);

  /* create the clipboard label */
  label = gtk_label_new (label_str);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);

  /* create the mnemonic label */
  label = gtk_label_new_with_mnemonic (mnemonic);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), item);
  gtk_widget_show (label);

  /* cleanup */
  g_free (label_str);

  return item;
}



static GtkWidget *
mousepad_window_paste_history_menu (MousepadWindow *window)
{
  GSList       *li;
  gchar        *text;
  gpointer      list_data = NULL;
  GtkWidget    *item;
  GtkWidget    *menu;
  GtkClipboard *clipboard;
  gchar         mnemonic[4];
  gint          n;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), NULL);

  /* create new menu and set the screen */
  menu = gtk_menu_new ();
  g_object_ref_sink (G_OBJECT (menu));
  g_signal_connect (G_OBJECT (menu), "deactivate", G_CALLBACK (g_object_unref), NULL);
  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (GTK_WIDGET (window)));

  /* get the current clipboard text */
  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (window), GDK_SELECTION_CLIPBOARD);
  text = gtk_clipboard_wait_for_text (clipboard);

  /* append the history items */
  for (li = clipboard_history, n = 1; li != NULL; li = li->next)
    {
      /* skip the active clipboard item */
      if (G_UNLIKELY (list_data == NULL && text && strcmp (li->data, text) == 0))
        {
          /* store the pointer so we can attach it at the end of the menu */
          list_data = li->data;
        }
      else
        {
          /* create mnemonic string */
          g_snprintf (mnemonic, sizeof (mnemonic), "_%d", n++);

          /* create menu item */
          item = mousepad_window_paste_history_menu_item (li->data, mnemonic);
          mousepad_object_set_data (G_OBJECT (item), "history-pointer", li->data);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (mousepad_window_paste_history_activate), window);
          gtk_widget_show (item);
        }
    }

  /* cleanup */
  g_free (text);

  if (list_data != NULL)
    {
      /* add separator between history and active menu items */
      if (GTK_MENU_SHELL (menu)->children != NULL)
        {
          item = gtk_separator_menu_item_new ();
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_show (item);
        }

      /* create menu item for current clipboard text */
      item = mousepad_window_paste_history_menu_item (list_data, "_0");
      mousepad_object_set_data (G_OBJECT (item), "history-pointer", list_data);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (mousepad_window_paste_history_activate), window);
      gtk_widget_show (item);
    }
  else if (GTK_MENU_SHELL (menu)->children == NULL)
    {
      /* create an item to inform the user */
      item = gtk_menu_item_new_with_label (_("No clipboard data"));
      gtk_widget_set_sensitive (item, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

  return menu;
}



/**
 * Miscellaneous Actions
 **/
static void
mousepad_window_button_close_tab (MousepadDocument *document,
                                  MousepadWindow   *window)
{
  gint page_num;

  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* switch to the tab we're going to close */
  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (document));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page_num);

  /* close the document */
  mousepad_window_close_document (window, document);
}



static gboolean
mousepad_window_delete_event (MousepadWindow *window,
                              GdkEvent       *event)
{
  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);

  /* try to close the window */
  mousepad_window_action_close_window (NULL, window);

  /* we will close the window when all the tabs are closed */
  return TRUE;
}



/**
 * Menu Actions
 *
 * All those function should be sorted by the menu structure so it's
 * easy to find a function. The function can always use window->active, since
 * we can assume there is always an active document inside a window.
 **/
static void
mousepad_window_action_new (GtkAction      *action,
                            MousepadWindow *window)
{
  MousepadDocument *document;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* create new document */
  document = mousepad_document_new ();

  /* add the document to the window */
  mousepad_window_add (window, document);
}



static void
mousepad_window_action_new_window (GtkAction      *action,
                                   MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* emit the new window signal */
  g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW], 0);
}



static void
mousepad_window_action_new_from_template (GtkMenuItem    *item,
                                          MousepadWindow *window)
{
  const gchar      *filename;
  gchar            *contents;
  GError           *error = NULL;
  gboolean          succeed;
  gsize             length;
  MousepadDocument *document;
  GtkTextIter       iter;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (GTK_IS_MENU_ITEM (item));

  /* get the filename from the menu item */
  filename = mousepad_object_get_data (G_OBJECT (item), "filename");

  /* test if the file exists */
  if (G_LIKELY (filename && g_file_test (filename, G_FILE_TEST_IS_REGULAR)))
    {
      /* get the content of the template */
      succeed = g_file_get_contents (filename, &contents, &length, &error);
      if (G_LIKELY (succeed))
        {
          /* check if the template is utf-8 valid */
          succeed = g_utf8_validate (contents, length, NULL);
          if (G_LIKELY (succeed))
            {
              /* create new document */
              document = mousepad_document_new ();

              /* lock the undo manager */
              mousepad_undo_lock (document->undo);

              /* set the template content */
              gtk_text_buffer_set_text (document->buffer, contents, length);

              /* move iter to the start of the document */
              gtk_text_buffer_get_start_iter (document->buffer, &iter);
              gtk_text_buffer_place_cursor (document->buffer, &iter);

              /* buffer is not modified */
              gtk_text_buffer_set_modified (document->buffer, FALSE);

              /* release the lock */
              mousepad_undo_unlock (document->undo);

              /* add the document to the window */
              mousepad_window_add (window, document);
            }
          else
            {
              /* set and error */
              g_set_error (&error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                           _("The template is not UTF-8 valid"));
            }

          /* cleanup */
          g_free (contents);
        }

      if (G_UNLIKELY (succeed == FALSE))
        {
          /* show an error */
          mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to open new file from a template"));
          g_error_free (error);
        }
    }
  else
    {
      /* destroy the menu item */
      gtk_widget_destroy (GTK_WIDGET (item));
    }
}



static void
mousepad_window_action_open (GtkAction      *action,
                             MousepadWindow *window)
{
  GtkWidget   *chooser;
  const gchar *filename;
  GSList      *filenames, *li;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* create new file chooser dialog */
  chooser = gtk_file_chooser_dialog_new (_("Open File"),
                                         GTK_WINDOW (window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), TRUE);

  /* select the active document in the file chooser */
  filename = mousepad_file_get_filename (window->active->file);
  if (filename && g_file_test (filename, G_FILE_TEST_EXISTS))
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);

  /* run the dialog */
  if (G_LIKELY (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT))
    {
      /* hide the dialog */
      gtk_widget_hide (chooser);

      /* get a list of selected filenames */
      filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (chooser));

      /* lock menu updates */
      lock_menu_updates++;

      /* open all the selected filenames in a new tab */
      for (li = filenames; li != NULL; li = li->next)
        {
          /* open the file */
          mousepad_window_open_file (window, li->data, NULL);

          /* cleanup */
          g_free (li->data);
        }

      /* cleanup */
      g_slist_free (filenames);

      /* allow menu updates again */
      lock_menu_updates--;

      /* update the menus */
      mousepad_window_recent_menu (window);
      mousepad_window_update_gomenu (window);
    }

  /* destroy dialog */
  gtk_widget_destroy (chooser);
}



static void
mousepad_window_action_open_recent (GtkAction      *action,
                                    MousepadWindow *window)
{
  const gchar   *uri, *description;
  const gchar   *encoding = NULL;
  GError        *error = NULL;
  gint           offset;
  gchar         *filename;
  gboolean       succeed = FALSE;
  GtkRecentInfo *info;

  _mousepad_return_if_fail (GTK_IS_ACTION (action));
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* get the info */
  info = mousepad_object_get_data (G_OBJECT (action), "gtk-recent-info");

  if (G_LIKELY (info != NULL))
    {
      /* get the file uri */
      uri = gtk_recent_info_get_uri (info);

      /* build a filename from the uri */
      filename = g_filename_from_uri (uri, NULL, NULL);

      if (G_LIKELY (filename != NULL))
        {
          /* open the file in a new tab if it exists */
          if (g_file_test (filename, G_FILE_TEST_EXISTS))
            {
              /* check if we set the encoding in the recent description */
              description = gtk_recent_info_get_description (info);
              if (G_LIKELY (description))
                {
                  /* get the offset length */
                  offset = strlen (_("Encoding")) + 2;

                  /* check if the encoding string looks valid and set it */
                  if (G_LIKELY (strlen (description) > offset))
                    encoding = description + offset;
                }

              /* try to open the file */
              succeed = mousepad_window_open_file (window, filename, encoding);
            }
          else
            {
              /* create an error */
              g_set_error (&error,  G_FILE_ERROR, G_FILE_ERROR_IO,
                           _("Failed to open \"%s\" for reading. It will be "
                             "removed from the document history"), filename);

              /* show the warning and cleanup */
              mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to open file"));
              g_error_free (error);
            }

          /* cleanup */
          g_free (filename);

          /* update the document history */
          if (G_LIKELY (succeed))
            gtk_recent_manager_add_item (window->recent_manager, uri);
          else
            gtk_recent_manager_remove_item (window->recent_manager, uri, NULL);
        }
    }
}



static void
mousepad_window_action_clear_recent (GtkAction      *action,
                                     MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* ask the user if he or she really want to clear the history */
  if (mousepad_dialogs_clear_recent (GTK_WINDOW (window)))
    {
      /* avoid updating the menu */
      lock_menu_updates++;

      /* clear the document history */
      mousepad_window_recent_clear (window);

      /* allow menu updates again */
      lock_menu_updates--;

      /* update the recent menu */
      mousepad_window_recent_menu (window);
    }
}



static gboolean
mousepad_window_action_save (GtkAction      *action,
                             MousepadWindow *window)
{
  MousepadDocument *document = window->active;
  GError           *error = NULL;
  gboolean          succeed = FALSE;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);
  _mousepad_return_val_if_fail (MOUSEPAD_IS_DOCUMENT (window->active), FALSE);

  if (mousepad_file_get_filename (document->file) == NULL)
    {
      /* file has no filename yet, open the save as dialog */
      mousepad_window_action_save_as (NULL, window);
    }
  else
    {
      /* save the document */
      succeed = mousepad_file_save (document->file, &error);

      /* update the window title */
      mousepad_window_set_title (window);

      if (G_LIKELY (succeed))
        {
          /* store the save state in the undo manager */
          mousepad_undo_save_point (document->undo);
        }
      else
        {
          /* show the error */
          mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to save the document"));
          g_error_free (error);
        }
    }

  return succeed;
}



static gboolean
mousepad_window_action_save_as (GtkAction      *action,
                                MousepadWindow *window)
{
  MousepadDocument *document = window->active;
  gchar            *filename;
  const gchar      *current_filename;
  GtkWidget        *dialog;
  gboolean          succeed = FALSE;

  _mousepad_return_val_if_fail (MOUSEPAD_IS_WINDOW (window), FALSE);
  _mousepad_return_val_if_fail (MOUSEPAD_IS_DOCUMENT (window->active), FALSE);

  /* create the dialog */
  dialog = gtk_file_chooser_dialog_new (_("Save As"),
                                        GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  /* set the current filename if there is one */
  current_filename = mousepad_file_get_filename (document->file);
  if (current_filename)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), current_filename);

  /* run the dialog */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      /* get the new filename */
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (G_LIKELY (filename))
        {
          /* set the new filename */
          mousepad_file_set_filename (document->file, filename);

          /* cleanup */
          g_free (filename);

          /* save the file with the function above */
          succeed = mousepad_window_action_save (NULL, window);

          /* add to the recent history is saving succeeded */
          if (G_LIKELY (succeed))
            mousepad_window_recent_add (window, document->file);
        }
    }

  /* destroy the dialog */
  gtk_widget_destroy (dialog);

  return succeed;
}



static void
mousepad_window_action_save_all (GtkAction      *action,
                                 MousepadWindow *window)
{
  guint      i, current;
  gint       page_num;
  GtkWidget *document;
  GSList    *li, *unnamed = NULL;
  gboolean   succeed = TRUE;
  GError    *error = NULL;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get the current active tab */
  current = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));

  /* walk though all the document in the window */
  for (i = 0; i < gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)); i++)
    {
      /* get the document */
      document = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i);

      /* debug check */
      _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));

      /* continue if the document is not modified */
      if (!gtk_text_buffer_get_modified (MOUSEPAD_DOCUMENT (document)->buffer))
        continue;

      if (mousepad_file_get_filename (MOUSEPAD_DOCUMENT (document)->file) == NULL ||
          mousepad_file_get_read_only ((MOUSEPAD_DOCUMENT (document)->file)))
        {
          /* add the document to a queue to bother the user later */
          unnamed = g_slist_prepend (unnamed, document);
        }
      else
        {
          /* try to save the file */
          succeed = mousepad_file_save (MOUSEPAD_DOCUMENT (document)->file, &error);

          if (G_LIKELY (succeed))
            {
              /* store save state */
              mousepad_undo_save_point (MOUSEPAD_DOCUMENT (document)->undo);
            }
          else
            {
              /* break on problems */
              break;
            }
        }
    }

  if (G_UNLIKELY (succeed == FALSE))
    {
      /* focus the tab that triggered the problem */
      gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), i);

      /* show the error */
      mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to save the document"));
      g_error_free (error);
    }
  else
    {
      /* open a save as dialog for all the unnamed files */
      for (li = unnamed; li != NULL; li = li->next)
        {
          /* get the documents page number */
          page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (li->data));

          if (G_LIKELY (page_num > -1))
            {
              /* focus the tab we're going to save */
              gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page_num);

              /* trigger the save as function, break when something went wrong */
              if (!mousepad_window_action_save_as (NULL, window))
                break;
            }
        }

      /* focus the origional doc if everything went fine */
      if (G_LIKELY (li == NULL))
        gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), current);
    }

  /* cleanup */
  g_slist_free (unnamed);
}



static void
mousepad_window_action_revert (GtkAction      *action,
                               MousepadWindow *window)
{
  MousepadDocument *document = window->active;
  GError           *error = NULL;
  gint              response;
  gboolean          succeed;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* ask the user if he really wants to do this when the file is modified */
  if (gtk_text_buffer_get_modified (document->buffer))
    {
      /* ask the user if he really wants to revert */
      response = mousepad_dialogs_revert (GTK_WINDOW (window));

      if (response == MOUSEPAD_RESPONSE_SAVE_AS)
        {
          /* open the save as dialog, leave when use user did not save (or it failed) */
          if (!mousepad_window_action_save_as (NULL, window))
            return;
        }
      else if (response == MOUSEPAD_RESPONSE_CANCEL)
        {
          /* meh, first click revert and then cancel... pussy... */
          return;
        }

      /* small check for debug builds */
      _mousepad_return_if_fail (response == MOUSEPAD_RESPONSE_REVERT);
    }

  /* lock the undo manager */
  mousepad_undo_lock (document->undo);

  /* clear the undo history */
  mousepad_undo_clear (document->undo);

  /* reload the file */
  succeed = mousepad_file_reload (document->file, &error);

  /* release the lock */
  mousepad_undo_unlock (document->undo);

  if (G_UNLIKELY (succeed == FALSE))
    {
      /* show the error */
      mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to reload the document"));
      g_error_free (error);
    }
}



static void
mousepad_window_action_print (GtkAction      *action,
                              MousepadWindow *window)
{
  MousepadPrint    *print;
  GError           *error = NULL;
  gboolean          succeed;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* create new print operation */
  print = mousepad_print_new ();

  /* print the current document */
  succeed = mousepad_print_document_interactive (print, window->active, GTK_WINDOW (window), &error);

  if (G_UNLIKELY (succeed == FALSE))
    {
      /* show the error */
      mousepad_dialogs_show_error (GTK_WINDOW (window), error, _("Failed to print the document"));
      g_error_free (error);
    }

  /* release the object */
  g_object_unref (G_OBJECT (print));
}



static void
mousepad_window_action_detach (GtkAction      *action,
                               MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

#if GTK_CHECK_VERSION (2,12,0)
  /* invoke function without cooridinates */
  mousepad_window_notebook_create_window (GTK_NOTEBOOK (window->notebook),
                                          GTK_WIDGET (window->active),
                                          -1, -1, window);
#else
  /* only detach when there are more then 2 tabs */
  if (G_LIKELY (gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)) >= 2))
    {
      /* take a reference */
      g_object_ref (G_OBJECT (window->active));

      /* remove the document from the active window */
      gtk_container_remove (GTK_CONTAINER (window->notebook), GTK_WIDGET (window->active));

      /* emit the new window with document signal */
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW_WITH_DOCUMENT], 0, window->active, -1, -1);

      /* release our reference */
      g_object_unref (G_OBJECT (window->active));
    }
#endif
}



static void
mousepad_window_action_close (GtkAction      *action,
                              MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* close active document */
  mousepad_window_close_document (window, window->active);
}



static void
mousepad_window_action_close_window (GtkAction      *action,
                                     MousepadWindow *window)
{
  gint       npages, i;
  GtkWidget *document;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get the number of page in the notebook */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)) - 1;

  /* prevent menu updates */
  lock_menu_updates++;

  /* ask what to do with the modified document in this window */
  for (i = npages; i >= 0; --i)
    {
      /* get the document */
      document = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i);

      /* check for debug builds */
      _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));

      /* focus the tab we're going to close */
      gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), i);

      /* close each document */
      if (!mousepad_window_close_document (window, MOUSEPAD_DOCUMENT (document)))
        {
          /* closing cancelled, release menu lock */
          lock_menu_updates--;

          /* rebuild go menu */
          mousepad_window_update_gomenu (window);

          /* leave function */
          return;
        }
    }

  /* release lock */
  lock_menu_updates--;
}



static void
mousepad_window_action_undo (GtkAction      *action,
                             MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* undo */
  mousepad_undo_do_undo (window->active->undo);

  /* scroll to visible area */
  mousepad_view_scroll_to_cursor (window->active->textview);
}



static void
mousepad_window_action_redo (GtkAction      *action,
                             MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* redo */
  mousepad_undo_do_redo (window->active->undo);

  /* scroll to visible area */
  mousepad_view_scroll_to_cursor (window->active->textview);
}



static void
mousepad_window_action_cut (GtkAction      *action,
                            MousepadWindow *window)
{
  GtkEditable *entry;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get searchbar entry */
  entry = mousepad_search_bar_entry (MOUSEPAD_SEARCH_BAR (window->search_bar));

  /* cut from search bar entry or textview */
  if (G_UNLIKELY (entry))
    gtk_editable_cut_clipboard (entry);
  else
    mousepad_view_clipboard_cut (window->active->textview);

  /* update the history */
  mousepad_window_paste_history_add (window);
}



static void
mousepad_window_action_copy (GtkAction      *action,
                             MousepadWindow *window)
{
  GtkEditable *entry;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get searchbar entry */
  entry = mousepad_search_bar_entry (MOUSEPAD_SEARCH_BAR (window->search_bar));

  /* copy from search bar entry or textview */
  if (G_UNLIKELY (entry))
    gtk_editable_copy_clipboard (entry);
  else
    mousepad_view_clipboard_copy (window->active->textview);

  /* update the history */
  mousepad_window_paste_history_add (window);
}



static void
mousepad_window_action_paste (GtkAction      *action,
                              MousepadWindow *window)
{
  GtkEditable *entry;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get searchbar entry */
  entry = mousepad_search_bar_entry (MOUSEPAD_SEARCH_BAR (window->search_bar));

  /* paste in search bar entry or textview */
  if (G_UNLIKELY (entry))
    gtk_editable_paste_clipboard (entry);
  else
    mousepad_view_clipboard_paste (window->active->textview, NULL, FALSE);
}



static void
mousepad_window_action_paste_history (GtkAction      *action,
                                      MousepadWindow *window)
{
  GtkWidget *menu;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get the history menu */
  menu = mousepad_window_paste_history_menu (window);

  /* select the first item in the menu */
  gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), TRUE);

  /* popup the menu */
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                  mousepad_window_paste_history_menu_position,
                  window, 0, gtk_get_current_event_time ());
}



static void
mousepad_window_action_paste_column (GtkAction      *action,
                                     MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* paste the clipboard into a column */
  mousepad_view_clipboard_paste (window->active->textview, NULL, TRUE);
}



static void
mousepad_window_action_delete (GtkAction      *action,
                               MousepadWindow *window)
{
  GtkEditable *entry;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* get searchbar entry */
  entry = mousepad_search_bar_entry (MOUSEPAD_SEARCH_BAR (window->search_bar));

  /* delete selection in search bar entry or textview */
  if (G_UNLIKELY (entry))
    gtk_editable_delete_selection (entry);
  else
    mousepad_view_delete_selection (window->active->textview);
}



static void
mousepad_window_action_select_all (GtkAction      *action,
                                   MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* select everything in the document */
  mousepad_view_select_all (window->active->textview);
}



static void
mousepad_window_action_change_selection (GtkAction      *action,
                                         MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* change the selection */
  mousepad_view_change_selection (window->active->textview);
}



static void
mousepad_window_action_find (GtkAction      *action,
                             MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* create a new search bar is needed */
  if (window->search_bar == NULL)
    {
      /* create a new toolbar and pack it into the box */
      window->search_bar = mousepad_search_bar_new ();
      gtk_box_pack_start (GTK_BOX (window->box), window->search_bar, FALSE, FALSE, PADDING);

      /* connect signals */
      g_signal_connect_swapped (G_OBJECT (window->search_bar), "hide-bar", G_CALLBACK (mousepad_window_hide_search_bar), window);
      g_signal_connect_swapped (G_OBJECT (window->search_bar), "search", G_CALLBACK (mousepad_window_search), window);
    }

  /* show the search bar */
  gtk_widget_show (window->search_bar);

  /* focus the search entry */
  mousepad_search_bar_focus (MOUSEPAD_SEARCH_BAR (window->search_bar));
}



static void
mousepad_window_action_find_next (GtkAction      *action,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* find the next occurence */
  if (G_LIKELY (window->search_bar != NULL))
    mousepad_search_bar_find_next (MOUSEPAD_SEARCH_BAR (window->search_bar));
}



static void
mousepad_window_action_find_previous (GtkAction      *action,
                                      MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* find the previous occurence */
  if (G_LIKELY (window->search_bar != NULL))
    mousepad_search_bar_find_previous (MOUSEPAD_SEARCH_BAR (window->search_bar));
}


static void
mousepad_window_action_replace_switch_page (MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_REPLACE_DIALOG (window->replace_dialog));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* page switched */
  mousepad_replace_dialog_page_switched (MOUSEPAD_REPLACE_DIALOG (window->replace_dialog));
}


static void
mousepad_window_action_replace_destroy (MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* disconnect tab switch signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (window->notebook), mousepad_window_action_replace_switch_page, window);

  /* reset the dialog variable */
  window->replace_dialog = NULL;
}


static void
mousepad_window_action_replace (GtkAction      *action,
                                MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  if (window->replace_dialog == NULL)
    {
      /* create a new dialog */
      window->replace_dialog = mousepad_replace_dialog_new ();

      /* popup the dialog */
      gtk_window_set_destroy_with_parent (GTK_WINDOW (window->replace_dialog), TRUE);
      gtk_window_set_transient_for (GTK_WINDOW (window->replace_dialog), GTK_WINDOW (window));
      gtk_widget_show (window->replace_dialog);

      /* connect signals */
      g_signal_connect_swapped (G_OBJECT (window->replace_dialog), "destroy", G_CALLBACK (mousepad_window_action_replace_destroy), window);
      g_signal_connect_swapped (G_OBJECT (window->replace_dialog), "search", G_CALLBACK (mousepad_window_search), window);
      g_signal_connect_swapped (G_OBJECT (window->notebook), "switch-page", G_CALLBACK (mousepad_window_action_replace_switch_page), window);
    }
  else
    {
      /* focus the existing dialog */
      gtk_window_present (GTK_WINDOW (window->replace_dialog));
    }
}



static void
mousepad_window_action_select_font (GtkAction      *action,
                                    MousepadWindow *window)
{
  GtkWidget        *dialog;
  MousepadDocument *document;
  gchar            *font_name;
  guint             i;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  dialog = gtk_font_selection_dialog_new (_("Choose Mousepad Font"));
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));

  /* set the current font name */
  g_object_get (G_OBJECT (window->preferences), "view-font-name", &font_name, NULL);
  if (G_LIKELY (font_name))
    {
      gtk_font_selection_dialog_set_font_name (GTK_FONT_SELECTION_DIALOG (dialog), font_name);
      g_free (font_name);
    }

  /* run the dialog */
  if (G_LIKELY (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK))
    {
      /* get the selected font from the dialog */
      font_name = gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (dialog));

      /* store the font in the preferences */
      g_object_set (G_OBJECT (window->preferences), "view-font-name", font_name, NULL);

      /* apply the font in all documents in this window */
      for (i = 0; i < gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)); i++)
        {
          /* get the document */
          document = MOUSEPAD_DOCUMENT (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i));

          /* debug check */
          _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (document));

          /* set the font */
          mousepad_document_set_font (document, font_name);

          /* update the tab array */
          mousepad_view_set_tab_size (document->textview, mousepad_view_get_tab_size (document->textview));
        }

      /* cleanup */
      g_free (font_name);
    }

  /* destroy dialog */
  gtk_widget_destroy (dialog);
}



static void
mousepad_window_action_statusbar_overwrite (MousepadWindow *window,
                                            gboolean        overwrite)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* set the new overwrite mode */
  mousepad_document_set_overwrite (window->active, overwrite);
}



static void
mousepad_window_action_statusbar (GtkToggleAction *action,
                                  MousepadWindow  *window)
{
  gboolean show_statusbar;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* whether we show the statusbar */
  show_statusbar = gtk_toggle_action_get_active (action);

  /* check if we should drop the statusbar */
  if (!show_statusbar && window->statusbar != NULL)
    {
      /* destroy the statusbar */
      gtk_widget_destroy (window->statusbar);
      window->statusbar = NULL;
    }
  else if (show_statusbar && window->statusbar == NULL)
    {
      /* setup a new statusbar */
      window->statusbar = mousepad_statusbar_new ();
      gtk_box_pack_end (GTK_BOX (window->box), window->statusbar, FALSE, FALSE, 0);
      gtk_widget_show (window->statusbar);

      /* overwrite toggle signal */
      g_signal_connect_swapped (G_OBJECT (window->statusbar), "enable-overwrite",
                                G_CALLBACK (mousepad_window_action_statusbar_overwrite), window);

      /* update the statusbar items */
      if (window->active)
        {
          /* debug check */
          _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

          /* ask document to resend the cursor status signals */
          mousepad_document_send_signals (window->active);
        }
    }

  /* remember the setting */
  g_object_set (G_OBJECT (window->preferences), "window-statusbar-visible", show_statusbar, NULL);
}



static void
mousepad_window_action_lowercase (GtkAction      *action,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert selection to lowercase */
  mousepad_view_convert_selection_case (window->active->textview, LOWERCASE);
}



static void
mousepad_window_action_uppercase (GtkAction      *action,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert selection to uppercase */
  mousepad_view_convert_selection_case (window->active->textview, UPPERCASE);
}



static void
mousepad_window_action_titlecase (GtkAction      *action,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert selection to titlecase */
  mousepad_view_convert_selection_case (window->active->textview, TITLECASE);
}



static void
mousepad_window_action_opposite_case (GtkAction      *action,
                                      MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert selection to opposite case */
  mousepad_view_convert_selection_case (window->active->textview, OPPOSITE_CASE);
}



static void
mousepad_window_action_tabs_to_spaces (GtkAction      *action,
                                       MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert tabs to spaces */
  mousepad_view_convert_spaces_and_tabs (window->active->textview, TABS_TO_SPACES);
}



static void
mousepad_window_action_spaces_to_tabs (GtkAction      *action,
                                       MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert spaces to tabs */
  mousepad_view_convert_spaces_and_tabs (window->active->textview, SPACES_TO_TABS);
}



static void
mousepad_window_action_strip_trailing_spaces (GtkAction      *action,
                                              MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* convert spaces to tabs */
  mousepad_view_strip_trailing_spaces (window->active->textview);
}



static void
mousepad_window_action_transpose (GtkAction      *action,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* transpose */
  mousepad_view_transpose (window->active->textview);
}



static void
mousepad_window_action_move_line_up (GtkAction      *action,
                                     MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* move the selection on line up */
  mousepad_view_move_selection (window->active->textview, MOVE_LINE_UP);
}



static void
mousepad_window_action_move_line_down (GtkAction      *action,
                                       MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* move the selection on line down */
  mousepad_view_move_selection (window->active->textview, MOVE_LINE_DOWN);
}



static void
mousepad_window_action_duplicate (GtkAction      *action,
                                  MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* dupplicate */
  mousepad_view_duplicate (window->active->textview);
}



static void
mousepad_window_action_increase_indent (GtkAction      *action,
                                        MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* increase the indent */
  mousepad_view_indent (window->active->textview, INCREASE_INDENT);
}



static void
mousepad_window_action_decrease_indent (GtkAction      *action,
                                        MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* decrease the indent */
  mousepad_view_indent (window->active->textview, DECREASE_INDENT);
}



static void
mousepad_window_action_line_numbers (GtkToggleAction *action,
                                     MousepadWindow  *window)
{
  gboolean active;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* leave when menu updates are locked */
  if (lock_menu_updates == 0)
    {
      /* get the current state */
      active = gtk_toggle_action_get_active (action);

      /* save as the last used line number setting */
      g_object_set (G_OBJECT (window->preferences), "view-line-numbers", active, NULL);

      /* update the active document */
      mousepad_view_set_line_numbers (window->active->textview, active);
    }
}



static void
mousepad_window_action_word_wrap (GtkToggleAction *action,
                                  MousepadWindow  *window)
{
  gboolean active;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* leave when menu updates are locked */
  if (lock_menu_updates == 0)
    {
      /* get the current state */
      active = gtk_toggle_action_get_active (action);

      /* store this as the last used wrap mode */
      g_object_set (G_OBJECT (window->preferences), "view-word-wrap", active, NULL);

      /* set the wrapping mode of the current document */
      mousepad_document_set_word_wrap (window->active, active);
    }
}



static void
mousepad_window_action_auto_indent (GtkToggleAction *action,
                                    MousepadWindow  *window)
{
  gboolean active;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* leave when menu updates are locked */
  if (lock_menu_updates == 0)
    {
      /* get the current state */
      active = gtk_toggle_action_get_active (action);

      /* save as the last auto indent mode */
      g_object_set (G_OBJECT (window->preferences), "view-auto-indent", active, NULL);

      /* update the active document */
      mousepad_view_set_auto_indent (window->active->textview, active);
    }
}



static void
mousepad_window_action_tab_size (GtkToggleAction *action,
                                 MousepadWindow  *window)
{
  gboolean tab_size;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* leave when menu updates are locked */
  if (lock_menu_updates == 0 && gtk_toggle_action_get_active (action))
    {
      _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

      /* get the tab size */
      tab_size = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

      /* whether the other item was clicked */
      if (tab_size == 0)
        {
          /* get tab size from document */
          tab_size = mousepad_view_get_tab_size (window->active->textview);

          /* select other size in dialog */
          tab_size = mousepad_dialogs_other_tab_size (GTK_WINDOW (window), tab_size);
        }

      /* store as last used value */
      g_object_set (G_OBJECT (window->preferences), "view-tab-size", tab_size, NULL);

      /* set the value */
      mousepad_view_set_tab_size (window->active->textview, tab_size);

      /* update menu */
      mousepad_window_menu_tab_sizes_update (window);
    }
}



static void
mousepad_window_action_insert_spaces (GtkToggleAction *action,
                                      MousepadWindow  *window)
{
  gboolean insert_spaces;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));

  /* leave when menu updates are locked */
  if (lock_menu_updates == 0)
    {
      /* get the current state */
      insert_spaces = gtk_toggle_action_get_active (action);

      /* save as the last auto indent mode */
      g_object_set (G_OBJECT (window->preferences), "view-insert-spaces", insert_spaces, NULL);

      /* update the active document */
      mousepad_view_set_insert_spaces (window->active->textview, insert_spaces);
    }
}



static void
mousepad_window_action_line_ending (GtkRadioAction *action,
                                    GtkRadioAction *current,
                                    MousepadWindow *window)
{
  MousepadLineEnding eol;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));
  _mousepad_return_if_fail (MOUSEPAD_IS_FILE (window->active->file));
  _mousepad_return_if_fail (GTK_IS_TEXT_BUFFER (window->active->buffer));

  /* leave when menu updates are locked */
  if (lock_menu_updates == 0)
    {
      /* get selected line ending */
      eol = gtk_radio_action_get_current_value (current);

      /* set the new line ending on the file */
      mousepad_file_set_line_ending (window->active->file, eol);

      /* make buffer as modified to show the user the change is not saved */
      gtk_text_buffer_set_modified (window->active->buffer, TRUE);
    }
}




static void
mousepad_window_action_prev_tab (GtkAction      *action,
                                 MousepadWindow *window)
{
  gint page_num;
  gint n_pages;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* get notebook info */
  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));
  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

  /* switch to the previous tab or cycle to the last tab */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), (page_num - 1) % n_pages);
}



static void
mousepad_window_action_next_tab (GtkAction      *action,
                                 MousepadWindow *window)
{
  gint page_num;
  gint n_pages;

  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* get notebook info */
  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));
  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

  /* switch to the next tab or cycle to the first tab */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), (page_num + 1) % n_pages);
}



static void
mousepad_window_action_go_to_tab (GtkRadioAction *action,
                                  GtkNotebook    *notebook)
{
  gint page;

  _mousepad_return_if_fail (GTK_IS_NOTEBOOK (notebook));
  _mousepad_return_if_fail (GTK_IS_RADIO_ACTION (action));
  _mousepad_return_if_fail (GTK_IS_TOGGLE_ACTION (action));

  /* leave when the menu is locked or this is not the active radio button */
  if (lock_menu_updates == 0
      && gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    {
      /* get the page number from the action value */
      page = gtk_radio_action_get_current_value (action);

      /* set the page */
      gtk_notebook_set_current_page (notebook, page);
    }
}



static void
mousepad_window_action_go_to_position (GtkAction      *action,
                                       MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));
  _mousepad_return_if_fail (MOUSEPAD_IS_DOCUMENT (window->active));
  _mousepad_return_if_fail (GTK_IS_TEXT_BUFFER (window->active->buffer));

  /* run jump dialog */
  if (mousepad_dialogs_go_to (GTK_WINDOW (window), window->active->buffer))
    {
      /* put the cursor on screen */
      mousepad_view_scroll_to_cursor (window->active->textview);
    }
}



static void
mousepad_window_action_contents (GtkAction      *action,
                                 MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* show help */
  mousepad_dialogs_show_help (GTK_WINDOW (window), NULL, NULL);
}



static void
mousepad_window_action_about (GtkAction      *action,
                              MousepadWindow *window)
{
  _mousepad_return_if_fail (MOUSEPAD_IS_WINDOW (window));

  /* show about dialog */
  mousepad_dialogs_show_about (GTK_WINDOW (window));
}