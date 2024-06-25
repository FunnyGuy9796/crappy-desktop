#ifndef CWM_HPP
#define CWM_HPP

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <unordered_map>

using namespace std;

class CWM {
    public:
        Display *dpy;
        int screen_num;
        Screen *screen;
        Visual *visual;
        int depth;
        Window root;
        Cursor csr;
        int offsetX, offsetY;
        XButtonPressedEvent *startEvent;
        unordered_map<Window, Window> windowMap;

        void createCursor(int cursor);

        Window createWindow(Window parent, int x, int y, int width, int height);

        Window getParent(Display *dpy, Window child);
};

#endif