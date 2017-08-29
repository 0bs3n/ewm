#include <X11/Xlib.h>
#include <stdio.h>
#include "window_manager.h"


void ewm_init();

int 
main(int argc, char **argv) 
{
    ewm_init();
    ewm_run();
    ewm_cleanup();
    return 0; 
}

