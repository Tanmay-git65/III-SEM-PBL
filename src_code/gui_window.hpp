#pragma once
#include "gui_app.hpp"

#if defined(MAZELAB_NO_GUI)

inline int runGuiWindow(GuiApp&) { return 1; }

#elif defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

class Win32Canvas : public Canvas {
public:
    explicit Win32Canvas(HDC deviceContext) : dc(deviceContext) {
        font = CreateFontA(-FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_NAME.c_str());
        previousFont = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        TEXTMETRICA metrics;
        GetTextMetricsA(dc, &metrics);
        lineHeight = metrics.tmHeight;
    }
    ~Win32Canvas() override {
        SelectObject(dc, previousFont);
        DeleteObject(font);
    }
    void fillRect(int x, int y, int width, int height, Color color) override {
        RECT area{x, y, x + width, y + height};
        HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
        FillRect(dc, &area, brush);
        DeleteObject(brush);
    }
    void fillCircle(int centerX, int centerY, int radius, Color color) override {
        HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
        HGDIOBJ previousBrush = SelectObject(dc, brush);
        HGDIOBJ previousPen = SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, centerX - radius, centerY - radius, centerX + radius + 1, centerY + radius + 1);
        SelectObject(dc, previousPen);
        SelectObject(dc, previousBrush);
        DeleteObject(brush);
    }
    void drawText(int x, int y, const std::string& text, Color color) override {
        SetTextColor(dc, RGB(color.r, color.g, color.b));
        TextOutA(dc, x, y, text.c_str(), (int)text.size());
    }
    int textWidth(const std::string& text) override {
        SIZE size;
        GetTextExtentPoint32A(dc, text.c_str(), (int)text.size(), &size);
        return size.cx;
    }
    int textHeight() override { return lineHeight; }
private:
    HDC dc;
    HFONT font;
    HGDIOBJ previousFont;
    int lineHeight = 16;
};

inline LRESULT CALLBACK mazeLabWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        CREATESTRUCTA* creation = reinterpret_cast<CREATESTRUCTA*>(lParam);
        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(creation->lpCreateParams));
        return DefWindowProcA(window, message, wParam, lParam);
    }
    GuiApp* app = reinterpret_cast<GuiApp*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    if (app == nullptr) return DefWindowProcA(window, message, wParam, lParam);
    switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC screen = BeginPaint(window, &paint);
            HDC buffer = CreateCompatibleDC(screen);
            HBITMAP bitmap = CreateCompatibleBitmap(screen, WINDOW_WIDTH, WINDOW_HEIGHT);
            HGDIOBJ previousBitmap = SelectObject(buffer, bitmap);
            {
                Win32Canvas canvas(buffer);
                app->draw(canvas);
            }
            BitBlt(screen, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, previousBitmap);
            DeleteObject(bitmap);
            DeleteDC(buffer);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_LBUTTONDOWN:
            app->onClick((short)LOWORD(lParam), (short)HIWORD(lParam));
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case WM_KEYDOWN:
            switch (wParam) {
                case VK_UP: app->onArrow(Up); break;
                case VK_RIGHT: app->onArrow(Right); break;
                case VK_DOWN: app->onArrow(Down); break;
                case VK_LEFT: app->onArrow(Left); break;
                default: return 0;
            }
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case WM_CHAR:
            app->onKeyChar((char)wParam);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

inline int runGuiWindow(GuiApp& app) {
    HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = mazeLabWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(32512));
    windowClass.lpszClassName = "MazeLabWindow";
    RegisterClassA(&windowClass);
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT frame{0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect(&frame, style, FALSE);
    HWND window = CreateWindowA("MazeLabWindow", WINDOW_TITLE.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top, nullptr, nullptr, instance, &app);
    if (window == nullptr) return 1;
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetForegroundWindow(window);
    MSG message;
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return 0;
}

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

