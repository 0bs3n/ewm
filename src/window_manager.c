#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "window_manager.h"

#define MOD Mod4Mask
#define NO_WINDOW 0x0

// TODO: utilize XGrabButton() and XAllowEvents()/XSendEvents() instead of current set up
// grabbing and ungrabbing buttons

ewm_instance wm;
XWindowAttributes saved_window_state;

void 
ewm_init()
{
    wm._display = XOpenDisplay(NULL);

    if (!wm._display) {
        printf("Could not open display %s\n", XDisplayName(NULL));
    } 
    wm._root = DefaultRootWindow(wm._display);
    init_array(&wm._window_list.a, 1);
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

    printf("Returned Root: %lu\n", returned_root);
    unsigned int i;
    if (nchildren > 2) {
        for (i = 1; i < nchildren; ++i) {
            printf("%lu\n", children[i]);
            push_array(&wm._window_list.a, children[i]);
            XSelectInput(
                wm._display,
                children[i],
                EnterWindowMask);
        }
        XSetInputFocus(
            wm._display, 
            wm._window_list.a.array[wm._window_list.a.used - 1], 
            RevertToPointerRoot, 
            CurrentTime);
    }
    print_array(&wm._window_list.a);
    wm._fullscreen_flag = 0;
}

void
ewm_run()
{
    XSelectInput(
        wm._display, 
        wm._root, 
        // Below is the line that is causing BadAccess on X_ChangeWindowAttributes,
        // however if SubstructureRedirectMask is not selected on root, then all 
        // wm functions only work when root is in focus.
        // TODO: allow child windows to configurerequest stuff
        SubstructureRedirectMask | 
        // StructureNotifyMask |
        SubstructureNotifyMask);
        // KeyPressMask | KeyReleaseMask |
        // ButtonPressMask | ButtonReleaseMask | 
        // OwnerGrabButtonMask);

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

    XGrabKey(
        wm._display, 
        XKeysymToKeycode(wm._display, XK_Return),
        MOD,
        wm._root,
        1,
        GrabModeAsync,
        GrabModeAsync);

    XGrabKey(
        wm._display, 
        XKeysymToKeycode(wm._display, XK_f),
        MOD,
        wm._root,
        1,
        GrabModeAsync,
        GrabModeAsync);

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
            ewm_on_create_notify(&e.xcreatewindow);
            break;
        case DestroyNotify:
            printf("DestroyNotify\n");
            break;
        case ReparentNotify:
            printf("ReparentNotify\n");
            break;
        case MapNotify:
            // It looks like when applications that use menus (or any 
            // window created by a client that is not the WM)
            // cause an X BadAccess Error. this happens after below,
            // but before the first line of on_map_request.
            printf("Mapping window: %lu\n override redirect: %d\n", e.xmap.window, e.xmap.override_redirect);
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
            // XMapWindow(wm._display, e.xmaprequest.window);
            ewm_on_map_request(&e.xmaprequest);
            break;
        case ConfigureRequest:
            printf("ConfigureRequest\n");
            ewm_on_configure_request(&e.xconfigurerequest);
            break;
        case ButtonPress:
            printf("ButtonPress\n");
            ewm_on_button_press(&e.xbutton);
            break;
        case ButtonRelease:
            printf("ButtonRelease\n");
            // ewm_on_button_release(&e.xbutton);
            wm._current_clicked_window = NO_WINDOW;
            break;
        case MotionNotify:
            ewm_on_motion_notify(&e.xmotion);
            break;
        case KeyPress:
            ewm_on_key_press(&e.xkey);
            break;
        case KeyRelease:
            break;
        case EnterNotify:
            ewm_on_enter_notify(&e.xcrossing);
            break;
        default:
            printf("Something else: %d\n", e.type);
            break;
        }
    }
}

void
ewm_on_create_notify(const XCreateWindowEvent *e)
{
    printf("window %lu: requesting creation\n", e->window);
}

void
ewm_on_map_request(const XMapRequestEvent *e)
{
    XWindowAttributes wa;
    if (!XGetWindowAttributes(wm._display, e->window, &wa))
        return;
    if(wa.override_redirect) {
        XMapWindow(wm._display, e->window);
        return;
    }

    printf("Mapping window: %lu\n", e->window);
    XMapWindow(wm._display, e->window);
    printf("Pushing array\n");
    push_array(&wm._window_list.a, e->window);
    printf("Setting input focus\n");
    XSetInputFocus(wm._display, e->window, RevertToPointerRoot, CurrentTime);
    printf("Selecting input\n");
    XSelectInput(
        wm._display,
        e->window,
        EnterWindowMask);
    /*
        | KeyPressMask);
        KeyPressMask | KeyReleaseMask |
        EnterWindowMask | ButtonPressMask | 
        ButtonReleaseMask | ButtonMotionMask | 
        OwnerGrabButtonMask);
        */
    /*
    print_array(&wm._window_list.a);
    */
}

void
ewm_on_configure_request(const XConfigureRequestEvent *e)
{
    printf("Configure Request sent by client: %lu\n", e->window);

}

void
ewm_on_enter_notify(const XEnterWindowEvent *e)
{
    if (e->window == 0) printf("Root window\n");

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

    if ((e->state & MOD) &&
        e->keycode == XKeysymToKeycode(wm._display, XK_f)) {
        // printf("Making window %lu fullscreen\n", e->subwindow);
        Window focus_window;
        int revert_return;
        XGetInputFocus(wm._display, &focus_window, &revert_return);
        printf("Current focused window: %lu\n", focus_window);
        fullscreen(focus_window);
    }

}

void
ewm_on_unmap_notify(const XUnmapEvent *e)
{
    remove_array(&wm._window_list.a, e->window);
    print_array(&wm._window_list.a);

    // Currently this creates behaviour where the next most recently created window
    // gains focus after a window is unmapped. This works as I intended but
    // is probably not the most intuitive way to do it.
    // 
    // TODO: figure out a more intuitive way to inherit focus once tiling functions are written.
    
    if (wm._window_list.a.used > 0) {
        XSetInputFocus(wm._display, wm._window_list.a.array[wm._window_list.a.used - 1], RevertToPointerRoot, CurrentTime);
        printf("Focusing on window: %lu\n", wm._window_list.a.array[wm._window_list.a.used - 1]);
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

/*
void
ewm_on_button_release(const XButtonEvent *e)
{
    wm._current_clicked_window = NO_WINDOW;
}
*/

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
    free_array(&wm._window_list.a);
}

void
fullscreen(const Window w)
{
    if (!w) 
        return;

    if (!wm._fullscreen_flag) {

        XGetWindowAttributes(wm._display, w, &wm._fullscreen_window);
         
        printf("Making window %lu fullscreen\n", w);
        Window returned_root;
        int x, y;
        unsigned int border_width, depth, width, height;
        XGetGeometry(wm._display, wm._root, &returned_root, &x, &y, &width, &height, &border_width, &depth);
        XMoveResizeWindow(wm._display, w, x, y, width, height);
        wm._fullscreen_flag = 1;
    } else {
        XMoveResizeWindow(
                wm._display, 
                w, 
                wm._fullscreen_window.x, 
                wm._fullscreen_window.y, 
                wm._fullscreen_window.width, 
                wm._fullscreen_window.height);

        wm._fullscreen_flag = 0;
        printf("Making window %lu not fullscreen\n", w);
    }
    
}
