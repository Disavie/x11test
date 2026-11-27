#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_errors.h>

#define bin( x ) for (int i = sizeof(x) * 8; i >= 0; i--) { \
        printf("%d", (x >> i) & 1); \
        if (i % 4 == 0 && i != 0) printf(" "); \
        } \
        printf("\n"); \

/*
 *  https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/
 */

xcb_gcontext_t init_xcb_font_gc(xcb_connection_t *conn, xcb_drawable_t *win, const char *fname);
void check_xcb_cookie(xcb_connection_t *conn, xcb_void_cookie_t c, const char * errstr);

static const int winw = 1920;
static const int winh= 1080;
static const int sx = 0;
static const int sy = 0;
static const int borderpx = 10;
//Available fonts, check xlsfonts
static const char * fname = "lucidasanstypewriter-8";
static xcb_void_cookie_t xcb_cookie;


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
  uint32_t winvalues[2] = { screen->black_pixel, 
      XCB_EVENT_MASK_EXPOSURE |
      XCB_EVENT_MASK_VISIBILITY_CHANGE |
      XCB_EVENT_MASK_BUTTON_PRESS |
      XCB_EVENT_MASK_BUTTON_RELEASE |
      XCB_EVENT_MASK_KEY_PRESS | 
      XCB_EVENT_MASK_KEY_RELEASE
  };

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
    .height = winh/2
  };
  xcb_rectangle_t rects[] = {rect_1};
  xcb_generic_event_t *ev;
  // Blocking way, alternatively may use xcb_poll_for_event(conn)
  // This one returns an event or NULL if no event. 
  while((ev = xcb_wait_for_event(conn)) != NULL){
//    printf("ev->response_type :");
//    bin(ev->response_type);
    /*
     * strips leading 1 in 1000.1100
     *leading 1 means another client sent this msg?
     *a 0 means it came from the x server
    */
    switch(ev->response_type & ~0x80){
      case XCB_EXPOSE:
        xcb_poly_fill_rectangle(conn,
            win,
            white_gc,
            1,
            rects
            );
        const char * text = "Hello world!";
        xcb_gcontext_t fontgc = init_xcb_font_gc(conn, &win, fname);
        xcb_cookie = xcb_image_text_8_checked(conn, strlen(text), win, fontgc, 0, 12, text);
        check_xcb_cookie(conn, xcb_cookie, "drawing text");
        xcb_cookie = xcb_free_gc_checked(conn, fontgc);
        check_xcb_cookie(conn, xcb_cookie, "freeing font gc");
        break;
      case XCB_KEY_PRESS:
      case XCB_KEY_RELEASE: {
                              xcb_key_press_event_t *k = (xcb_key_press_event_t *)ev;
                              printf("%u\n", k->detail);
                              break;
                            }
      case XCB_BUTTON_PRESS: {
                               xcb_button_press_event_t *b = (xcb_button_press_event_t *)ev;
                               printf("button pushed @ {%d, %d}\n", b->event_x, b->event_y);
                             }
      case XCB_BUTTON_RELEASE: {
                               xcb_button_press_event_t *b = (xcb_button_press_event_t *)ev;
                               printf("button released @ {%d, %d}\n", b->event_x, b->event_y);
                             }

    }
    xcb_flush(conn);
    free(ev);
  }
  xcb_cookie = xcb_free_gc_checked(conn, white_gc);
  check_xcb_cookie(conn, xcb_cookie, "freeing white_gc");
  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}

xcb_gcontext_t
init_xcb_font_gc(xcb_connection_t *conn, xcb_drawable_t *win, const char *font_name){
  const xcb_setup_t *setup = xcb_get_setup(conn);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_screen_t *screen = iter.data;
  //Open a a font
  // Adding XBC_GC_FONT to the gc's valuemask allows it to draw fonts
  xcb_font_t f = xcb_generate_id(conn);
  xcb_cookie = xcb_open_font_checked(conn, f, strlen(font_name), font_name);
  check_xcb_cookie(conn, xcb_cookie, "opening font");

  xcb_gcontext_t fontgc = xcb_generate_id(conn); 
  uint32_t gcmask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
  uint32_t gcvalues[3] = { screen->black_pixel, screen->white_pixel , f};
  xcb_cookie = xcb_create_gc_checked(conn, fontgc, *win, gcmask, gcvalues);
  check_xcb_cookie(conn, xcb_cookie, "create gc");
  xcb_close_font(conn, f);
  return fontgc;
}

void
check_xcb_cookie(xcb_connection_t *conn, xcb_void_cookie_t c, const char *errstr){
  xcb_generic_error_t *err = xcb_request_check(conn, c);
    if(err){
      fprintf(stderr, "error: %u @ %s\n", err->error_code, errstr);
      exit(EXIT_FAILURE);
    }
}
