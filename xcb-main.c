#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xproto.h>
#include <xcb/xcb_errors.h>
//#include <X11/keysym.h>
#include <xcb/xcb_keysyms.h>


#define bin( x ) do{ \
  printf("%d", (x >> i) & 1); \
  for (int i = sizeof(x) * 8; i >= 0; i--) { \
    if (i % 4 == 0 && i != 0) printf(" "); \
  } \
  printf("\n"); \
  while(0); \

#define ARR_LEN(x) sizeof(x) / sizeof(x[0]);

#define MARGIN 25

/*
 *  https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/
 */
static inline int min(int a, int b){ return a < b ? a : b; }
static int char_width;

struct m_point_arr {
  int cap;
  int idx;
  struct xcb_point_t *elems;
};

typedef struct{
  int x;
  int y;
}Cursor_t;


void free_point_arr(struct m_point_arr *points);
void push_point(struct m_point_arr *points, struct xcb_point_t point);
void update_rects_geometry(xcb_connection_t *conn, xcb_drawable_t *win, xcb_rectangle_t *rect);
xcb_gcontext_t init_xcb_font_gc(xcb_connection_t *conn, xcb_drawable_t *win, const char *fname, uint32_t col);
void check_xcb_cookie(xcb_connection_t *conn, xcb_void_cookie_t c, const char * errstr);
void sprintscr(xcb_connection_t *conn, xcb_drawable_t *win, xcb_gcontext_t *fontgc, int startx, int starty, const char *text);
void cprintscr(xcb_connection_t *conn, xcb_drawable_t *win, xcb_gcontext_t *fontgc, const char c);

static int winw = 1920;
static int winh= 1080;
static const int sx = 0;
static const int sy = 0;
static const int borderpx = 10;
//Available fonts, check xlsfonts
static const char *fname = "lucidasanstypewriter-8";
static xcb_void_cookie_t xcb_cookie;
static const char *winname = "poop window";

static Cursor_t cursor = {
  .x = MARGIN,
  .y = MARGIN
};

