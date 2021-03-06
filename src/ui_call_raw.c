/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 ** == 1)
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
 * @file ui_call_raw.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_raw.h
 *
 * @todo Code help screen. Please.
 * @todo Replace the panel refresh. Wclear sucks on high latency conections.
 *
 */
#include <string.h>
#include <stdlib.h>
#include "ui_manager.h"
#include "ui_call_raw.h"
#include "ui_save_raw.h"
#include "option.h"
#include "capture.h"

/**
 * Ui Structure definition for Call Raw panel
 */
ui_t ui_call_raw = {
    .type = PANEL_CALL_RAW,
    .panel = NULL,
    .create = call_raw_create,
    .draw = call_raw_draw,
    .handle_key = call_raw_handle_key
};

PANEL *
call_raw_create()
{
    PANEL *panel;
    call_raw_info_t *info;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));

    // Initialize Call List specific data
    info = malloc(sizeof(call_raw_info_t));
    memset(info, 0, sizeof(call_raw_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Create a initial pad of 1000 lines
    info->pad = newpad(500, COLS);
    info->padline = 0;
    info->scroll = 0;

    return panel;
}

int
call_raw_draw(PANEL *panel)
{
    sip_msg_t *msg = NULL;

    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);

    if (info->group) {
        // Print the call group messages into the pad
        while ((msg = call_group_get_next_msg(info->group, info->last)))
            call_raw_print_msg(panel, msg);
    } else {
        call_raw_set_msg(info->msg);
    }

    // Copy the visible part of the pad into the panel window
    copywin(info->pad, panel_window(panel), info->scroll, 0, 0, 0, LINES - 1, COLS - 1, 0);
    return 0;
}

int
call_raw_print_msg(PANEL *panel, sip_msg_t *msg)
{
    int payload_lines, i, column, height, width;
    // Message ngrep style Header
    char header[256];

    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);

    // Get the pad window
    WINDOW *pad = info->pad;

    // Get current pad dimensions
    getmaxyx(pad, height, width);

    // Check how many lines we well need to draw this message
    payload_lines = 0;
    column = 0;
    for (i = 0; i < strlen(msg->payload); i++) {
        if (column == width || msg->payload[i] == '\n') {
            payload_lines++;
            column = 0;
            continue;
        }
        column++;
    }

    // Check if we have enough space in our huge pad to store this message
    if (info->padline + payload_lines > height) {
        // Delete previous pad
        delwin(info->pad);
        // Create a new pad with more lines!
        pad = newpad(height + 500, COLS);
        // And copy all previous information
        overwrite(info->pad, pad);
        info->pad = pad;
    }

    // Color the message {
    if (is_option_enabled("color.request")) {
        // Determine arrow color
        if (msg_is_request(msg)) {
            msg->color = CP_RED_ON_DEF;
        } else {
            msg->color = CP_GREEN_ON_DEF;
        }
    } else if (info->group && is_option_enabled("color.callid")) {
        // Color by call-id
        msg->color = call_group_color(info->group, msg->call);
    } else if (is_option_enabled("color.cseq")) {
        // Color by CSeq within the same call
        msg->color = msg->cseq % 7 + 1;
    }

    // Turn on the message color
    wattron(pad, COLOR_PAIR(msg->color));

    // Print msg header
    wattron(pad, A_BOLD);
    mvwprintw(pad, info->padline++, 0, "%s", msg_get_header(msg, header));
    wattroff(pad, A_BOLD);

    // Print msg payload
    info->padline += draw_message_pos(pad, msg, info->padline);
    // Extra line between messages
    info->padline++;

    // Set this as the last printed message
    info->last = msg;

    return 0;
}

int
call_raw_handle_key(PANEL *panel, int key)
{
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);
    ui_t *next_panel;
    int rnpag_steps = get_option_int_value("cr.scrollstep");
    int action = -1;

    // Sanity check, this should not happen
    if (!info)
        return -1;

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                info->scroll++;
                break;
            case ACTION_UP:
                info->scroll--;
                break;
            case ACTION_HNPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_NPAGE:
                // Next page => N key down strokes
                info->scroll += rnpag_steps;
                break;
            case ACTION_HPPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_PPAGE:
                // Prev page => N key up strokes
                info->scroll -= rnpag_steps;
                break;
            case ACTION_SHOW_HOSTNAMES:
                // Tooggle Host/Address display
                toggle_option("sngrep.displayhost");
                // Force refresh panel
                if (info->group) {
                    call_raw_set_group(info->group);
                } else {
                    call_raw_set_msg(info->msg);
                }
                break;
            case ACTION_SAVE:
                if (info->group) {
                    // KEY_S, Display save panel
                    next_panel = ui_create(ui_find_by_type(PANEL_SAVE_RAW));
                    save_raw_set_group(next_panel->panel, info->group);
                    wait_for_input(next_panel);
                }
                break;
            case ACTION_TOGGLE_SYNTAX:
            case ACTION_CYCLE_COLOR:
                // Handle colors using default handler
                default_handle_key(ui_find_by_panel(panel), key);
                // Create a new pad (forces messages draw)
                delwin(info->pad);
                info->pad = newpad(500, COLS);
                info->last = NULL;
                // Force refresh panel
                if (info->group) {
                    call_raw_set_group(info->group);
                } else {
                    call_raw_set_msg(info->msg);
                }
                break;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    if (info->scroll < 0 || info->padline < LINES) {
        info->scroll = 0;   // Disable scrolling if there's nothing to scroll
    } else {
        if (info->scroll + LINES / 2 > info->padline)
            info->scroll = info->padline - LINES / 2;
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

int
call_raw_set_group(sip_call_group_t *group)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!group)
        return -1;

    if (!(raw_panel = ui_find_by_type(PANEL_CALL_RAW)))
        return -1;

    if (!(panel = raw_panel->panel))
        return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel)))
        return -1;

    // Set call raw call group
    info->group = group;
    info->msg = NULL;

    // Initialize internal pad
    info->padline = 0;
    wclear(info->pad);

    return 0;
}

int
call_raw_set_msg(sip_msg_t *msg)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!msg)
        return -1;

    if (!(raw_panel = ui_find_by_type(PANEL_CALL_RAW)))
        return -1;

    if (!(panel = raw_panel->panel))
        return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel)))
        return -1;

    // Set call raw message
    info->group = NULL;
    info->msg = msg;

    // Initialize internal pad
    info->padline = 0;
    wclear(info->pad);

    // Print the message in the pad
    call_raw_print_msg(panel, msg);

    return 0;

}
