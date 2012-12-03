/*
 * xdelimitarea.h
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

#ifndef _XDELIMITAREA_H
#define _XDELIMITAREA_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define XPAD 10
#define YPAD 1
#define TXT_BORDER 1

/*
 * Windows-related routines.
 */
extern int screen_number
  (Screen *screen);

extern int visual_depth
  (Screen *screen, Visual *visual);

extern Window create_full_screen_window
  (Display *dpy, int screen, Colormap cmap);

extern Window create_text_window
  (Display *dpy, Window parent, int screen, int width, int height,
   unsigned long bg_pixel);

extern Pixmap create_backing_store
  (Display *dpy, Window under, Window over);

extern GC create_xor_gc
  (Display *dpy, Window win);

extern GC create_text_gc
  (Display *dpy, Window win, unsigned long bg_pixel, unsigned long fg_pixel,
   Font fid);

extern unsigned long get_pixel
  (Display *dpy, int screen, Colormap cmap, const char *colorstr);

/*
 * Text-related routines.
 */
extern void get_text_extents
  (XFontStruct *xfs, int *w, int *h);

extern void get_text_location
  (Display *dpy, int screen, int curX, int curY, int otherX, int otherY,
   int txt_x, int txt_y, int txt_w, int txt_h, int *x, int *y);

extern char *get_readout
  (int x, int y, unsigned int w, unsigned int h);

#endif /* _XDELIMITAREA_H */
