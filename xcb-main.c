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

  const xcb_setup_t *setup = xcb_get_setup(conn);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_screen_t *screen = iter.data;
  //GC is created on the root
  xcb_drawable_t win = screen->root;
  xcb_gcontext_t white_gc = xcb_generate_id(conn); 
  uint32_t gcmask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  uint32_t gcvalues[2] = { screen->white_pixel, 0};
  xcb_create_gc(conn, white_gc, win, gcmask, gcvalues);

  //Reassign win to be a new win
  win = xcb_generate_id(conn);
  uint32_t winmask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t winvalues[2] = { screen->black_pixel, XCB_EVENT_MASK_EXPOSURE};


  xcb_create_window(
      conn, XCB_COPY_FROM_PARENT,
      win, screen->root,
      sx, sy,                                     //startx, starty
      winw, winh,                                 //win width / height
      borderpx,                                   //border width
      XCB_WINDOW_CLASS_INPUT_OUTPUT,              //class
      screen->root_visual,                        // visual
      winmask, winvalues
      );
  xcb_map_window(conn, win);
  xcb_flush(conn);

  xcb_rectangle_t rect_1 = {
    .x = 0,
    .y = 0,
    .width = winw,
    .height = winh
  };
  xcb_rectangle_t rects[] = {rect_1};
  xcb_generic_event_t *ev;
  while((ev = xcb_wait_for_event(conn)) != NULL){
    switch(ev->response_type & ~0x80){
      case XCB_EXPOSE:
        printf("xcb_expose ev\n");
        xcb_poly_fill_rectangle(conn,
            win,
            white_gc,
            1,
            rects
            );
        xcb_flush(conn);
        break;
    }
    free(ev);
  }
  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}