class X11Canvas : public Canvas {
public:
    X11Canvas(Display* display, Drawable target, GC gc, XFontStruct* font)
        : display(display), target(target), gc(gc), font(font), visual(DefaultVisual(display, DefaultScreen(display))) {}
    void fillRect(int x, int y, int width, int height, Color color) override {
        if (width <= 0 || height <= 0) return;
        XSetForeground(display, gc, pixelFor(color));
        XFillRectangle(display, target, gc, x, y, (unsigned)width, (unsigned)height);
    }
    void fillCircle(int centerX, int centerY, int radius, Color color) override {
        XSetForeground(display, gc, pixelFor(color));
        XFillArc(display, target, gc, centerX - radius, centerY - radius, (unsigned)(2 * radius), (unsigned)(2 * radius), 0, 360 * 64);
    }
    void drawText(int x, int y, const std::string& text, Color color) override {
        XSetForeground(display, gc, pixelFor(color));
        XDrawString(display, target, gc, x, y + font->ascent, text.c_str(), (int)text.size());
    }
    int textWidth(const std::string& text) override { return XTextWidth(font, text.c_str(), (int)text.size()); }
    int textHeight() override { return font->ascent + font->descent; }
private:
    Display* display;
    Drawable target;
    GC gc;
    XFontStruct* font;
    Visual* visual;
    static unsigned long scaleToMask(unsigned value, unsigned long mask) {
        if (mask == 0) return 0;
        int shift = 0;
        while (!((mask >> shift) & 1)) ++shift;
        unsigned long maxValue = mask >> shift;
        return ((value * maxValue / 255) << shift) & mask;
    }
    unsigned long pixelFor(Color color) const {
        return scaleToMask(color.r, visual->red_mask) | scaleToMask(color.g, visual->green_mask) | scaleToMask(color.b, visual->blue_mask);
    }
};

inline XFontStruct* loadInterfaceFont(Display* display) {
    const char* candidates[] = {"-*-helvetica-medium-r-normal--14-*-*-*-*-*-*-*", "-*-dejavu sans-medium-r-normal--14-*-*-*-*-*-*-*", "9x15", "8x13", "fixed"};
    for (const char* name : candidates)
        if (XFontStruct* font = XLoadQueryFont(display, name)) return font;
    return nullptr;
}

inline int runGuiWindow(GuiApp& app) {
    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) return 1;
    XFontStruct* font = loadInterfaceFont(display);
    if (font == nullptr) {
        XCloseDisplay(display);
        return 1;
    }
    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XStoreName(display, window, WINDOW_TITLE.c_str());
    XSizeHints* hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = hints->max_width = WINDOW_WIDTH;
    hints->min_height = hints->max_height = WINDOW_HEIGHT;
    XSetWMNormalHints(display, window, hints);
    XFree(hints);
    Atom closeRequest = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &closeRequest, 1);
    XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);
    Pixmap buffer = XCreatePixmap(display, window, WINDOW_WIDTH, WINDOW_HEIGHT, DefaultDepth(display, screen));
    GC gc = XCreateGC(display, buffer, 0, nullptr);
    XSetFont(display, gc, font->fid);
    XMapWindow(display, window);
    auto redraw = [&]() {
        X11Canvas canvas(display, buffer, gc, font);
        app.draw(canvas);
        XCopyArea(display, buffer, window, gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0);
        XFlush(display);
    };
    bool running = true;
    while (running) {
        XEvent event;
        XNextEvent(display, &event);
        switch (event.type) {
            case Expose:
                if (event.xexpose.count == 0) redraw();
                break;
            case ButtonPress:
                if (event.xbutton.button == Button1) {
                    app.onClick(event.xbutton.x, event.xbutton.y);
                    redraw();
                }
                break;
            case KeyPress: {
                KeySym symbol = XLookupKeysym(&event.xkey, 0);
                if (symbol == XK_Up) app.onArrow(Up);
                else if (symbol == XK_Right) app.onArrow(Right);
                else if (symbol == XK_Down) app.onArrow(Down);
                else if (symbol == XK_Left) app.onArrow(Left);
                else {
                    char typed[8] = {0};
                    if (XLookupString(&event.xkey, typed, sizeof(typed), nullptr, nullptr) > 0) app.onKeyChar(typed[0]);
                }
                redraw();
                break;
            }
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == closeRequest) running = false;
                break;
        }
    }
    XFreeGC(display, gc);
    XFreePixmap(display, buffer);
    XFreeFont(display, font);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}

#endif
