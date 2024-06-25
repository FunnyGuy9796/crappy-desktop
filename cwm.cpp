#include "cwm.hpp"

CWM wm;
int dragging = 0;

int errorHandler(Display *dpy, XErrorEvent *ev) {
    char errorText[1024];
    XGetErrorText(dpy, ev->error_code, errorText, sizeof(errorText));
    cerr << "X Error: " << errorText << endl;

    cerr << "  Request code: " << int(ev->request_code) << endl;
    cerr << "  Error code: " << int(ev->error_code) << endl;

    cerr << "  Serial number of failed request: " << int(ev->serial) << endl;
    
    return 0;
}

void CWM::createCursor(int cursor) {
    if (wm.csr != None) {
        cout << "Cursor exists, changing cursor" << endl;
        XFreeCursor(wm.dpy, wm.csr);
    }

    wm.csr = XCreateFontCursor(wm.dpy, 2);
    XDefineCursor(wm.dpy, wm.root, wm.csr);
    cout << "Cursor creation successfull" << endl;
}

Window CWM::createWindow(Window parent, int x, int y, int width, int height) {
    Window win = XCreateSimpleWindow(wm.dpy, parent, x, y, width, height, 2, BlackPixel(wm.dpy, wm.screen_num), WhitePixel(wm.dpy, wm.screen_num));
    XSelectInput(wm.dpy, win, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
    EnterWindowMask | LeaveWindowMask | FocusChangeMask | StructureNotifyMask | PropertyChangeMask);

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    XChangeWindowAttributes(wm.dpy, win, CWOverrideRedirect, &attrs);

    XMapWindow(wm.dpy, win);
    cout << "Window creation successfull: " << win << endl;
    return win;
}

void sendConfigureRequest(Display *dpy, Window win, int x, int y, int width, int height) {
    Atom atom = XInternAtom(dpy, "_NET_WM_MOVERESIZE", False);
    if (atom == None) {
        cerr << "Cannot find _NET_WM_MOVERESIZE atom" << endl;
        return;
    }

    XClientMessageEvent ev;
    ev.type = ClientMessage;
    ev.window = win;
    ev.message_type = atom;
    ev.format = 32;
    ev.data.l[0] = static_cast<long>(3);
    ev.data.l[1] = static_cast<long>(x);
    ev.data.l[2] = static_cast<long>(y);
    ev.data.l[3] = static_cast<long>(width);
    ev.data.l[4] = static_cast<long>(height);

    XSendEvent(dpy, win, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&ev);
}

void handleCreateNotify(Display *dpy, XEvent *ev) {
    if (ev->type == CreateNotify) {
        if (ev->xcreatewindow.override_redirect) {
            cout << "Transient window created: " << ev->xcreatewindow.window << endl;
        } else {
            cout << "Regular window created: " << ev->xcreatewindow.window << endl;
        }
    }
}

void handleMapRequest(Display *dpy, XEvent *ev) {
    if (ev->type == MapRequest) {
        XMapWindow(dpy, ev->xmaprequest.window);
        cout << "MapRequest event handled: " << ev->xmaprequest.window << endl;
    }
}

void handleMapNotify(Display *dpy, XEvent *ev) {
    if (ev->type == MapNotify) {
        XWindowAttributes attrs;
        XGetWindowAttributes(dpy, ev->xmap.window, &attrs);
        if (!attrs.override_redirect) {
            Window frame = wm.createWindow(wm.root, attrs.x, attrs.y, attrs.width, attrs.height + 35);
            XReparentWindow(dpy, ev->xmap.window, frame, 0, 35);
            wm.windowMap[ev->xmap.window] = frame;
            cout << "Regular window reparented: " << ev->xmap.window << endl;
        } else {
            cout << "Transient window raised and focused: " << ev->xmap.window << endl;
        }

        cout << "MapNotify event handled: " << ev->xmap.window << endl;
    }
}

void handleButtonPress(Display *dpy, XEvent *ev) {
    if (ev->type == ButtonPress) {
        switch (ev->xbutton.button) {
            case Button1:
                wm.startEvent = &ev->xbutton;
                wm.offsetX = wm.startEvent->x;
                wm.offsetY = wm.startEvent->y;
                dragging = 1;
        }

        XWindowAttributes attrs;
        XGetWindowAttributes(dpy, ev->xmap.window, &attrs);
        if (!attrs.override_redirect) {
            XSetInputFocus(dpy, ev->xbutton.window, RevertToParent, CurrentTime);
            XRaiseWindow(dpy, ev->xbutton.window);
        }

        cout << "Button pressed: " << ev->xbutton.button << endl;
    }
}

