/*
 * xdelimitarea.c
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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if STDC_HEADERS
#include <stdlib.h>
#endif /* STDC_HEADERS */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "xdelimitarea.h"

/*
 * These are the default settings for the readout window.
 * They can be overridden at run-time either via command-line
 * options or X resources.
 */
#define DEFAULT_FG "black"
#define DEFAULT_BG "tan"
#define DEFAULT_FONT "fixed"

/*
 * Following are used for dealing with X resources.
 */
static char *progname = NULL;
static XrmDatabase xrm_db = NULL;

/*
 * Forward declarations.
 */
static void parse_command_line
  (Display *dpy, int *argc, char *argv[]);

static char *get_resource
  (char *res_name, char *res_class, char *defvalue);

static void set_program_name
  (char *candidate);

/*
 * These options control the appearance of the text-window.
 */
static XrmOptionDescRec options[] = {
  {"-font", ".font", XrmoptionSepArg, NULL},
  {"-fg", ".foreground", XrmoptionSepArg, NULL},
  {"-bg", ".background", XrmoptionSepArg, NULL}
};

static void usage (int extval)
{
#define PUSAGE(x) fprintf (stderr, x)

  PUSAGE ("Usage: xdelimitarea [OPTION]...\n");
  PUSAGE ("\n");
  PUSAGE ("Delimit arbitrary rectangular area on screen.\n");
  PUSAGE ("\n");
  PUSAGE ("  -help         display this help and exit\n");
  PUSAGE ("  -font         specify font of readout; default is fixed\n");
  PUSAGE ("  -fg           specify foreground of readout; default is black\n");
  PUSAGE ("  -bg           specify background of readout; default is tan\n");
  PUSAGE ("\n");
  exit (extval);

#undef PUSAGE
}

