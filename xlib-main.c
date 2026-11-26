#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <unistd.h>

#define msleep( x ) usleep(1000*x)

typedef union{
  unsigned char colors[4]; //BGR null
  unsigned long data;

}color_t;

int rc = 0;

int main(int argc, char ** argv){
  
  const char * win_name = "my first window";
  static const int width, height = 100;
  static const int sx, sy = 0;
  color_t bg = {};
  bg.colors[0] = 0;
  bg.colors[1] = 0;
  bg.colors[2] = 255;

  Display *dpy = XOpenDisplay(NULL);
  if(dpy == NULL){
    perror("XOpenDisplay");
    exit(EXIT_FAILURE);
  }
  int winno = 0;
  Window root = XRootWindow(dpy, winno);
/*
*  extern Window XCreateSimpleWindow(
*      Display*		 display 
*      Window		 parent 
*      int			 x 
*      int			 y 
*      unsigned int	 width 
*      unsigned int	 height 
*      unsigned int	 border_width 
*      unsigned long	 border 
*      unsigned long	 background
*      );
*/
  Window win = XCreateWindow(dpy, root, 
      sx, sy,
      width, height,
      0, 0,
      bg.data
      );
  XMapWindow(dpy, win);
  rc = XStoreName(dpy, win, win_name);
  XFlush(dpy);
  for(;;){
    msleep(500);
  }

  return EXIT_SUCCESS;
}
