/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file ui_call_media.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_media.h
 *
 */
#include <stdlib.h>
#include <string.h>
#include "ui_call_media.h"
#include "group.h"
#include "option.h"

/***
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------+
 * |            Title               |
 * |   addr1  addr2  addr3  addr4   |
 * |   -----  -----  -----  -----   |
 * |     |      |      |      |     |
 * | port|----->|port  |      |     |
 * |     |  port|<---->|port  |     |
 * | port|<------------|port  |     |
 * |     |      | port |      |     |
 * |     |      |      |      |port |
 * |     |      |      |      |     |
 * | Usefull hotkeys                |
 * +--------------------------------+
 *
 */

/**
 * Ui Structure definition for Call Flow panel
 */
ui_t ui_call_media =
    {
      .type = PANEL_CALL_MEDIA,
      .panel = NULL,
      .create = call_media_create,
      .draw = call_media_draw,
      .handle_key = call_media_handle_key,
      .help = call_media_help };

PANEL *
call_media_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    call_media_info_t *info;

    // Calculate window dimensions
    height = 31;
    width = 90;

    // Create a new panel to fill all the screen
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize Call List specific data
    info = malloc(sizeof(call_media_info_t));
    memset(info, 0, sizeof(call_media_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Draw a box around the panel window
    title_foot_box(win);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    return panel;
}

call_media_info_t *
call_media_info(PANEL *panel)
{
    return (call_media_info_t*) panel_userptr(panel);
}

void
call_media_destroy(PANEL *panel)
{
}

int
call_media_draw(PANEL *panel)
{
    /*
    int height, width;
    char *coltext;

    WINDOW *win = panel_window(panel);
    getyx(win, height, width);


     mvwvline(win, 3, 15 + 30 * 0, ACS_VLINE, height - 5);
     mvwhline(win, 4, 5 + 30 * 0, ACS_HLINE, 20);
     mvwaddch(win, 4, 15 + 30 * 0, ACS_TTEE);
     coltext = "192.168.8.110";
     mvwprintw(win, 3, 5 + 30 * 0 + (22 - strlen(coltext)) / 2, "%s", coltext);

     mvwvline(win, 3, 15 + 30 * 1, ACS_VLINE, height - 5);
     mvwhline(win, 4, 5 + 30 * 1, ACS_HLINE, 20);
     mvwaddch(win, 4, 15 + 30 * 1, ACS_TTEE);
     coltext = "10.10.1.2";
     mvwprintw(win, 3, 5 + 30 * 1 + (22 - strlen(coltext)) / 2, "%s", coltext);

     mvwvline(win, 3, 15 + 30 * 2, ACS_VLINE, height - 5);
     mvwhline(win, 4, 5 + 30 * 2, ACS_HLINE, 20);
     mvwaddch(win, 4, 15 + 30 * 2, ACS_TTEE);
     coltext = "10.10.1.142";
     mvwprintw(win, 3, 5 + 30 * 2 + (22 - strlen(coltext)) / 2, "%s", coltext);


     wattron(win, COLOR_PAIR(CP_RED_ON_DEF)); mvwprintw(win, 5, 16, "512");  wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
     wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF)); mvwprintw(win, 5, 20, "512");  wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
     wattron(win, COLOR_PAIR(CP_RED_ON_DEF)); mvwprintw(win, 5, 38, "512");  wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
     wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF)); mvwprintw(win, 5, 42, "512");  wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
     mvwprintw(win, 6, 11, "3591");
     mvwhline(win, 6, 16, ACS_HLINE, 29);
     mvwprintw(win, 6, 16, "<");
     mvwprintw(win, 6, 44, ">");
     mvwprintw(win, 6, 46, "14590");


     wattron(win, COLOR_PAIR(CP_RED_ON_DEF)); mvwprintw(win, 7, 46, "1080");  wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
     wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF)); mvwprintw(win, 7, 51, "1070");  wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
     wattron(win, COLOR_PAIR(CP_RED_ON_DEF)); mvwprintw(win, 7, 66, "1080");  wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
     wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF)); mvwprintw(win, 7, 71, "1070");  wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
     mvwprintw(win, 8, 40, "14612");
     mvwhline(win, 8, 46, ACS_HLINE, 29);
     mvwprintw(win, 8, 46, "<");
     mvwprintw(win, 8, 74, ">");
     mvwprintw(win, 8, 76, "17800");

     mvwprintw(win, 10, 11, "3591");
     mvwhline(win, 10, 16, '~', 59);
     mvwprintw(win, 10, 16, "<");
     mvwprintw(win, 10, 74, ">");
     mvwprintw(win, 10, 76, "17800");
     */

    WINDOW *win = panel_window(panel);
    call_media_info_t *info;
    int rowpos = 0;

    if (!(info = call_media_info(panel)))
        return 1;

    sip_call_t *call = NULL;
    sdp_media_t *m;
    while ((call = call_group_get_next(info->group, call))) {
        mvwprintw(win, rowpos++, 3, "%s", "ola que ase");

        for (m = call->medias; m; m = m->next) {
            mvwprintw(win, rowpos++, 3, "%s:%d", m->address, m->port);
        }
    }

    return 0;
}

int
call_media_handle_key(PANEL *panel, int key)
{
    // Return if this panel has handled or not the key
    return key;
}

int
call_media_help(PANEL *panel)
{
    return 0;
}


int
call_media_set_group(sip_call_group_t *group)
{
    call_media_info_t *info;

    if (!ui_call_media.panel)
        return -1;

    if (!(info = call_media_info(ui_call_media.panel)))
        return -1;

    info->group = group;

    return 0;
}