int main(int argc, char **argv){
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if(xcb_connection_has_error(conn)){
    xcb_disconnect(conn);
    exit(EXIT_FAILURE);
  }

  xcb_key_symbols_t * syms = xcb_key_symbols_alloc(conn);
  const xcb_setup_t *setup = xcb_get_setup(conn);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_screen_t *screen = iter.data;

  //GC is created on the root
  xcb_drawable_t win = screen->root;

  xcb_gcontext_t white_gc = xcb_generate_id(conn); 
  uint32_t gcmask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  uint32_t gcvalues[2] = { screen->white_pixel, 0};
  xcb_create_gc(conn, white_gc, win, gcmask, gcvalues);
  
  xcb_gcontext_t black_gc = xcb_generate_id(conn); 
  uint32_t gcvalues_black[2] = { screen->black_pixel, 0};
  xcb_create_gc(conn, black_gc, win, gcmask, gcvalues_black);

  //Reassign win to be a new win
  win = xcb_generate_id(conn);
  uint32_t winmask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t winvalues[2] = { screen->black_pixel, 
      XCB_EVENT_MASK_EXPOSURE |
      XCB_EVENT_MASK_VISIBILITY_CHANGE |
      //XCB_EVENT_MASK_RESIZE_REDIRECT |
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
  xcb_change_property_checked(conn,
      XCB_PROP_MODE_REPLACE,
      win, 
      XCB_ATOM_WM_NAME,
      XCB_ATOM_STRING , 
      8,
      strlen(winname),
      winname
      );
  xcb_generic_error_t *err;
  xcb_get_geometry_cookie_t geo_cookie = xcb_get_geometry(conn, win);
  xcb_get_geometry_reply_t *g_reply = xcb_get_geometry_reply(conn, geo_cookie, &err);
  if(err != NULL){
    fprintf(stderr, "get_geometry: %d", err->error_code);
    free(err);
    exit(EXIT_FAILURE);
  }
  winw = g_reply->width;
  winh = g_reply->height;

  free(g_reply);
  xcb_map_window(conn, win);
  xcb_flush(conn);

  xcb_rectangle_t rect_1 = {
    .x = MARGIN,
    .y = MARGIN,
    .width = winw - rect_1.x,
    .height = winh - rect_1.y
  };
  xcb_rectangle_t rects[] = {rect_1};
  xcb_generic_event_t *ev;

  // Blocking way, alternatively may use xcb_poll_for_event(conn)
  // This one returns an event or NULL if no event. 
  int is_hidden = 0;
  xcb_gcontext_t fontgc = init_xcb_font_gc(conn, &win, fname, 0xFFFFFF);
  xcb_gcontext_t fontgc_black = init_xcb_font_gc(conn, &win, fname, 0x0);

  struct m_point_arr points = {
    .cap = 1,
    .idx = 0,
    .elems =  (xcb_point_t *)malloc(sizeof(xcb_point_t) * points.cap)
  };
  memset(points.elems, 0, sizeof(xcb_point_t) * points.cap);

  while((ev = xcb_wait_for_event(conn)) != NULL){
    /*
     * strips leading 1 in 1000.1100
     *leading 1 means another client sent this msg?
     *a 0 means it came from the x server
    */

    xcb_cookie = xcb_poly_line(conn, XCB_COORD_MODE_ORIGIN, win, black_gc, points.idx, points.elems);
    check_xcb_cookie(conn, xcb_cookie, "poly_line: ");
    switch(ev->response_type & ~0x80){
      case XCB_EXPOSE:
        for(int i = 0; i < points.idx; i++){
             xcb_point_t p = points.elems[i];
            fprintf(stdout, "p%d: %d, %d\n", i, p.x, p.y);
        }
        //Redraw everything, clear first
        xcb_clear_area(conn, 0, win, 0, 0, winw, winh);
        update_rects_geometry(conn, &win, &rects[0]);
        xcb_poly_fill_rectangle(conn,
            win,
            white_gc,
            1,
            rects
            );
        const char * text = "Hello world! <ESC> to exit";
        xcb_cookie = xcb_image_text_8_checked(conn, strlen(text), win, fontgc, 0, 12, text);
        check_xcb_cookie(conn, xcb_cookie, "drawing text");
        break;
      case XCB_KEY_PRESS:{
                              xcb_key_press_event_t *k = (xcb_key_press_event_t *)ev;
                              xcb_keysym_t sym = xcb_key_press_lookup_keysym(syms, k, 0);
                              cprintscr(conn, &win, &fontgc_black, (char)sym); 
                              switch (k->detail){ 
                                case 9: // < esc
                                  free(ev);
                                  goto post_event_loop;
                              }
                              break;
                         }
      case XCB_KEY_RELEASE: {
                              xcb_key_press_event_t *k = (xcb_key_press_event_t *)ev;
                              break;
                            }
      case XCB_BUTTON_PRESS: {
                               xcb_button_press_event_t *b = (xcb_button_press_event_t *)ev;
                               xcb_point_t point = {b->event_x, b->event_y};
                               push_point(&points, point);
                               break;
                             }
      case XCB_BUTTON_RELEASE: {
                               xcb_button_press_event_t *b = (xcb_button_press_event_t *)ev;
                               break;
                             }

    }
    xcb_flush(conn);
    free(ev);
  }
post_event_loop:
  xcb_key_symbols_free(syms);
  free_point_arr(&points);
  xcb_cookie = xcb_free_gc_checked(conn, white_gc);
  check_xcb_cookie(conn, xcb_cookie, "freeing white_gc");
  xcb_cookie = xcb_free_gc_checked(conn, fontgc);
  check_xcb_cookie(conn, xcb_cookie, "freeing font gc");
  xcb_cookie = xcb_free_gc_checked(conn, fontgc_black);
  check_xcb_cookie(conn, xcb_cookie, "freeing font gc");
  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}


void //?
free_point_arr(struct m_point_arr *points){
    free(points->elems);
}

void
push_point(struct m_point_arr *points,  xcb_point_t point){
  points->elems[points->idx] = point;
  points->idx++;
  if (points->idx >= points->cap){
    points->elems = realloc(points->elems, sizeof( xcb_point_t) * (points->cap*=2));
  }
}

void
update_rects_geometry(xcb_connection_t *conn, xcb_drawable_t *win, xcb_rectangle_t *rect){
  
  xcb_generic_error_t *err;
  xcb_get_geometry_cookie_t geo_cookie = xcb_get_geometry(conn, *win);
  xcb_get_geometry_reply_t *g_reply = xcb_get_geometry_reply(conn, geo_cookie, &err);
  if(err != NULL){
    fprintf(stderr, "get_geometry: %d", err->error_code);
    free(err);
    exit(EXIT_FAILURE);
  }
  
  rect->width = g_reply->width - (2*MARGIN);
  rect->height = g_reply->height -(2*MARGIN);
  free(g_reply);
}

void
cprintscr(xcb_connection_t *conn, 
    xcb_drawable_t *win,
    xcb_gcontext_t *fontgc,
    const char c
    ){
    char w[2] = {c, '\0'};
    if( c != '\r'){
      xcb_cookie = xcb_image_text_8_checked(conn, 1, *win, *fontgc, cursor.x, cursor.y, w);
      cursor.x += char_width;
    }else{
      cursor.x = MARGIN;
      cursor.y += char_width + 4;
    }
}


/*
 * Print line by line to the screen, unfinished, gets very slow
 */
void
sprintscr(xcb_connection_t *conn,
    xcb_drawable_t *win,
    xcb_gcontext_t *fontgc,
    int startx,
    int starty,
    const char *text) {
  if (strlen(text) == 0 ){
    return;
  }
  char * textcpy = strdup(text);
  int LINEHEIGHT = char_width + 2; //temporary hardcoded
  char *line = strtok(textcpy, "\n\r");
  while(line != NULL){
    xcb_cookie = xcb_image_text_8_checked(conn, strlen(line), *win, *fontgc, startx, starty, line);
    check_xcb_cookie(conn, xcb_cookie, "drawing text");
    starty+=LINEHEIGHT;
    line = strtok(NULL, "\n\r");
  }
  free(textcpy);
}

/*
 * Creates graphics context for a font
 * Also sets global @char_width
 * @param font_name font name
 * @param color foreground
 * ~@parma color background
 *
 */
xcb_gcontext_t
init_xcb_font_gc(xcb_connection_t *conn, xcb_drawable_t *win, const char *font_name, uint32_t col){
  //Setup
  const xcb_setup_t *setup = xcb_get_setup(conn);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_screen_t *screen = iter.data;
  //Open a a font
  // Adding XBC_GC_FONT to the gc's valuemask allows it to draw fonts
  xcb_font_t f = xcb_generate_id(conn);

  //Open the font
  xcb_cookie = xcb_open_font_checked(conn, f, strlen(font_name), font_name);
  check_xcb_cookie(conn, xcb_cookie, "opening font");
  //Create GC for the font
  xcb_gcontext_t fontgc = xcb_generate_id(conn); 
  uint32_t gcmask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
  uint32_t gcvalues[3] = { col, ~col , f};
//  uint32_t gcvalues[3] = { screen->black_pixel, screen->white_pixel , f};
  xcb_cookie = xcb_create_gc_checked(conn, fontgc, *win, gcmask, gcvalues);
  check_xcb_cookie(conn, xcb_cookie, "create gc");

  //Get font info
  xcb_query_font_cookie_t qfont_cookie = xcb_query_font(conn, f);
  xcb_generic_error_t *err;
  xcb_query_font_reply_t *qfont_reply = xcb_query_font_reply(conn, qfont_cookie, &err);
  xcb_charinfo_t * font_char_info =  xcb_query_font_char_infos (qfont_reply);
  char_width = font_char_info->character_width;
  //Ugly hack for now to correctly set the starting y pos
  //TODO, clean this (entire function honestly) up
  if(cursor.y < MARGIN + char_width + 2){
    cursor.y = MARGIN + char_width + 2;
  }
  
  free(qfont_reply);
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