int main (int argc, char *argv[])
{
  Display *dpy;
  Window full_screen_win, txt_win;
  Pixmap txt_backing_store;
  Colormap cmap;
  XEvent event;
  GC xor_gc, txt_gc;
  XFontStruct *font_obj;
  char *readout, *fg_spec, *bg_spec, *font_spec;
  unsigned long txt_win_bg_pixel, txt_win_fg_pixel;
  int screen;
  int xorig, yorig, xlast, ylast, txt_x, txt_y;
  int txt_win_width, txt_win_height;

  /* Remember who we really are. */
  set_program_name (argv[0]);

  dpy = XOpenDisplay (NULL);
  if (dpy == NULL)
    {
      perror ("X Display failed to open");
      exit (1);
    }

  /* Deal with options. */
  parse_command_line (dpy, &argc, argv);
  if (argc > 1)
    {
      if (! strcmp (argv[1], "-help"))
        usage (0);
      else
        usage (1);
    }

  screen = DefaultScreen (dpy);
  cmap = DefaultColormap (dpy, screen);
  full_screen_win = create_full_screen_window (dpy, screen, cmap);

  /* Set graphics context for XOR operation. */
  xor_gc = create_xor_gc (dpy, full_screen_win);

  /* Figure out colors and font for text-window. */
  fg_spec = get_resource ("foreground", "Foreground", DEFAULT_FG);
  bg_spec = get_resource ("background", "Background", DEFAULT_BG);
  font_spec = get_resource ("font", "Font", DEFAULT_FONT);

  /* Get background, foreground pixels for text window. */
  txt_win_bg_pixel = get_pixel (dpy, screen, cmap, bg_spec);
  txt_win_fg_pixel = get_pixel (dpy, screen, cmap, fg_spec);

  /* Load font, and create window for text of suitable size. */
  font_obj = XLoadQueryFont (dpy, font_spec);
  if (font_obj == NULL)
    {
      perror ("Could not load font");
      exit (1);
    }
  get_text_extents (font_obj, &txt_win_width, &txt_win_height);
  txt_win = create_text_window (dpy, full_screen_win, screen,
                                txt_win_width, txt_win_height,
                                txt_win_bg_pixel);

  /* Set graphics context for drawing text. */
  txt_gc = create_text_gc (dpy, txt_win, txt_win_bg_pixel,
                           txt_win_fg_pixel, font_obj->fid);

  /* The backing-store is a Pixmap that stores any area of the
   * full-screen window that is obscured when we render the
   * text-window. */
  txt_backing_store = create_backing_store (dpy, full_screen_win, txt_win);

  /* Map the full-screen window.  Note that visually, this should
   * have no effect at all.  The important result for us is that
   * we now can delimit any area on the screen via interaction
   * with the mouse-pointer.  We DO NOT map the text window, yet.
   * That happens only after the user has started to delimit
   * some region of the screen. */
  XMapWindow (dpy, full_screen_win);

  /* Grab keyboard so we are certain to get keypresses. */
  XGrabKeyboard (dpy, full_screen_win, True, GrabModeAsync, GrabModeAsync,
                 CurrentTime);

  /* Initialize variables. */
  xorig = -1;
  yorig = -1;
  txt_x = -1;
  txt_y = -1;

  while (1)
    {
#define ABS(x) ((x) >= 0 ? (x) : -(x))
      XNextEvent (dpy, &event);
      switch (event.type)
        {
        case Expose:
          break;
        case ButtonPress:
          /* If Button1, then record (x,y) coordinate. */
          if (event.xbutton.button == Button1)
            {
              xorig = event.xbutton.x;
              yorig = event.xbutton.y;
              XDrawRectangle (dpy, full_screen_win, xor_gc,
                              xorig, yorig, 0, 0);

              /* Get readout string for delimited region. */
              readout = get_readout (xorig, yorig, 0, 0);

              xlast = xorig;
              ylast = yorig;

              /* Copy obscured region into our local backing store.
               * Then move the text-window to suitable position. */
              txt_x = xorig;
              txt_y = yorig;
              XCopyArea (dpy, full_screen_win, txt_backing_store,
                         txt_gc, txt_x, txt_y, txt_win_width + 2*TXT_BORDER,
                         txt_win_height + 2*TXT_BORDER, 0, 0);
              XMoveWindow (dpy, txt_win, txt_x, txt_y);
              XMapWindow (dpy, txt_win);

              /* Draw the readout-string, but only after mapping
               * the text window. */
              XDrawString (dpy, txt_win, txt_gc, XPAD, txt_win_height - YPAD,
                           readout, strlen (readout));
            }
          else
            {
              /* Reset, though probably unnecessary here. */
              xorig = yorig = -1;
              txt_x = txt_y = -1;
            }
          break;
        case ButtonRelease:
          do {
            int x0 = (xlast > xorig) ? xorig : xlast;
            int y0 = (ylast > yorig) ? yorig : ylast;

            /* Unmap text window. */
            XUnmapWindow (dpy, txt_win);

            /* Restore area of screen obscured by the text-window. */
            XCopyArea (dpy, txt_backing_store, full_screen_win,
                       txt_gc, 0, 0, txt_win_width + 2*TXT_BORDER,
                       txt_win_height + 2*TXT_BORDER, txt_x, txt_y);

            /* Draw final rectangle to erase last XOR'd lines. */
            XDrawRectangle (dpy, full_screen_win, xor_gc, x0, y0,
                            ABS (xlast - xorig), ABS (ylast - yorig));

            /* Reset to indicate drawing is over. */
            xorig = yorig = -1;
            txt_x = txt_y = -1;
          } while (0);
          break;
        case MotionNotify:
          if (xorig >= 0 && yorig >= 0)
            {
              int x, y, w, h, x0, y0;
              x = event.xmotion.x;
              y = event.xmotion.y;

              /* Unmap text window. */
              XUnmapWindow (dpy, txt_win);

              /* Restore area of screen obscured by the text-window. */
              XCopyArea (dpy, txt_backing_store, full_screen_win,
                         txt_gc, 0, 0, txt_win_width + 2*TXT_BORDER,
                         txt_win_height + 2*TXT_BORDER, txt_x, txt_y);

              /* Draw over last rectangle drawn in order to
               * revert back to original pixels. */
              x0 = (xlast > xorig) ? xorig : xlast;
              y0 = (ylast > yorig) ? yorig : ylast;
              XDrawRectangle (dpy, full_screen_win, xor_gc, x0, y0,
                              ABS (xlast - xorig), ABS (ylast - yorig));

              /* Draw new rectangle to reflect movement of cursor. */
              x0 = (x > xorig) ? xorig : x;
              y0 = (y > yorig) ? yorig : y;
              w = ABS (x - xorig);
              h = ABS (y - yorig);
              XDrawRectangle (dpy, full_screen_win, xor_gc, x0, y0, w, h);

              /* Get readout string for delimited region. */
              readout = get_readout (x0, y0, w, h);

              get_text_location (dpy, screen, x, y, xorig, yorig,
                                 txt_x, txt_y, txt_win_width, txt_win_height,
                                 &txt_x, &txt_y);

              /* Save area of screen obscured by text-window. */
              XCopyArea (dpy, full_screen_win, txt_backing_store,
                         txt_gc, txt_x, txt_y, txt_win_width + 2*TXT_BORDER,
                         txt_win_height + 2*TXT_BORDER, 0, 0);

              XMoveWindow (dpy, txt_win, txt_x, txt_y);
              XMapWindow (dpy, txt_win);

              /* Draw the readout-string, but only after mapping
               * the text window. */
              XDrawString (dpy, txt_win, txt_gc, XPAD, txt_win_height - YPAD,
                           readout, strlen (readout));

              /* Record last coordinate so we can 'undraw'
               * the rectangle we just drew. */
              xlast = x;
              ylast = y;
            }
          break;
        case KeyPress:
          {
            /* If either 'q', 'Q', or 'Ctrl-c' is pressed, then exit.
             * We explicitly check for Ctrl-c because we have grabbed
             * the keyboard, and that key-sequence will not auto-
             * matically kill the current process. */
            Bool exit_now = False;
            KeySym ks = XKeycodeToKeysym (dpy, event.xkey.keycode, 0);
            if (ks == XK_Q || ks == XK_q)
              {
                exit_now = True;
              }
            else if (ks == XK_c)
              {
                Window root, child;
                int root_x, root_y, win_x, win_y;
                unsigned int mask;
                Bool rc = XQueryPointer (dpy, full_screen_win, &root,
                                         &child, &root_x, &root_y,
                                         &win_x, &win_y, &mask);
                assert (rc == True);
                if (mask & ControlMask)
                  exit_now = True;
              }
            if (exit_now)
              {
                XUngrabKeyboard (dpy, CurrentTime);
                XrmDestroyDatabase (xrm_db);
                return 0;
              }
            break;
          }
        default:
          /* This case should not happen. */
          break;
        }
#undef ABS
    }
  return 0;
}

