#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <unistd.h>

#define msleep( x ) usleep(1000*x)

int rc = 0;

int main(int argc, char ** argv){
  
  const char * win_name = "my first window";

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
  Window win = XCreateSimpleWindow(dpy, root, 
      1920/2, 1080/2,
      100, 100,
      0, 0,
      0
      );
  XMapWindow(dpy, win);
  rc = XStoreName(dpy, win, win_name);
  XFlush(dpy);
  for(;;){
    msleep(500);
  }

  return EXIT_SUCCESS;
}
