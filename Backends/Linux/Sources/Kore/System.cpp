#include "pch.h"
#include <Kore/System.h>
#include <cstring>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include "System_Screens.h"

#include <stdio.h>
#include <stdlib.h>

#include <GL/glx.h>
#include <GL/gl.h>

#include <X11/keysym.h>

//apt-get install mesa-common-dev
//apt-get install libgl-dev

namespace {
    //static int snglBuf[] = {GLX_RGBA, GLX_DEPTH_SIZE, 16, None};
    //static int dblBuf[]  = {GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};

    Display* dpy;
    Window win;
    GLboolean doubleBuffer = GL_TRUE;

    void fatalError(const char* message) {
        printf("main: %s\n", message);
        exit(1);
    }
}

namespace appimpl {
	bool running = false;
	const char * name = "KoreApplication";

	void (*callback)();
	void (*foregroundCallback)();
	void (*backgroundCallback)();
	void (*pauseCallback)();
	void (*resumeCallback)();
	void (*shutdownCallback)();
	void (*orientationCallback)(Kore::Orientation);
}

namespace windowimpl {
    struct OsWindow {
        Window handle;
        int x, y;
        int width, height;

        OsWindow( Window handle, int x, int y, int width, int height ) {
            this->handle = handle;
            this->x = x;
            this->y = y;
            this->width = width;
            this->height = height;
        }
    };

    OsWindow* windows[10] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    int windowCounter = -1;
}

namespace Kore { namespace System {
	void setup() {
		Monitor::enumerate();
	}

	void start() {
		appimpl::running = true;

#if !defined(SYS_HTML5) && !defined(SYS_TIZEN)
		// if (Graphics::hasWindow()) Graphics::swapBuffers();
		while (appimpl::running) {
			appimpl::callback();
			handleMessages();
		}
#endif
	}

	void stop() {
		appimpl::running = false;

		// TODO (DK) destroy graphics, but afaik Application::~Application() was never called, so it's the same behavior now as well

		//for (int windowIndex = 0; windowIndex < sizeof(windowIds) / sizeof(int); ++windowIndex) {
		//	Graphics::destroy(windowIndex);
		//}
	}

	bool isFullscreen() {
		// TODO (DK)
		return false;
	}

	void setCallback( void (*value)() ) {
		appimpl::callback = value;
	}

	void setForegroundCallback( void (*value)() ) {
		appimpl::foregroundCallback = value;
	}

	void setResumeCallback( void (*value)() ) {
		appimpl::resumeCallback = value;
	}

	void setPauseCallback( void (*value)() ) {
		appimpl::pauseCallback = value;
	}

	void setBackgroundCallback( void (*value)() ) {
		appimpl::backgroundCallback = value;
	}

	void setShutdownCallback( void (*value)() ) {
		appimpl::shutdownCallback = value;
	}

	void setOrientationCallback( void (*value)(Orientation) ) {
		appimpl::orientationCallback = value;
	}
}}

namespace Kore { namespace System {
    int windowCount() {
        return windowimpl::windowCounter + 1;
    }

    int windowWidth( int id ) {
        return windowimpl::windows[id]->width;
    }

    int windowHeight( int id ) {
        return windowimpl::windows[id]->height;
    }

    int initWindow( WindowOptions options ) {
        return createWindow(options.title, options.x, options.y, options.width, options.height, options.mode, options.targetDisplay, options.rendererOptions.depthBufferBits, options.rendererOptions.stencilBufferBits);
    }

    void* windowHandle( int id ) {
        // TODO (DK) throw new std::exception("implement me");
        return nullptr;
    }
}}

namespace Kore { namespace System {
    int currentDevice() {
        // TODO (DK)    throw new std::exception("implement me");
        return -1;
    }

    void setCurrentDevice( int id ) {
        // TODO (DK) throw new std::exception("implement me");
    }
}}

void
Kore::System::setName( const char* name ) {
    appimpl::name = name; // TODO (DK) strcpy?
}

