/*
    WDG -- window widget

    Copyright (C) ALoR

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: wdg_window.c,v 1.3 2003/10/25 21:57:42 alor Exp $
*/

#include <wdg.h>

#include <ncurses.h>
#include <stdarg.h>

/* GLOBALS */

struct wdg_window {
   WINDOW *win;
   WINDOW *sub;
   char *title;
   char align;
};

/* PROTOS */

void wdg_create_window(struct wdg_object *wo);

static int wdg_window_destroy(struct wdg_object *wo);
static int wdg_window_resize(struct wdg_object *wo);
static int wdg_window_redraw(struct wdg_object *wo);
static int wdg_window_get_focus(struct wdg_object *wo);
static int wdg_window_lost_focus(struct wdg_object *wo);
static int wdg_window_get_msg(struct wdg_object *wo, int key);

void wdg_window_set_title(struct wdg_object *wo, char *title, size_t align);
static void wdg_window_border(struct wdg_object *wo);

void wdg_window_print(wdg_t *wo, size_t x, size_t y, char *fmt, ...);

/*******************************************/

/* 
 * called to create a window
 */
void wdg_create_window(struct wdg_object *wo)
{
   /* set the callbacks */
   wo->destroy = wdg_window_destroy;
   wo->resize = wdg_window_resize;
   wo->redraw = wdg_window_redraw;
   wo->get_focus = wdg_window_get_focus;
   wo->lost_focus = wdg_window_lost_focus;
   wo->get_msg = wdg_window_get_msg;

   WDG_SAFE_CALLOC(wo->extend, 1, sizeof(struct wdg_window));
}

/* 
 * called to destroy a window
 */
static int wdg_window_destroy(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_window, ww);

   /* erase the window */
   werase(ww->sub);
   werase(ww->win);
   wrefresh(ww->sub);
   wrefresh(ww->win);
   
   /* dealloc the structures */
   delwin(ww->sub);
   delwin(ww->win);

   WDG_SAFE_FREE(ww->title);

   WDG_SAFE_FREE(wo->extend);

   return WDG_ESUCCESS;
}

/* 
 * called to resize a window
 */
static int wdg_window_resize(struct wdg_object *wo)
{
   wdg_window_redraw(wo);

   return WDG_ESUCCESS;
}

/* 
 * called to redraw a window
 */
static int wdg_window_redraw(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_window, ww);
   size_t c = wdg_get_ncols(wo);
   size_t l = wdg_get_nlines(wo);
   size_t x = wdg_get_begin_x(wo);
   size_t y = wdg_get_begin_y(wo);
 
   /* the window already exist */
   if (ww->win) {
      /* erase the border */
      werase(ww->win);
      touchwin(ww->win);
      wrefresh(ww->win);
      
      /* resize the window and draw the new border */
      mvwin(ww->win, y, x);
      wresize(ww->win, l, c);
      wdg_window_border(wo);
      
      /* resize the actual window and touch it */
      mvwin(ww->sub, y + 1, x + 1);
      wresize(ww->sub, l - 2, c - 2);
      /* set the window color */
      wbkgd(ww->sub, COLOR_PAIR(wo->window_color));
      touchwin(ww->sub);

   /* the first time we have to allocate the window */
   } else {

      /* create the outher window */
      if ((ww->win = newwin(l, c, y, x)) == NULL)
         return -WDG_EFATAL;

      /* draw the borders */
      wdg_window_border(wo);

      /* create the inner (actual) window */
      if ((ww->sub = newwin(l - 2, c - 2, y + 1, x + 1)) == NULL)
         return -WDG_EFATAL;
      
      /* set the window color */
      wbkgd(ww->sub, COLOR_PAIR(wo->window_color));
      redrawwin(ww->sub);

      /* initialize the pointer */
      wmove(ww->sub, 0, 0);

      scrollok(ww->sub, TRUE);

   }
   
   /* refresh the window */
   touchwin(ww->sub);
   wrefresh(ww->win);
   wrefresh(ww->sub);
   
   wo->flags |= WDG_OBJ_VISIBLE;

   return WDG_ESUCCESS;
}

/* 
 * called when the window gets the focus
 */
static int wdg_window_get_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags |= WDG_OBJ_FOCUSED;

   /* redraw the window */
   wdg_window_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called when the window looses the focus
 */
static int wdg_window_lost_focus(struct wdg_object *wo)
{
   /* set the flag */
   wo->flags &= ~WDG_OBJ_FOCUSED;
   
   /* redraw the window */
   wdg_window_redraw(wo);
   
   return WDG_ESUCCESS;
}

/* 
 * called by the messages dispatcher when the window is focused
 */
static int wdg_window_get_msg(struct wdg_object *wo, int key)
{
   WDG_WO_EXT(struct wdg_window, ww);
   wprintw(ww->sub, "WDG WIN: char %d\n", key);
   wrefresh(ww->sub);

   if (key == 'q')
      wdg_destroy_object(&wo);
   
   return WDG_ESUCCESS;
}

/*
 * set the window's title 
 */
void wdg_window_set_title(struct wdg_object *wo, char *title, size_t align)
{
   WDG_WO_EXT(struct wdg_window, ww);

   ww->align = align;
   ww->title = strdup(title);
}

static void wdg_window_border(struct wdg_object *wo)
{
   WDG_WO_EXT(struct wdg_window, ww);
   size_t c = wdg_get_ncols(wo);
      
   /* the object was focused */
   if (wo->flags & WDG_OBJ_FOCUSED) {
      wattron(ww->win, A_BOLD);
      wbkgdset(ww->win, COLOR_PAIR(wo->focus_color));
   } else
      wbkgdset(ww->win, COLOR_PAIR(wo->border_color));

   /* draw the borders */
   box(ww->win, 0, 0);
   
   /* set the border color */
   wbkgdset(ww->win, COLOR_PAIR(wo->title_color));
   
   /* there is a title: print it */
   if (ww->title) {
      switch (ww->align) {
         case WDG_ALIGN_LEFT:
            wmove(ww->win, 0, 3);
            break;
         case WDG_ALIGN_CENTER:
            wmove(ww->win, 0, (c - strlen(ww->title)) / 2);
            break;
         case WDG_ALIGN_RIGHT:
            wmove(ww->win, 0, c - strlen(ww->title) - 3);
            break;
      }
      wprintw(ww->win, ww->title);
   }
   
   /* restore the attribute */
   if (wo->flags & WDG_OBJ_FOCUSED)
      wattroff(ww->win, A_BOLD);
}

/*
 * print a string in the window
 */
void wdg_window_print(wdg_t *wo, size_t x, size_t y, char *fmt, ...)
{
   WDG_WO_EXT(struct wdg_window, ww);
   va_list ap;

   /* move the pointer */
   wmove(ww->sub, y, x);
   
   /* print the message */
   va_start(ap, fmt);
   vw_printw(ww->sub, fmt, ap);
   va_end(ap);

   wrefresh(ww->sub);
}

/* EOF */

// vim:ts=3:expandtab

