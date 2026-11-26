#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>


/*
 *  https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/
 */

static const int winw = 100;
static const int winh= 100;
static const int sx = 100;
static const int sy = 100;
static const int borderpx = 10;

int main(int argc, char **argv){
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if(xcb_connection_has_error(conn)){
    xcb_disconnect(conn);
  }
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err;

  const xcb_setup_t *setup = xcb_get_setup(conn);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_screen_t *screen = iter.data;

  xcb_window_t win = xcb_generate_id(conn);

  cookie = xcb_create_window(
      conn, XCB_COPY_FROM_PARENT,
      win, screen->root,
      sx, sy,                       //startx, starty
      winw, winh,                   //win width / height
      borderpx,                     //border width
      XCB_WINDOW_CLASS_INPUT_OUTPUT,//class
      screen->root_visual,          // visual
      0, NULL                       // value mask and value list
      );
  err = xcb_request_check(conn, cookie);
  if (err != NULL){
    perror("create_window");
  }
  cookie = xcb_map_window(conn, win);

  err = xcb_request_check(conn, cookie);
  if (err != NULL){
    perror("map_window");
  }

  xcb_flush(conn);
  for(;;);

  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}
