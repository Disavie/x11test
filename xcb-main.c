#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <unistd.h>


/*
 *  https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/
 */

static const int winw = 500;
static const int winh= 500;
static const int sx = 0;
static const int sy = 0;
static const int borderpx = 10;

int main(int argc, char **argv){
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if(xcb_connection_has_error(conn)){
    xcb_disconnect(conn);
    exit(EXIT_FAILURE);
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
      sx, sy,                                     //startx, starty
      winw, winh,                                 //win width / height
      borderpx,                                   //border width
      XCB_WINDOW_CLASS_INPUT_OUTPUT,              //class
      screen->root_visual,                        // visual
      XCB_CW_BACK_PIXEL, (uint32_t[]){screen->black_pixel}      // value mask and value list
      );
  err = xcb_request_check(conn, cookie);
  if (err != NULL){
    perror("create_window");
    exit(EXIT_FAILURE);
  }

  /*
   * We give now an example on how to allocate a graphic 
   * context that specifies that each drawing function 
   * that uses it will draw in foreground with a black color.
   */
  xcb_drawable_t drawable_win = win;
  xcb_gcontext_t white_gc = xcb_generate_id(conn); 
  uint32_t mask = XCB_GC_FOREGROUND;
  uint32_t value[] = { screen->white_pixel};
  cookie = xcb_create_gc( conn, white_gc, drawable_win, mask, value);
  err = xcb_request_check(conn, cookie);
  if (err != NULL){
    perror("create_gc");
    exit(EXIT_FAILURE);
  }
  /*
   * Properties of the graphics context may be changed by
   * calling the following function:
   * xcb_change_gc
   * @p valuemask should be xcb_gc_t enum
   */
  xcb_rectangle_t rect_1 = {
    .x = 0,
    .y = 0,
    .width = winw,
    .height = winh
  };
  xcb_rectangle_t rects[] = {rect_1};
  xcb_poly_fill_rectangle(conn,
      drawable_win,
      white_gc,
      1,
      rects
      );

  cookie = xcb_map_window(conn, win);
  err = xcb_request_check(conn, cookie);
  if (err != NULL){
    perror("map_window");
    exit(EXIT_FAILURE);
  }

   
  xcb_flush(conn);
  //for(;;);
  pause();

  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}
