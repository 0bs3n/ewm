#include <X11/Xlib.h>
#include <stdio.h>
#include "array.h"

typedef struct {
    int x;
    int y;
} Vector;


typedef struct {
    unsigned int width;
    unsigned int height;
} Size;

typedef struct {
    Array a;
    int mapped;
} window_list;


typedef struct {
    Display *_display;
    Window _root;
    Vector _cursor_start_position;
    Vector _window_start_position;
    Size _window_start_size;
    window_list _window_list;
    Window _current_clicked_window;
    XWindowAttributes _fullscreen_window;
    int _fullscreen_flag;
} ewm_instance;


void ewm_init();
void ewm_run();
void ewm_cleanup();

void fullscreen(const Window w);

void ewm_on_map_request(const XMapRequestEvent *e);
void ewm_on_configure_request(const XConfigureRequestEvent *e);
void ewm_on_unmap_notify(const XUnmapEvent *e);
void ewm_on_button_press(const XButtonEvent *e);
void ewm_on_button_release(const XButtonEvent *e);
void ewm_on_key_press(const XKeyEvent *e);
void ewm_on_motion_notify(const XMotionEvent *e);
void ewm_on_enter_notify(const XEnterWindowEvent *e);
void ewm_on_create_notify(const XCreateWindowEvent *e);


