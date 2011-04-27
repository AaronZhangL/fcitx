/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "core/fcitx.h"

#include "MessageWindow.h"
#include "ui.h"
#include "core/xim.h"
#include "configfile.h"
#include "fcitx-config/cutils.h"

#include <ctype.h>

#include <iconv.h>
#include <X11/Xatom.h>

extern Display *dpy;
extern int      iScreen;
extern Atom killAtom;

MessageWindow messageWindow;

#define MESSAGE_WINDOW_MARGIN 20
#define MESSAGE_WINDOW_LINESPACE 2
static void            InitMessageWindowProperty (void);

Bool CreateMessageWindow (void)
{
    memset(&messageWindow, 0, sizeof(MessageWindow));
    messageWindow.color.r = messageWindow.color.g = messageWindow.color.b = 220.0 / 256;
    messageWindow.fontColor.r = messageWindow.fontColor.g = messageWindow.fontColor.b = 0;
    messageWindow.fontSize = 15;
    messageWindow.width = 1;
    messageWindow.height = 1;

    messageWindow.window =
	XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 0, 0, 1, 1, 0, WhitePixel (dpy, DefaultScreen (dpy)), WhitePixel (dpy, DefaultScreen (dpy)));

    messageWindow.surface = cairo_xlib_surface_create(dpy, messageWindow.window, DefaultVisual(dpy, iScreen), 1, 1); 
    if (messageWindow.window == None)
	return False;

    InitMessageWindowProperty ();
    XSelectInput (dpy, messageWindow.window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask );

    return True;
}

void InitMessageWindowProperty (void)
{
    XSetTransientForHint (dpy, messageWindow.window, DefaultRootWindow (dpy));

    SetWindowProperty(dpy, messageWindow.window, FCITX_WINDOW_DIALOG, "Fcitx - Message");

    XSetWMProtocols(dpy, messageWindow.window, &killAtom, 1);
}

void DisplayMessageWindow (void)
{
    int dwidth, dheight;
    GetScreenSize(&dwidth, &dheight);
    XMapRaised (dpy, messageWindow.window);
    XMoveWindow (dpy, messageWindow.window, (dwidth - messageWindow.width) / 2, (dheight - messageWindow.height) / 2);
}

void DrawMessageWindow (char *title, char **msg, int length)
{
    int i = 0;
    if (messageWindow.window == None)
        CreateMessageWindow();
    if (title)
    {
        if (messageWindow.title)
            free(messageWindow.title);
        messageWindow.title = strdup(title);
    }
    else
        if (!messageWindow.title)
            return;
    
    title = messageWindow.title;
    FcitxLog(INFO, "%s", title);

    XTextProperty   tp;
    Xutf8TextListToTextProperty(dpy, &title, 1, XUTF8StringStyle, &tp);
    XSetWMName(dpy, messageWindow.window, &tp);
    XFree(tp.value);

    if (msg)
    {
        if (messageWindow.msg)
        {
            for(i =0 ;i<messageWindow.length; i++)
                free(messageWindow.msg[i]);
            free(messageWindow.msg);
        }
        messageWindow.length = length;
        messageWindow.msg = malloc(sizeof(char*) * length);
        for (i = 0; i < messageWindow.length; i++)
            messageWindow.msg[i] = strdup(msg[i]);
    }
    else
    {
        if (!messageWindow.msg)
            return;
    }
    msg = messageWindow.msg;
    length = messageWindow.length;

    if (!msg || length == 0)
        return;

    messageWindow.height = MESSAGE_WINDOW_MARGIN * 2 + length *(messageWindow.fontSize + MESSAGE_WINDOW_LINESPACE);
    messageWindow.width = 0;

    for (i = 0; i< length ;i ++)
    {
        int width = StringWidth(msg[i], gs.font, messageWindow.fontSize);
        if (width > messageWindow.width)
            messageWindow.width = width;
    }

    messageWindow.width += MESSAGE_WINDOW_MARGIN * 2;
    XResizeWindow(dpy, messageWindow.window, messageWindow.width, messageWindow.height);
    cairo_xlib_surface_set_size(messageWindow.surface, messageWindow.width,messageWindow.height);

    cairo_t *c = cairo_create(messageWindow.surface);
    cairo_set_source_rgb(c, messageWindow.color.r, messageWindow.color.g, messageWindow.color.b);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);

    SetFontContext(c, gs.font, messageWindow.fontSize);

    cairo_paint(c);

    cairo_set_source_rgb(c, messageWindow.fontColor.r, messageWindow.fontColor.g, messageWindow.fontColor.b);

    int x, y;
    x = MESSAGE_WINDOW_MARGIN;
    y = MESSAGE_WINDOW_MARGIN;
    for (i = 0; i< length ;i ++)
    {
        OutputStringWithContext(c, msg[i], x, y);
        y += messageWindow.fontSize + MESSAGE_WINDOW_LINESPACE;
    }

    ResetFontContext();
    cairo_destroy(c);

    ActiveWindow(dpy, messageWindow.window);
}