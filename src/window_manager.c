#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "window_manager.h"

#define MOD ControlMask
#define NO_WINDOW 0x0

ewm_instance wm;

void 
ewm_init()
{
    wm._display = XOpenDisplay(NULL);

    if (!wm._display) {
        printf("Could not open display %s\n", XDisplayName(NULL));
    } 
    wm._root = DefaultRootWindow(wm._display);
    init_array(&wm._window_list, 1);
    print_array(&wm._window_list);
}

void
ewm_run()
{
    XSelectInput(
        wm._display, 
        wm._root, 
        SubstructureRedirectMask | SubstructureNotifyMask | 
        KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | 
        OwnerGrabButtonMask);

    XSync(wm._display, 0);
    XGrabServer(wm._display);
    
    Window returned_root, returned_parent;
    Window *top_level_windows;
    unsigned int num_top_level_windows;

    XQueryTree(
        wm._display,
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
        MOD,
        wm._root,
        1,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    
    XGrabButton(
        wm._display,
        Button3,
        MOD,
        wm._root,
        1,
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
            ewm_on_unmap_notify(&e.xunmap);
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
            ewm_on_button_release(&e.xbutton);
            break;
        case MotionNotify:
            ewm_on_motion_notify(&e.xmotion);
            break;
        case KeyPress:
            printf("KeyPress\n");
            ewm_on_key_press(&e.xkey);
            break;
        case KeyRelease:
            printf("KeyRelease\n");
            break;
        case EnterNotify:
            ewm_on_enter_notify(&e.xcrossing);
            break;
        default:
            printf("Something else\n");
        }
    }
}

void
ewm_on_map_request(const XMapRequestEvent *e)
{
    printf("Mapping window: %lu\n", e->window);

    XMapWindow(wm._display, e->window);
    push_array(&wm._window_list, e->window);
    XSetInputFocus(wm._display, e->window, RevertToPointerRoot, CurrentTime);
    XSelectInput(
        wm._display,
        e->window,
        KeyPressMask | KeyReleaseMask |
        EnterWindowMask | ButtonPressMask | 
        ButtonReleaseMask | ButtonMotionMask | 
        OwnerGrabButtonMask);
    print_array(&wm._window_list);
}

void
ewm_on_enter_notify(const XEnterWindowEvent *e)
{
    printf("Entered window: %lu\n", e->window);
    XSetInputFocus(wm._display, e->window, RevertToParent, CurrentTime);

    Window *children;
    Window returned_root;
    Window returned_parent;
    unsigned int nchildren;
    XQueryTree(
        wm._display,
        wm._root,
        &returned_root,
        &returned_parent,
        &children,
        &nchildren);

    if (e->window != children[nchildren - 1]) {
        XGrabButton(
            wm._display,
            Button1,
            AnyModifier,
            wm._root,
            1,
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
            GrabModeAsync,
            GrabModeAsync,
            None,
            None);
        printf("Button Grabbed\n");
    }
}


void
ewm_on_key_press(const XKeyEvent *e)
{
    if ((e->state & MOD) && 
        e->keycode == XKeysymToKeycode(wm._display, XK_q)) {
            printf("Destroying window\n");
            XDestroyWindow(wm._display, e->window);
    }

    if ((e->state & MOD) && 
         e->keycode == XKeysymToKeycode(wm._display, XK_Return)) {
        printf("Enter Works\n");
        if (fork() == 0) {
            char *args[] = {"urxvt", NULL};
            setsid();
            execvp(args[0], args);
                perror("execvp");
        }
    }

    if ((e->state & Mod4Mask) && 
        e->keycode == XKeysymToKeycode(wm._display, XK_d)) {
            printf("Opening rofi\n");
            system("dmenu_run");
    }
}

void
ewm_on_unmap_notify(const XUnmapEvent *e)
{
    remove_array(&wm._window_list, e->window);
    print_array(&wm._window_list);

    // Currently this creates behaviour where the next most recently created window
    // gains focus after a window is unmapped. This works as I intended but
    // is probably not the most intuitive way to do it.
    // 
    // TODO: figure out a more intuitive way to inherit focus once tiling functions are written.
    
    if (wm._window_list.used > 0) {
        XSetInputFocus(wm._display, wm._window_list.array[wm._window_list.used - 1], RevertToPointerRoot, CurrentTime);
        printf("Focusing on window: %lu\n", wm._window_list.array[wm._window_list.used - 1]);
    }
}


