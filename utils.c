/*
 * utils.c
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xdelimitarea.h"

enum motion_dir_code {
  UP_LEFT = 0,
  UP_RIGHT = 1,
  DOWN_LEFT = 2,
  DOWN_RIGHT = 3
};

/*
 * For the given font, determine an acceptable rectangular
 * region that encompasses the kind of geometry-related
 * text that we intend to display.  Return the width, height
 * in the W, H arguments, which are passed by reference.
 */
void get_text_extents (XFontStruct *xfs, int *w, int *h)
{
  /* Measure the biggest possible geometry string. */
  static const char *text = "1000x1000+50+50";
  int dir, ascent, descent;
  XCharStruct overall;

  XTextExtents (xfs, text, strlen (text), &dir, &ascent, &descent,
                &overall);
  *h = overall.ascent + overall.descent + 2*YPAD;
  *w = overall.rbearing - overall.lbearing;

  /* Pad the width to make readout less tight. */
  *w += 1.5*XPAD;
}

/*
 * Find a suitable location for the text window.
 * The bounding box given by (curX,curY) to (otherX,otherY)
 * represents the currently delimited region.  (curX,curY) is
 * the last pointer location.  (txt_x,txt_y) is the last position
 * where the text window was positioned.  The text window has
 * dimensions txt_w by txt_h.  Return the location found
 * in the X, Y arguments, which are passed by reference.
 */
void get_text_location (Display *dpy, int screen,
                        int curX, int curY, int otherX, int otherY,
                        int txt_x, int txt_y, int txt_w, int txt_h,
                        int *x, int *y)
{
  Screen *screen_obj = ScreenOfDisplay (dpy, screen);
  int direction;

  /* This case is special. It happens when the user first
   * initiates a delimiting operation with the pointer. In
   * that case, we always position the text window to the
   * lower right of the first point (curX,curY).  This is
   * the DOWN_RIGHT case, handled in the switch below. */

  if (txt_x < 0 && txt_y < 0)
    direction = DOWN_RIGHT;

  direction = 0;
  direction |= (curX >= otherX) ? 0x1 : 0x0;
  direction |= (curY >= otherY) ? 0x2 : 0x0;

  switch (direction)
    {
    case UP_LEFT:
      *x = curX - txt_w;
      *y = curY - txt_h;

      /* Check if we overreach left of screen. */
      if (*x < 0)
        *x = curX;

      /* Check if we overreach top of screen. */
      if (*y < 0)
        *y = curY;
      break;

    case UP_RIGHT:
      *x = curX;
      *y = curY - txt_h;

      /* Check if we overreach right of screen. */
      if ((*x + txt_w) > WidthOfScreen (screen_obj))
        *x -= (txt_w + TXT_BORDER);

      /* Check if we overreach top of screen. */
      if (*y < 0)
        *y = curY;
      break;

    case DOWN_LEFT:
      *x = curX - txt_w;
      *y = curY;

      /* Check if we overreach left of screen. */
      if (*x < 0)
        *x = curX;

      /* Check if we overreach bottom of screen. */
      if ((*y + txt_h) > HeightOfScreen (screen_obj))
        *y -= (txt_h + TXT_BORDER);
      break;

    case DOWN_RIGHT:
      *x = curX;
      *y = curY;

      /* Check if we overreach right of screen. */
      if ((*x + txt_w) > WidthOfScreen (screen_obj))
        *x -= (txt_w + TXT_BORDER);

      /* Check if we overreach bottom of screen. */
      if ((*y + txt_h) > HeightOfScreen (screen_obj))
        *y -= (txt_h + TXT_BORDER);
      break;

    default:
      assert (0);
      break;
    }
}

/*
 * Return a geometry string for the given position and
 * dimensions.  Note that we do not readout coordinates
 * with respect to the lower-right corner of the screen,
 * as does, for example, xwininfo; ie, all coordinates are
 * given with respect to the upper-left corner of the screen.
 */
char *get_readout (int x, int y, unsigned int w, unsigned int h)
{
  static char readout[64];
  sprintf (readout, "%dX%d+%d+%d", w, h, x, y);
  return readout;
}
