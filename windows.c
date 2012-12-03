/*
 * windows.c
 * Charlie Read <clread@mindspring.com>
 *
 * This software is free; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 * 
 * See the COPYING file located in the top-level-directory of
 * the archive of this library for complete text of license.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "xdelimitarea.h"

int screen_number (Screen *screen)
{
  Display *dpy = DisplayOfScreen (screen);
  int i;
  for (i = 0; i < ScreenCount (dpy); i++)
    if (ScreenOfDisplay (dpy, i) == screen)
      return i;
  abort ();
  return 0;
}

int visual_depth (Screen *screen, Visual *visual)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count, depth;
  vi_in.screen = screen_number (screen);
  vi_in.visualid = XVisualIDFromVisual (visual);
  vi_out = XGetVisualInfo (dpy, VisualScreenMask|VisualIDMask,
                           &vi_in, &out_count);
  if (! vi_out) abort ();
  depth = vi_out[0].depth;
  XFree ((char *) vi_out);
  return depth;
}

Window create_full_screen_window (Display *dpy, int screen, Colormap cmap)
{
  XSetWindowAttributes attrs;
  Window win;
  Visual *visual;
  Screen *screen_obj;
  unsigned long attrmask;
  int depth;

  screen_obj = ScreenOfDisplay (dpy, screen);
  visual = DefaultVisualOfScreen (screen_obj);
  depth = visual_depth (screen_obj, visual);

  attrmask = (CWOverrideRedirect | CWEventMask | CWBackingStore | CWColormap
              | CWCursor);
  attrs.override_redirect = True;
  attrs.event_mask = (ExposureMask | KeyPressMask | ButtonPressMask
                      | Button1MotionMask | ButtonReleaseMask);
  attrs.backing_store = NotUseful;
  attrs.colormap = cmap;
  attrs.cursor = XCreateFontCursor (dpy, XC_cross);

  win = XCreateWindow (dpy, RootWindowOfScreen (screen_obj),
                       0, 0, WidthOfScreen (screen_obj),
                       HeightOfScreen (screen_obj), 0,
                       depth, InputOutput, visual,
                       attrmask, &attrs);
  return win;
}

Window create_text_window (Display *dpy, Window parent, int screen,
                           int width, int height,
                           unsigned long bg_pixel)
{
  XSetWindowAttributes attrs;
  XWindowAttributes parent_attrs;
  Window win;
  Visual *visual;
  Screen *screen_obj;
  unsigned long attrmask;
  int depth;

  screen_obj = ScreenOfDisplay (dpy, screen);
  visual = DefaultVisualOfScreen (screen_obj);

  /* Get depth from parent. */
  XGetWindowAttributes (dpy, parent, &parent_attrs);
  depth = parent_attrs.depth;

  attrmask = (CWOverrideRedirect | CWBackingStore | CWBackPixel);
  attrs.override_redirect = True;
  attrs.backing_store = NotUseful;
  attrs.background_pixel = bg_pixel;

  win = XCreateWindow (dpy, parent, 0, 0, width, height, TXT_BORDER,
                       depth, InputOutput, visual,
                       attrmask, &attrs);
  return win;
}

Pixmap create_backing_store (Display *dpy, Window under, Window over)
{
  Pixmap pmap;
  XWindowAttributes over_attrs, under_attrs;

  XGetWindowAttributes (dpy, under, &under_attrs);
  XGetWindowAttributes (dpy, over, &over_attrs);
  pmap = XCreatePixmap (dpy, under, over_attrs.width + 2*TXT_BORDER,
                        over_attrs.height + 2*TXT_BORDER, under_attrs.depth);
  return pmap;
}

GC create_xor_gc (Display *dpy, Window win)
{
  XGCValues xgcv;
  GC gc;

  xgcv.function = GXxor;
  xgcv.foreground = 0xffffff;
  gc = XCreateGC (dpy, win, GCFunction | GCForeground, &xgcv);
  return gc;
}

GC create_text_gc (Display *dpy, Window win,
                   unsigned long bg_pixel, unsigned long fg_pixel,
                   Font fid)
{
  XGCValues xgcv;
  GC gc;

  xgcv.background = bg_pixel;
  xgcv.foreground = fg_pixel;
  xgcv.font = fid;
  gc = XCreateGC (dpy, win, GCForeground | GCBackground | GCFont, &xgcv);
  return gc;
}

unsigned long get_pixel (Display *dpy, int screen,
                         Colormap cmap, const char *colorstr)
{
  XColor exact_fg, screen_fg;
  unsigned long pixel;
  int rc;

  rc = XAllocNamedColor (dpy, cmap, colorstr, &screen_fg, &exact_fg);
  if (rc > 0)
    {
      pixel = screen_fg.pixel;
    }
  else
    {
      /* If we can't allocate the requested color-name,
       * then just choose the white pixel. */
      pixel = WhitePixel (dpy, screen);
    }
  return pixel;
}