void parse_command_line (Display *dpy, int *argc, char *argv[])
{
  XrmDatabase other_db;
  char *rsrc_mgr, *homedir, *xenv, fname[128];

  /* Fire up the X resource manager. */
  XrmInitialize ();

  /* Load any relevant command-line options into our db. */
  XrmParseCommand (&xrm_db, options,
                   sizeof (options) / sizeof (XrmOptionDescRec),
                   "xdelimitarea", argc, argv);

  /* Merge in any server-specified resources. */
  rsrc_mgr = XResourceManagerString (dpy);
  if (rsrc_mgr != NULL)
    {
      other_db = XrmGetStringDatabase(rsrc_mgr);
      XrmMergeDatabases (other_db, &xrm_db);
    }

  /* Merge in user defaults. */
  homedir = getenv ("HOME");
  sprintf (fname, "%s/.Xdefaults", homedir);
  if (access (fname, R_OK) == 0)
    {
      other_db = XrmGetFileDatabase (fname);
      XrmMergeDatabases (other_db, &xrm_db);
    }

  /* Merge in XENVIRONMENT. We do not care if it is the same
   * .Xdefaults file we checked above. */
  xenv = getenv("XENVIRONMENT");
  if (xenv != NULL && access (xenv, R_OK) == 0)
    {
      other_db = XrmGetFileDatabase (xenv);
      XrmMergeDatabases (other_db, &xrm_db);
    }
}

void set_program_name (char *candidate)
{
  progname = strrchr (candidate, '/');
  if (progname)
    ++progname;
  else
    progname = candidate;
}

char *get_resource (char *res_name, char *res_class, char *defvalue)
{
#define PROGCLASS "XDelimitArea"

  char *type, full_name[1024], full_class[1024];
  XrmValue value;
  Bool rc;

  /* Only call this if we have a valid db. */
  assert (xrm_db != NULL);

  strcpy (full_name, progname);
  strcat (full_name, ".");
  strcat (full_name, res_name);
  strcpy (full_class, PROGCLASS);
  strcat (full_class, ".");
  strcat (full_class, res_class);

  rc = XrmGetResource (xrm_db, full_name, full_class, &type, &value);
  if (rc == True)
    {
      char *str = (char *) malloc (value.size + 1);
      strncpy (str, (char *) value.addr, value.size);
      str [value.size] = 0;
      return str;
    }

  return defvalue;

#undef PROGCLASS
}
