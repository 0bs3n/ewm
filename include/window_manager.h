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
    Display *_display;
    Window _root;
    Vector _cursor_start_position;
    Vector _window_start_position;
    Size _window_start_size;
    Array _window_list;
    unsigned long _current_clicked_window;
} ewm_instance;


void ewm_init();
void ewm_run();
void ewm_cleanup();

void ewm_on_map_request(const XMapRequestEvent *e);
void ewm_on_unmap_notify(const XUnmapEvent *e);
void ewm_on_button_press(const XButtonEvent *e);
void ewm_on_button_release(const XButtonEvent *e);
void ewm_on_key_press(const XKeyEvent *e);
void ewm_on_motion_notify(const XMotionEvent *e);
void ewm_on_enter_notify(const XEnterWindowEvent *e);