int
createWindow( const char * title, int x, int y, int width, int height, int windowMode, int targetDisplay, int depthBufferBits, int stencilBufferBits ) {
	XVisualInfo*         vi;
	Colormap             cmap;
	XSetWindowAttributes swa;
	GLXContext           cx;
	//XEvent               event;
	//GLboolean            needRedraw = GL_FALSE;
	//GLboolean            recalcModelView = GL_TRUE;
	int                  dummy;

	// (1) open a connection to the X server

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) fatalError("could not open display");

	// (2) make sure OpenGL's GLX extension supported

	if (!glXQueryExtension(dpy, &dummy, &dummy)) fatalError("X server has no OpenGL GLX extension");

	// (3) find an appropriate visual

	// find an OpenGL-capable RGB visual with depth buffer
    int snglBuf[] = {GLX_RGBA, GLX_DEPTH_SIZE, depthBufferBits, GLX_STENCIL_SIZE, stencilBufferBits, None};
    int dblBuf[]  = {GLX_RGBA, GLX_DEPTH_SIZE, depthBufferBits, GLX_STENCIL_SIZE, stencilBufferBits, GLX_DOUBLEBUFFER, None};

	vi = glXChooseVisual(dpy, DefaultScreen(dpy), dblBuf);

	if (vi == NULL) {
		vi = glXChooseVisual(dpy, DefaultScreen(dpy), snglBuf);

		if (vi == NULL) {
            fatalError("no RGB visual with valid depth/stencil buffer");
		}

		doubleBuffer = GL_FALSE;
	}
	//if(vi->class != TrueColor)
	//  fatalError("TrueColor visual required for this program");

	// (4) create an OpenGL rendering context

	// create an OpenGL rendering context
	cx = glXCreateContext(dpy, vi, /* no shared dlists */ None, /* direct rendering if possible */ GL_TRUE);

	if (cx == NULL) {
        fatalError("could not create rendering context");
	}

	// (5) create an X window with the selected visual

	// create an X colormap since probably not using default visual
	cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
	win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XSetStandardProperties(dpy, win, "main", "main", None, NULL, 0, NULL);

	// (6) bind the rendering context to the window

	glXMakeCurrent(dpy, win, cx);

	// (7) request the X window to be displayed on the screen

	XMapWindow(dpy, win);
    XMoveWindow(dpy, win, DisplayWidth(dpy, vi->screen) / 2 - width / 2, DisplayHeight(dpy, vi->screen) / 2 - height / 2);
	//Scheduler::addFrameTask(HandleMessages, 1001);

	return nullptr;
}

bool Kore::System::handleMessages() {
	while (XPending(dpy) > 0) {
		XEvent event;
		XNextEvent(dpy, &event);
		switch (event.type) {
		case KeyPress: {
			XKeyEvent* key = (XKeyEvent*)&event;
			KeySym keysym;
			char buffer[1];
			XLookupString(key, buffer, 1, &keysym, NULL);
			#define KEY(xkey, korekey, keychar) case xkey: Kore::Keyboard::the()->_keydown(Kore::korekey, keychar); break;
			switch (keysym) {
			KEY(XK_Right, Key_Right, ' ')
			KEY(XK_Left, Key_Left, ' ')
			KEY(XK_Up, Key_Up, ' ')
			KEY(XK_Down, Key_Down, ' ')
			KEY(XK_space, Key_Space, ' ')
			KEY(XK_a, Key_A, 'a')
			KEY(XK_b, Key_B, 'b')
			KEY(XK_c, Key_C, 'c')
			KEY(XK_d, Key_D, 'd')
			KEY(XK_e, Key_E, 'e')
			KEY(XK_f, Key_F, 'f')
			KEY(XK_g, Key_G, 'g')
			KEY(XK_h, Key_H, 'h')
			KEY(XK_i, Key_I, 'i')
			KEY(XK_j, Key_J, 'j')
			KEY(XK_k, Key_K, 'k')
			KEY(XK_l, Key_L, 'l')
			KEY(XK_m, Key_M, 'm')
			KEY(XK_n, Key_N, 'n')
			KEY(XK_o, Key_O, 'o')
			KEY(XK_p, Key_P, 'p')
			KEY(XK_q, Key_Q, 'q')
			KEY(XK_r, Key_R, 'r')
			KEY(XK_s, Key_S, 's')
			KEY(XK_t, Key_T, 't')
			KEY(XK_u, Key_U, 'u')
			KEY(XK_v, Key_V, 'v')
			KEY(XK_w, Key_W, 'w')
			KEY(XK_x, Key_X, 'x')
			KEY(XK_y, Key_Y, 'y')
			KEY(XK_z, Key_Z, 'z')
			case XK_Escape:
                System::stop();
				break;
			}
			break;
			#undef KEY
		}
		case KeyRelease: {
			XKeyEvent* key = (XKeyEvent*)&event;
			KeySym keysym;
			char buffer[1];
			XLookupString(key, buffer, 1, &keysym, NULL);
			#define KEY(xkey, korekey, keychar) case xkey: Kore::Keyboard::the()->_keyup(Kore::korekey, keychar); break;
			switch (keysym) {
			KEY(XK_Right, Key_Right, ' ')
			KEY(XK_Left, Key_Left, ' ')
			KEY(XK_Up, Key_Up, ' ')
			KEY(XK_Down, Key_Down, ' ')
			KEY(XK_space, Key_Space, ' ')
			KEY(XK_a, Key_A, 'a')
			KEY(XK_b, Key_B, 'b')
			KEY(XK_c, Key_C, 'c')
			KEY(XK_d, Key_D, 'd')
			KEY(XK_e, Key_E, 'e')
			KEY(XK_f, Key_F, 'f')
			KEY(XK_g, Key_G, 'g')
			KEY(XK_h, Key_H, 'h')
			KEY(XK_i, Key_I, 'i')
			KEY(XK_j, Key_J, 'j')
			KEY(XK_k, Key_K, 'k')
			KEY(XK_l, Key_L, 'l')
			KEY(XK_m, Key_M, 'm')
			KEY(XK_n, Key_N, 'n')
			KEY(XK_o, Key_O, 'o')
			KEY(XK_p, Key_P, 'p')
			KEY(XK_q, Key_Q, 'q')
			KEY(XK_r, Key_R, 'r')
			KEY(XK_s, Key_S, 's')
			KEY(XK_t, Key_T, 't')
			KEY(XK_u, Key_U, 'u')
			KEY(XK_v, Key_V, 'v')
			KEY(XK_w, Key_W, 'w')
			KEY(XK_x, Key_X, 'x')
			KEY(XK_y, Key_Y, 'y')
			KEY(XK_z, Key_Z, 'z')
			}
			break;
			#undef KEY
		}
		case ButtonPress: {
			XButtonEvent* button = (XButtonEvent*)&event;
			switch (button->button) {
			case Button1:
                Kore::Mouse::the()->_press(0, button->x, button->y);
                break;
            case Button3:
                Kore::Mouse::the()->_press(1, button->x, button->y);
                break;
			}
			break;
		}
		case ButtonRelease: {
			XButtonEvent* button = (XButtonEvent*)&event;
			switch (button->button) {
			case Button1:
                Kore::Mouse::the()->_release(0, button->x, button->y);
                break;
            case Button3:
                Kore::Mouse::the()->_release(1, button->x, button->y);
                break;
			}
			break;
		}
		case MotionNotify: {
			XMotionEvent* motion = (XMotionEvent*)&event;
			Kore::Mouse::the()->_move(motion->x, motion->y);
			break;
		}
		case ConfigureNotify:
			glViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
			// fall through...
		case Expose:
			//needRedraw = GL_TRUE;
			break;
		}
	}
	return true;
}