void handleButtonRelease(Display *dpy, XEvent *ev) {
    if (ev->type == ButtonRelease) {
        dragging = 0;
        cout << "Button released: " << ev->xbutton.button << endl;
    }
}

void handleMotionNotify(Display *dpy, XEvent *ev) {
    if (ev->type == MotionNotify) {
        if (dragging == 1) {
            int newX = ev->xmotion.x_root - wm.offsetX;
            int newY = ev->xmotion.y_root - wm.offsetY;
            XMoveWindow(dpy, ev->xmotion.window, newX, newY);
        }
    }
}

void handleConfigureRequest(Display *dpy, XEvent *ev) {
    if (ev->type == ConfigureRequest) {
        XConfigureRequestEvent* cre = &(ev->xconfigurerequest);
        
        XWindowChanges changes;
        changes.x = cre->x;
        changes.y = cre->y;
        changes.width = cre->width;
        changes.height = cre->height;
        changes.border_width = cre->border_width;
        changes.sibling = cre->above;
        changes.stack_mode = cre->detail;

        XConfigureWindow(dpy, cre->window, cre->value_mask, &changes);
        cout << "ConfigureRequest event handled: " <<
        cre->window << " {x: " << changes.x << " y: " << changes.y << " width: " << changes.width << " height: "
        << changes.height << " border_width: " << changes.border_width << " sibling: " << changes.sibling << " stack_mode: " << changes.stack_mode << endl;

        Window frame = cre->parent;

        XWindowChanges pChanges;
        pChanges.x = cre->x;
        pChanges.y = cre->y - 35;
        pChanges.width = cre->width;
        pChanges.height = cre->height + 35;
        pChanges.border_width = cre->border_width;
        pChanges.sibling = cre->above;
        pChanges.stack_mode = cre->detail;
        XConfigureWindow(dpy, frame, cre->value_mask, &pChanges);
    }
}

void handleFocusIn(Display *dpy, XEvent *ev) {
    if (ev->type == FocusIn) {
        cout << "FocusIn handled: " << ev->xfocus.window << endl;
    }
}

void handleDestroyNotify(Display *dpy, XEvent *ev) {
    if (ev->type == DestroyNotify) {
        Window destroyedWindow = ev->xdestroywindow.window;
        cout << "DestroyNotify event handled: " << destroyedWindow << endl;
    }
}

int main() {
    wm.dpy = XOpenDisplay(nullptr);
    if (wm.dpy == nullptr) {
        cerr << "Unable to open X display" << endl;
        return 1;
    }

    XSetErrorHandler(errorHandler);

    wm.screen_num = DefaultScreen(wm.dpy);
    wm.screen = XScreenOfDisplay(wm.dpy, wm.screen_num);
    wm.visual = DefaultVisual(wm.dpy, DefaultScreen(wm.dpy));
    wm.depth = DefaultDepth(wm.dpy, DefaultScreen(wm.dpy));
    wm.root = RootWindow(wm.dpy, wm.screen_num);
    XSelectInput(wm.dpy, wm.root, SubstructureRedirectMask | SubstructureNotifyMask);

    wm.createCursor(2);

    XEvent ev;
    while (true) {
        XNextEvent(wm.dpy, &ev);
        switch (ev.type) {
            case CreateNotify:
                handleCreateNotify(wm.dpy, &ev);
                break;
            case MapRequest:
                handleMapRequest(wm.dpy, &ev);
                break;
            case MapNotify:
                handleMapNotify(wm.dpy, &ev);
                break;
            case ButtonPress:
                handleButtonPress(wm.dpy, &ev);
                break;
            case ButtonRelease:
                handleButtonRelease(wm.dpy, &ev);
                break;
            case MotionNotify:
                handleMotionNotify(wm.dpy, &ev);
                break;
            case ConfigureRequest:
                handleConfigureRequest(wm.dpy, &ev);
                break;
            case FocusIn:
                handleFocusIn(wm.dpy, &ev);
                break;
            case DestroyNotify:
                handleDestroyNotify(wm.dpy, &ev);
                break;
        }
    }

    XFreeCursor(wm.dpy, wm.csr);
    XCloseDisplay(wm.dpy);
    return 0;
}