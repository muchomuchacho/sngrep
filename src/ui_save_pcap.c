/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2014 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file ui_save_pcap.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_save_pcap.c
 */

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <form.h>
#include <ctype.h>
#include "ui_call_list.h"
#include "ui_save_pcap.h"
#include "option.h"
#include "capture.h"
#include "filter.h"

/**
 * Ui Structure definition for Save panel
 */
ui_t ui_save = {
    .type = PANEL_SAVE,
    .panel = NULL,
    .create = save_create,
    .draw = save_draw,
    .handle_key = save_handle_key,
    .destroy = save_destroy
};

PANEL *
save_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    save_info_t *info;
    char savefile[128];

    // Calculate window dimensions
    height = 11;
    width = 90;

    // Cerate a new indow for the panel and form
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize save panel specific data
    info = malloc(sizeof(save_info_t));
    memset(info, 0, sizeof(save_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Initialize the fields
    info->fields[FLD_SAVE_FILE] = new_field(1, 68, 3, 15, 0, 0);
    info->fields[FLD_SAVE_ALL] = new_field(1, 1, 5, 4, 0, 0);
    info->fields[FLD_SAVE_SELECTED] = new_field(1, 1, 6, 4, 0, 0);
    info->fields[FLD_SAVE_DISPLAYED] = new_field(1, 1, 7, 4, 0, 0);
    info->fields[FLD_SAVE_SAVE] = new_field(1, 10, height - 2, 30, 0, 0);
    info->fields[FLD_SAVE_CANCEL] = new_field(1, 10, height - 2, 50, 0, 0);
    info->fields[FLD_SAVE_COUNT] = NULL;

    // Set fields options
    field_opts_off(info->fields[FLD_SAVE_FILE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_ALL], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_SELECTED], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_DISPLAYED], O_AUTOSKIP);

    // Change background of input fields
    set_field_back(info->fields[FLD_SAVE_FILE], A_UNDERLINE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, win);
    post_form(info->form);

    // Fields labels
    mvwprintw(win, 3, 3, "Save file:");

    // Set Default field values
    sprintf(savefile, "%s/sngrep-capture-%u.pcap", get_option_value("sngrep.savepath"),
            (unsigned) time(NULL));

    set_field_buffer(info->fields[FLD_SAVE_FILE], 0, savefile);
    set_field_buffer(info->fields[FLD_SAVE_SAVE], 0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_SAVE_CANCEL], 0, "[ Cancel ]");

    // Set the window title and boxes
    mvwprintw(win, 1, 32, "Save capture to file");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel_window(panel));
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 1);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_SAVE_FILE]);
    form_driver(info->form, REQ_END_LINE);
    curs_set(1);

    // Pause the capture while saving
    capture_set_paused(1);

    return panel;
}

void
save_destroy(PANEL *panel)
{
    // Unpause capture
    capture_set_paused(0);

    // Disable cursor position
    curs_set(0);
}

int
save_draw(PANEL *panel)
{
    int total, displayed;

    // Get panel information
    save_info_t *info = (save_info_t*) panel_userptr(panel);
    WINDOW *win = panel_window(panel);

    // Get filter stats
    filter_stats(&total, &displayed);

    mvwprintw(win, 5, 3, "( ) Save all dialogs");
    mvwprintw(win, 6, 3, "( ) Save selected dialogs (%d dialogs)",
              call_group_count(info->group));
    mvwprintw(win, 7, 3, "( ) Save displayed dialogs (%d dialogs)", displayed);

    set_field_buffer(info->fields[FLD_SAVE_ALL], 0,
                     (info->savemode == SAVE_ALL) ? "*" : "");
    set_field_buffer(info->fields[FLD_SAVE_SELECTED], 0,
                     (info->savemode == SAVE_SELECTED) ? "*" : "");
    set_field_buffer(info->fields[FLD_SAVE_DISPLAYED], 0,
                     (info->savemode == SAVE_DISPLAYED) ? "*" : "");

    set_current_field(info->form, current_field(info->form));
    form_driver(info->form, REQ_VALIDATION);

    return 0;
}