void
ewm_on_button_press(const XButtonEvent *e)
{
    if (e->subwindow != 0) {
        wm._current_clicked_window = e->subwindow;
        
        // Save initial cursor position;
        wm._cursor_start_position = (Vector){e->x_root, e->y_root};

        // Save initial window info
        Window returned_root;
        int x, y;
        unsigned int width, height, depth, border_width;
        XGetGeometry(wm._display,
                     e->subwindow,
                     &returned_root,
                     &x, &y, 
                     &width, &height,
                     &border_width,
                     &depth);
        wm._window_start_position = (Vector){x, y};
        wm._window_start_size = (Size){width, height};

        XRaiseWindow(wm._display, e->subwindow);
        XSetInputFocus(wm._display, e->subwindow, RevertToParent, CurrentTime);
        printf("Raising window %lu\n", e->subwindow);

        // TODO XSendEvent() may be able to pass the button event to the child window
        // foregoing the need for the current workaround (below)
        XUngrabButton(wm._display, Button1, AnyModifier, wm._root);
        XGrabButton(
            wm._display,
            Button1,
            MOD,
            wm._root,
            1,
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
            GrabModeAsync,
            GrabModeAsync,
            None,
            None);
        printf("Button Ungrabbed\n");
    }
}

void
ewm_on_button_release(const XButtonEvent *e)
{
    wm._current_clicked_window = NO_WINDOW;
}

void
ewm_on_motion_notify(const XMotionEvent *e)
{
    printf("Current wm.window: %lu\n", wm._current_clicked_window);
    //current cursor position
    const Vector drag_pos = {e->x_root, e->y_root};
    printf("x_root: %d, y_root: %d\n", e->x_root, e->y_root);

    // Change in pixels of the cursor since ButtonPressEvent set wm._cursor_start_position (last button press)
    const Vector delta = {
        (drag_pos.x - wm._cursor_start_position.x), 
        (drag_pos.y - wm._cursor_start_position.y)
    };
    printf("delta.x: %d, delta.y: %d\n", delta.x, delta.y);
    printf("cursor start.x: %d, cursor start.y: %d\n", wm._cursor_start_position.x, wm._cursor_start_position.y);


    if ((e->state & Button1Mask) && (e->state & MOD)) {
        const Vector dest_window_pos = {
            (wm._window_start_position.x + delta.x), 
            (wm._window_start_position.y + delta.y)
        };

        if (e->subwindow != 0) {
            XMoveWindow(wm._display, 
                    e->subwindow, 
                    dest_window_pos.x, 
                    dest_window_pos.y);
        }
    }

    else if ((e->state & Button3Mask) && e->state & MOD) {

        
        Vector size_delta = {
            (delta.x > (signed)-wm._window_start_size.width ? delta.x : (signed)-wm._window_start_size.width),
            (delta.y > (signed)-wm._window_start_size.height ? delta.y : (signed)-wm._window_start_size.height)
        };
        printf("size_delta x: %d\n", size_delta.x);
        printf("size_delta y: %d\n", size_delta.y);
        printf("window start size width: %d\n", wm._window_start_size.width);
        printf("window start size height: %d\n", wm._window_start_size.height);
            // size_delta.x = delta.x;
            // size_delta.y = delta.y;

        const Size dest_window_size = {
            wm._window_start_size.width + size_delta.x,
            wm._window_start_size.height + size_delta.y
        };
        printf("Current window: %lu\n", e->subwindow);

        if (dest_window_size.width > 0 
            && dest_window_size.height > 0 
            && wm._current_clicked_window != 0) {
                XResizeWindow(
                wm._display, 
                wm._current_clicked_window, 
                dest_window_size.width, 
                dest_window_size.height);
        }
    }
}

void 
ewm_cleanup()
{
    XCloseDisplay(wm._display);
    free_array(&wm._window_list);
}
