#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "window_manager.h"


ewm_instance wm;

void 
ewm_init()
{
    wm._display = XOpenDisplay(NULL);

    if (!wm._display) {
        printf("Could not open display %s\n", XDisplayName(NULL));
    } 
    wm._root = DefaultRootWindow(wm._display);
}

void
ewm_run()
{
    XSelectInput(wm._display, 
                 wm._root, 
                 SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(wm._display, 0);
    XGrabServer(wm._display);
    Window returned_root, returned_parent;
    Window *top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(wm._display,
               wm._root,
               &returned_root,
               &returned_parent,
               &top_level_windows,
               &num_top_level_windows);
    XFree(top_level_windows);
    XUngrabServer(wm._display);
    XGrabButton(
        wm._display,
        Button1,
        AnyModifier,
        wm._root,
        0,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);

    for (;;) {
        XEvent e;
        XNextEvent(wm._display, &e);

        switch (e.type) {
        case CreateNotify:
            printf("CreateNotify\n");
            break;
        case DestroyNotify:
            printf("DestroyNotify\n");
            break;
        case ReparentNotify:
            printf("ReparentNotify\n");
            break;
        case MapNotify:
            printf("Mapping Window\n");
            break;
        case UnmapNotify:
            printf("UnmapNotify\n");
            break;
        case ConfigureNotify:
            printf("ConfigureNotify\n");
            break;
        case MapRequest:
            printf("MapRequest\n");
            ewm_on_map_request(&e.xmaprequest);
            break;
        case ConfigureRequest:
            printf("ConfigureRequest\n");
            break;
        case ButtonPress:
            printf("ButtonPress\n");
            ewm_on_button_press(&e.xbutton);
            break;
        case ButtonRelease:
            printf("ButtonRelease\n");
            break;
        case MotionNotify:
            printf("MotionNotify\n");
            ewm_on_motion_notify(&e.xmotion);
            break;
        case KeyPress:
            printf("KeyPress\n");
            ewm_on_key_press(&e.xkey);
            break;
        case KeyRelease:
            printf("KeyRelease\n");
            break;
        default:
            printf("Something else\n");
        }
    }
}

void
ewm_on_map_request(const XMapRequestEvent *e)
{
    XSelectInput(
            wm._display,
            e->window,
            SubstructureNotifyMask | 
            SubstructureRedirectMask | 
            KeyPressMask | KeyReleaseMask);

    XMapWindow(wm._display, e->window);
    
}

void
ewm_on_button_press(const XButtonEvent *e)
{
    //XDestroyWindow(wm._display, e->window);
    printf("Mouse clicked\n");
    // ewm_cleanup(); 
}

void
ewm_on_key_press(const XKeyEvent *e)
{
    printf("Destroying window\n");
    XDestroyWindow(wm._display, e->window);
}
void
ewm_on_motion_notify(const XMotionEvent *e)
{
    printf("Motion Event Occuring\n");
}

void 
ewm_cleanup()
{
    XCloseDisplay(wm._display);
}
