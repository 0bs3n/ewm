#include <X11/Xlib.h>
#include <stdio.h>

typedef struct {
    Display *_display;
    Window _root;
} ewm_instance;

void ewm_init();
void ewm_run();
void ewm_cleanup();

void ewm_on_map_request(const XMapRequestEvent *e);
void ewm_on_button_press(const XButtonEvent *e);
void ewm_on_key_press(const XKeyEvent *e);
void ewm_on_motion_notify(const XMotionEvent *e);