const char* Kore::System::systemId() {
    return "Linux";
}

/*void* Kore::System::windowHandle() {
    return (void*)win;
}*/

void Kore::System::swapBuffers( int contextId ) {
    // TODO (DK) correct contexts
    glXSwapBuffers(dpy, win);
}

void Kore::System::destroyWindow( int id ) {
    // TODO (DK) implement me
}

void Kore::System::changeResolution(int width, int height, bool fullscreen) {

}

void Kore::System::setTitle(const char* title) {

}

void Kore::System::showWindow() {

}

void Kore::System::showKeyboard() {

}

void Kore::System::hideKeyboard() {

}

void Kore::System::loadURL(const char* url) {

}

int Kore::System::screenWidth() {
    // TODO (DK) 0 instead of currentDevice()?
    return windowimpl::windows[currentDevice()]->width;
}

int Kore::System::screenHeight() {
    // TODO (DK) 0 instead of currentDevice()?
    return windowimpl::windows[currentDevice()]->height;
}

int Kore::System::desktopWidth() {
    return XWidthOfScreen(XDefaultScreenOfDisplay(XOpenDisplay(NULL)));
}

int Kore::System::desktopHeight() {
    return XHeightOfScreen(XDefaultScreenOfDisplay(XOpenDisplay(NULL)));
}

namespace {
    char save[2000];
    bool saveInitialized = false;
}

const char* Kore::System::savePath() {
    if (!saveInitialized) {
        strcpy(save, "Ķ~/.");
        strcat(save, appimpl::name);
        strcat(save, "/");
        saveInitialized = true;
    }
	return save;
}

namespace {
	const char* videoFormats[] = { "ogv", nullptr };
}

const char** Kore::System::videoFormats() {
	return ::videoFormats;
}

#include <sys/time.h>
#include <time.h>

double Kore::System::frequency() {
	return 1000000.0;
}

Kore::System::ticks Kore::System::timestamp() {
	timeval now;
	gettimeofday(&now, NULL);
	return static_cast<ticks>(now.tv_sec) * 1000000 + static_cast<ticks>(now.tv_usec);
}

extern int kore(int argc, char** argv);

int main(int argc, char** argv) {
    kore(argc, argv);
}