int
save_handle_key(PANEL *panel, int key)
{
    int field_idx;
    char field_value[48];
    int action = -1;

    // Get panel information
    save_info_t *info = (save_info_t*) panel_userptr(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Get current field value.
    // We trim spaces with sscanf because and empty field is stored as
    // space characters
    memset(field_value, 0, sizeof(field_value));
    sscanf(field_buffer(current_field(info->form), 0), "%[^ ]", field_value);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                if (field_idx == FLD_SAVE_FILE)
                    form_driver(info->form, key);
                break;
            case ACTION_NEXT_FIELD:
                form_driver(info->form, REQ_NEXT_FIELD);
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_PREV_FIELD:
                form_driver(info->form, REQ_PREV_FIELD);
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_RIGHT:
                form_driver(info->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(info->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(info->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_DELETE:
                form_driver(info->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                if (strlen(field_value) > 0)
                    form_driver(info->form, REQ_DEL_PREV);
                break;
            case ACTION_CLEAR:
                form_driver(info->form, REQ_CLR_FIELD);
                break;
            case ACTION_SELECT:
                switch (field_idx) {
                    case FLD_SAVE_ALL:
                        info->savemode = SAVE_ALL;
                        break;
                    case FLD_SAVE_SELECTED:
                        info->savemode = SAVE_SELECTED;
                        break;
                    case FLD_SAVE_DISPLAYED:
                        info->savemode = SAVE_DISPLAYED;
                        break;
                    case FLD_SAVE_FILE:
                        form_driver(info->form, key);
                        break;
                    default:
                        break;
                }
                break;
            case ACTION_CONFIRM:
                if (field_idx != FLD_SAVE_CANCEL) {
                    if (!strcasecmp(field_value, "")) {
                        save_error_message(panel, "Invalid filename");
                        return 0;
                    }
                    return save_to_file(panel);
                }
                return KEY_ESC;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    // Change background and cursor of "button fields"
    set_field_back(info->fields[FLD_SAVE_SAVE], A_NORMAL);
    set_field_back(info->fields[FLD_SAVE_CANCEL], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(info->form));
    if (field_idx == FLD_SAVE_SAVE || field_idx == FLD_SAVE_CANCEL) {
        set_field_back(info->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

void
save_set_group(PANEL *panel, sip_call_group_t *group)
{
    // Get panel information
    save_info_t *info = (save_info_t*) panel_userptr(panel);
    info->group = group;
}

void
save_error_message(PANEL *panel, const char *message)
{
    WINDOW *win = panel_window(panel);
    mvwprintw(win, 4, 3, "Error: %s", message);
    wmove(win, 3, 15);
}

int
save_to_file(PANEL *panel)
{
    char field_value[256];
    sip_call_t *call = NULL;
    sip_msg_t *msg = NULL;
    int i;

    // Get panel information
    save_info_t *info = (save_info_t*) panel_userptr(panel);

    // Get current field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(info->fields[FLD_SAVE_FILE], 0));
    // Trim trailing spaces
    for (i = strlen(field_value) - 1; isspace(field_value[i]); i--)
        field_value[i] = '\0';

    // Don't allow to save no packets!
    if (info->savemode == SAVE_SELECTED && call_group_msg_count(info->group) == 0) {
        save_error_message(panel, "Unable to save no packets to selected file.");
        return 1;
    }

    // Open dump file
    pcap_dumper_t *pd = dump_open(field_value);
    if (access(field_value, W_OK) != 0) {
        save_error_message(panel, capture_last_error());
        return 1;
    }

    switch(info->savemode) {
        case SAVE_ALL:
            // Save all packets to the file
            while ((call = call_get_next(call))) {
                while ((msg = call_get_next_msg(call, msg))) {
                    dump_packet(pd, msg->pcap_header, msg->pcap_packet);
                }
            }
            break;
        case SAVE_SELECTED:
            // Save selected packets to file
            while ((call = call_group_get_next(info->group, call))) {
                while ((msg = call_get_next_msg(call, msg))) {
                    dump_packet(pd, msg->pcap_header, msg->pcap_packet);
                }
            }
            break;
        case SAVE_DISPLAYED:
            // Save selected packets to file
            while ((call = call_get_next_filtered(call))) {
                while ((msg = call_get_next_msg(call, msg))) {
                    dump_packet(pd, msg->pcap_header, msg->pcap_packet);
                }
            }
            break;
    }

    // Close dump file
    dump_close(pd);

    return 27;
}

