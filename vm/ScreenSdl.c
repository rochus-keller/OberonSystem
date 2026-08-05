/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Project Oberon System project.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

// adopted from Micron oakwood

#include <SDL2/SDL.h>
#include <assert.h>
#include <string.h>

#ifdef _WIN32
#define DllExport __declspec(dllexport)
#else
#define DllExport
#endif

// Sync enums with Screen.mic!!!
enum {
    FLAG_LSB_FIRST = 1,
    FLAG_BOTTOM_UP = 2
};

enum EvtType {
    EVT_NONE       = 0,
    EVT_MOUSE_MOVE = 1,
    EVT_MOUSE_DOWN = 2,
    EVT_MOUSE_UP   = 3,
    EVT_KEY_DOWN   = 4,
    EVT_KEY_UP     = 5,
    EVT_KEY_CHAR   = 6
};

enum { BTN_LEFT = 1, BTN_MIDDLE = 2, BTN_RIGHT = 3 };

// Abstract key codes (must match Screen.mic)
enum {
    KEY_BACKSPACE = 0x100,
    KEY_TAB       = 0x101,
    KEY_RETURN    = 0x102,
    KEY_ESCAPE    = 0x103,
    KEY_DELETE    = 0x104,
    KEY_LEFT      = 0x105,
    KEY_RIGHT     = 0x106,
    KEY_UP        = 0x107,
    KEY_DOWN      = 0x108,
    KEY_LSHIFT    = 0x110,
    KEY_RSHIFT    = 0x111,
    KEY_LCTRL     = 0x112,
    KEY_RCTRL     = 0x113,
    KEY_CAPSLOCK  = 0x114
};

enum { QUEUE_LEN = 512 };

static int32_t evtQueue[QUEUE_LEN];
static int evtHead = 0, evtTail = 0, evtCount = 0;

// PS/2 set 2 scan code queue, filled in parallel to the abstract key events;
// systems talking to the keyboard controller directly (e.g. Oberon) read this one
static uint8_t scanQueue[QUEUE_LEN];
static int scanHead = 0, scanTail = 0, scanCount = 0;

static void scanEnqueue(uint8_t b)
{
    if( scanCount == QUEUE_LEN )
        return;
    scanQueue[scanHead] = b;
    scanHead = (scanHead + 1) % QUEUE_LEN;
    scanCount++;
}

// SDL scan codes are USB HID usage IDs, i.e. independent of the keyboard layout,
// and thus can be mapped to PS/2 set 2 codes by a fixed table.
// Returns the make code, or 0 if the key has no set 2 representation;
// *ext is set if the code is prefixed by E0.
static uint8_t ps2FromScanCode(SDL_Scancode sc, int* ext)
{
    *ext = 0;
    switch( sc )
    {
    case SDL_SCANCODE_A: return 0x1C;
    case SDL_SCANCODE_B: return 0x32;
    case SDL_SCANCODE_C: return 0x21;
    case SDL_SCANCODE_D: return 0x23;
    case SDL_SCANCODE_E: return 0x24;
    case SDL_SCANCODE_F: return 0x2B;
    case SDL_SCANCODE_G: return 0x34;
    case SDL_SCANCODE_H: return 0x33;
    case SDL_SCANCODE_I: return 0x43;
    case SDL_SCANCODE_J: return 0x3B;
    case SDL_SCANCODE_K: return 0x42;
    case SDL_SCANCODE_L: return 0x4B;
    case SDL_SCANCODE_M: return 0x3A;
    case SDL_SCANCODE_N: return 0x31;
    case SDL_SCANCODE_O: return 0x44;
    case SDL_SCANCODE_P: return 0x4D;
    case SDL_SCANCODE_Q: return 0x15;
    case SDL_SCANCODE_R: return 0x2D;
    case SDL_SCANCODE_S: return 0x1B;
    case SDL_SCANCODE_T: return 0x2C;
    case SDL_SCANCODE_U: return 0x3C;
    case SDL_SCANCODE_V: return 0x2A;
    case SDL_SCANCODE_W: return 0x1D;
    case SDL_SCANCODE_X: return 0x22;
    case SDL_SCANCODE_Y: return 0x35;
    case SDL_SCANCODE_Z: return 0x1A;
    case SDL_SCANCODE_1: return 0x16;
    case SDL_SCANCODE_2: return 0x1E;
    case SDL_SCANCODE_3: return 0x26;
    case SDL_SCANCODE_4: return 0x25;
    case SDL_SCANCODE_5: return 0x2E;
    case SDL_SCANCODE_6: return 0x36;
    case SDL_SCANCODE_7: return 0x3D;
    case SDL_SCANCODE_8: return 0x3E;
    case SDL_SCANCODE_9: return 0x46;
    case SDL_SCANCODE_0: return 0x45;
    case SDL_SCANCODE_MINUS: return 0x4E;
    case SDL_SCANCODE_EQUALS: return 0x55;
    case SDL_SCANCODE_LEFTBRACKET: return 0x54;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x5B;
    case SDL_SCANCODE_BACKSLASH: return 0x5D;
    case SDL_SCANCODE_NONUSHASH: return 0x5D;
    case SDL_SCANCODE_SEMICOLON: return 0x4C;
    case SDL_SCANCODE_APOSTROPHE: return 0x52;
    case SDL_SCANCODE_GRAVE: return 0x0E;
    case SDL_SCANCODE_COMMA: return 0x41;
    case SDL_SCANCODE_PERIOD: return 0x49;
    case SDL_SCANCODE_SLASH: return 0x4A;
    case SDL_SCANCODE_NONUSBACKSLASH: return 0x61;
    case SDL_SCANCODE_SPACE: return 0x29;
    case SDL_SCANCODE_TAB: return 0x0D;
    case SDL_SCANCODE_RETURN: return 0x5A;
    case SDL_SCANCODE_BACKSPACE: return 0x66;
    case SDL_SCANCODE_ESCAPE: return 0x76;
    case SDL_SCANCODE_CAPSLOCK: return 0x58;
    case SDL_SCANCODE_LSHIFT: return 0x12;
    case SDL_SCANCODE_RSHIFT: return 0x59;
    case SDL_SCANCODE_LCTRL: return 0x14;
    case SDL_SCANCODE_LALT: return 0x11;
    case SDL_SCANCODE_RCTRL: *ext = 1; return 0x14;
    case SDL_SCANCODE_RALT: *ext = 1; return 0x11;
    case SDL_SCANCODE_LGUI: *ext = 1; return 0x1F;
    case SDL_SCANCODE_RGUI: *ext = 1; return 0x27;
    case SDL_SCANCODE_APPLICATION: *ext = 1; return 0x2F;
    case SDL_SCANCODE_F1: return 0x05;
    case SDL_SCANCODE_F2: return 0x06;
    case SDL_SCANCODE_F3: return 0x04;
    case SDL_SCANCODE_F4: return 0x0C;
    case SDL_SCANCODE_F5: return 0x03;
    case SDL_SCANCODE_F6: return 0x0B;
    case SDL_SCANCODE_F7: return 0x83;
    case SDL_SCANCODE_F8: return 0x0A;
    case SDL_SCANCODE_F9: return 0x01;
    case SDL_SCANCODE_F10: return 0x09;
    case SDL_SCANCODE_F11: return 0x78;
    case SDL_SCANCODE_F12: return 0x07;
    case SDL_SCANCODE_SCROLLLOCK: return 0x7E;
    case SDL_SCANCODE_INSERT: *ext = 1; return 0x70;
    case SDL_SCANCODE_HOME: *ext = 1; return 0x6C;
    case SDL_SCANCODE_PAGEUP: *ext = 1; return 0x7D;
    case SDL_SCANCODE_DELETE: *ext = 1; return 0x71;
    case SDL_SCANCODE_END: *ext = 1; return 0x69;
    case SDL_SCANCODE_PAGEDOWN: *ext = 1; return 0x7A;
    case SDL_SCANCODE_UP: *ext = 1; return 0x75;
    case SDL_SCANCODE_LEFT: *ext = 1; return 0x6B;
    case SDL_SCANCODE_DOWN: *ext = 1; return 0x72;
    case SDL_SCANCODE_RIGHT: *ext = 1; return 0x74;
    case SDL_SCANCODE_NUMLOCKCLEAR: return 0x77;
    case SDL_SCANCODE_KP_DIVIDE: *ext = 1; return 0x4A;
    case SDL_SCANCODE_KP_MULTIPLY: return 0x7C;
    case SDL_SCANCODE_KP_MINUS: return 0x7B;
    case SDL_SCANCODE_KP_PLUS: return 0x79;
    case SDL_SCANCODE_KP_ENTER: *ext = 1; return 0x5A;
    case SDL_SCANCODE_KP_PERIOD: return 0x71;
    case SDL_SCANCODE_KP_0: return 0x70;
    case SDL_SCANCODE_KP_1: return 0x69;
    case SDL_SCANCODE_KP_2: return 0x72;
    case SDL_SCANCODE_KP_3: return 0x7A;
    case SDL_SCANCODE_KP_4: return 0x6B;
    case SDL_SCANCODE_KP_5: return 0x73;
    case SDL_SCANCODE_KP_6: return 0x74;
    case SDL_SCANCODE_KP_7: return 0x6C;
    case SDL_SCANCODE_KP_8: return 0x75;
    case SDL_SCANCODE_KP_9: return 0x7D;
    default:
        return 0;
    }
}

// Feeds the set 2 code sequence of a key press or release to the scan code queue:
// make is [E0] code, break is [E0] F0 code.
static void scanKey(SDL_Scancode sc, int down)
{
    int ext;
    const uint8_t code = ps2FromScanCode(sc, &ext);
    if( code == 0 )
        return;
    if( ext )
        scanEnqueue(0xE0);
    if( !down )
        scanEnqueue(0xF0);
    scanEnqueue(code);
}

static void evtEnqueue(int32_t e)
{
    if( evtCount == QUEUE_LEN )
        return;
    evtQueue[evtHead] = e;
    evtHead = (evtHead + 1) % QUEUE_LEN;
    evtCount++;
}

static int32_t evtDequeue(void)
{
    int32_t e;
    if( evtCount == 0 )
        return 0;
    e = evtQueue[evtTail];
    evtTail = (evtTail + 1) % QUEUE_LEN;
    evtCount--;
    return e;
}

static int32_t packEvent(int type, int data)
{
    return (type << 24) | (data & 0x00FFFFFF);
}

static int32_t packMouseMove(int px, int py)
{
    return packEvent(EVT_MOUSE_MOVE, ((px & 0xFFF) << 12) | (py & 0xFFF));
}

static SDL_Window*   window   = NULL;
static SDL_Texture*  texture  = NULL;
static SDL_Renderer* renderer = NULL;
static uint8_t* buffer  = NULL;
static int      bufLen  = 0;
static uint8_t* pixelBuf = NULL;
static int WIDTH = 0, HEIGHT = 0;
static unsigned int flags = 0;
static int sdlInitialized = 0;

// Dirty rectangle tracking
static SDL_Rect dirtyArea = {0, 0, 0, 0};
static int isDirty = 0;

// Mouse state
static int mouseX = 0, mouseY = 0;
static int btnLeft = 0, btnMid = 0, btnRight = 0;

// Modifier tracking (for Ctrl+Q quit shortcut)
static int ctrlDown = 0;

// Custom cursor
static SDL_Cursor* customCursor = NULL;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static void updateTexture(SDL_Rect* patch)
{
    int ax, ay, aw, ah, sw, dw, maxRow;

    if( patch->w <= 0 || patch->h <= 0 )
        return;

    if( flags & FLAG_LSB_FIRST )
    {
        // Oberon-style: LSB-first, 32-bit word aligned
        sw = (WIDTH / 32) * 4;
    } else
    {
        // Smalltalk-style: MSB-first, 16-bit word aligned
        const int pixPerWord = 16;
        const int pixLineWidth = ((WIDTH + pixPerWord - 1) / pixPerWord) * pixPerWord;
        sw = pixLineWidth / 8;
    }

    dw = WIDTH * 4;
    maxRow = bufLen / sw;
    if( maxRow > HEIGHT )
        maxRow = HEIGHT;

    ax = patch->x;
    aw = patch->w;
    ah = patch->y + patch->h;
    if( ah > maxRow )
        ah = maxRow;
    if( ah > HEIGHT )
        ah = HEIGHT;
    ay = patch->y;
    if( ay >= ah )
        return;

    for( int y = ay; y < ah; y++ )
    {
        int srcLine, dstLine;
        uint32_t* p;

        if( flags & FLAG_BOTTOM_UP )
        {
            srcLine = (HEIGHT - 1 - y);
            dstLine = y;
        } else
        {
            srcLine = y;
            dstLine = y;
        }

        const uint8_t* src = buffer + sw * srcLine;
        p = (uint32_t*)(pixelBuf + dw * dstLine) + ax;

        if( flags & FLAG_LSB_FIRST )
        {
            // Oberon: bit 0 is leftmost pixel within each 32-bit word
            for( int x = ax; x < ax + aw; x++ )
            {
                const int wordIdx = x / 32;
                const int bitIdx  = x % 32;
                uint32_t word = ((uint32_t*)src)[wordIdx];
                uint32_t v = (word >> bitIdx) & 1;
                *p++ = v ? 0xFFFFFFFF : 0xFF000000;
            }
        } else
        {
            // Smalltalk: MSB-first within each byte
            for( int x = ax; x < ax + aw; x++ )
            {
                const int byteIdx = x >> 3;
                if( byteIdx < sw )
                {
                    uint8_t v = src[byteIdx];
                    v = (v >> (7 - (x & 7))) & 1;
                    *p = v ? 0xFF000000 : 0xFFFFFFFF;
                }
                p++;
            }
        }
    }

    // Upload modified rectangle to GPU
    {
        uint8_t* patchPixels = pixelBuf + (ay * dw) + (ax * 4);
        SDL_UpdateTexture(texture, patch, patchPixels, dw);
    }
}

static int mapKeyCode(SDL_Keycode sym)
{
    switch( sym )
    {
    case SDLK_BACKSPACE:
        return KEY_BACKSPACE;
    case SDLK_TAB:
        return KEY_TAB;
    case SDLK_RETURN:
        return KEY_RETURN;
    case SDLK_ESCAPE:
        return KEY_ESCAPE;
    case SDLK_DELETE:
        return KEY_DELETE;
    case SDLK_LEFT:
        return KEY_LEFT;
    case SDLK_RIGHT:
        return KEY_RIGHT;
    case SDLK_UP:
        return KEY_UP;
    case SDLK_DOWN:
        return KEY_DOWN;
    case SDLK_LSHIFT:
        return KEY_LSHIFT;
    case SDLK_RSHIFT:
        return KEY_RSHIFT;
    case SDLK_LCTRL:
        return KEY_LCTRL;
    case SDLK_RCTRL:
        return KEY_RCTRL;
    case SDLK_CAPSLOCK:
        return KEY_CAPSLOCK;
    case SDLK_SPACE:
        return ' ';
    }
    if( sym >= SDLK_a && sym <= SDLK_z )
        return sym;
    if( sym >= SDLK_0 && sym <= SDLK_9 )
        return sym;
    if( sym >= 0x20 && sym <= 0x7E )
        return sym;
    return 0;
}

static int decodeUtf8char(const char* encoded)
{
    int c = (unsigned char)encoded[0];
    if( c <= 0x7F )
        return c;
    if( c >= 0xC0 && c <= 0xDF )
        return ((c & 0x1F) << 6) | ((unsigned char)encoded[1] & 0x3F);
    return '?';
}

static int mapButton(int sdlButton)
{
    switch( sdlButton )
    {
    case SDL_BUTTON_LEFT:
        return BTN_LEFT;
    case SDL_BUTTON_MIDDLE:
        return BTN_MIDDLE;
    case SDL_BUTTON_RIGHT:
        return BTN_RIGHT;
    default:
        return 0;
    }
}

static void disposeWindow(void)
{
    if( texture )
        SDL_DestroyTexture(texture);
    if( renderer )
        SDL_DestroyRenderer(renderer);
    if( window )
        SDL_DestroyWindow(window);
    window   = NULL;
    renderer = NULL;
    texture  = NULL;
}

#ifndef _MIC_NO_BEGIN_
DllExport void Screen$begin$(void)
{
    // NOP; SDL is initialized lazily in Open
}
#endif

DllExport int32_t Screen$Open(uint8_t* buf, int32_t bLen, int32_t w, int32_t h, unsigned int fl)
{
    if( !sdlInitialized )
    {
        if( SDL_Init(SDL_INIT_VIDEO) < 0 )
        {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 0;
        }
        sdlInitialized = 1;
    }

    disposeWindow();

    WIDTH  = w;
    HEIGHT = h;
    buffer = buf;
    bufLen = bLen;
    flags  = fl;

    window = SDL_CreateWindow("RV32 VM Screen (SDL)",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WIDTH, HEIGHT,
                              SDL_WINDOW_SHOWN);
    if( !window )
    {
        SDL_Log("Cannot create window: %s", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if( !renderer )
    {
        SDL_Log("Cannot create renderer: %s", SDL_GetError());
        disposeWindow();
        return 0;
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                WIDTH, HEIGHT);
    if( !texture )
    {
        SDL_Log("Cannot create texture: %s", SDL_GetError());
        disposeWindow();
        return 0;
    }

    if( pixelBuf )
        free(pixelBuf);
    pixelBuf = (uint8_t*)malloc(WIDTH * HEIGHT * 4);
    if( pixelBuf )
        memset(pixelBuf, 0xFF, WIDTH * HEIGHT * 4);

    // Reset event queue
    evtHead = evtTail = evtCount = 0;
    scanHead = scanTail = scanCount = 0;

    // Reset mouse state
    mouseX = mouseY = 0;
    btnLeft = btnMid = btnRight = 0;
    ctrlDown = 0;

    // Mark whole screen dirty for initial render
    isDirty = 1;
    dirtyArea.x = 0;
    dirtyArea.y = 0;
    dirtyArea.w = w;
    dirtyArea.h = h;

    return 1;
}

DllExport void Screen$Close(void)
{
    disposeWindow();
    if( pixelBuf )
    {
        free(pixelBuf);
        pixelBuf = NULL;
    }
    if( customCursor )
    {
        SDL_FreeCursor(customCursor);
        customCursor = NULL;
    }
    buffer = NULL;
    bufLen = 0;
}

DllExport void Screen$UpdateArea(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int left, top, right, bottom;

    if( w <= 0 || h <= 0 )
        return;

    // Clamp to screen bounds
    left   = MAX(x, 0);
    top    = MAX(y, 0);
    right  = MIN(x + w, WIDTH);
    bottom = MIN(y + h, HEIGHT);
    if( left >= right || top >= bottom )
        return;

    if( !isDirty )
    {
        dirtyArea.x = left;
        dirtyArea.y = top;
        dirtyArea.w = right - left;
        dirtyArea.h = bottom - top;
        isDirty = 1;
    } else
    {
        const int oldRight  = dirtyArea.x + dirtyArea.w;
        const int oldBottom = dirtyArea.y + dirtyArea.h;
        dirtyArea.x = MIN(dirtyArea.x, left);
        dirtyArea.y = MIN(dirtyArea.y, top);
        dirtyArea.w = MAX(oldRight, right) - dirtyArea.x;
        dirtyArea.h = MAX(oldBottom, bottom) - dirtyArea.y;
    }
}

DllExport int32_t Screen$ProcessEvents(int32_t sleep)
{
    SDL_Event e;
    SDL_Rect r;
    int hasEvent;
    int newEvents = 0;

    if( !window )
        return -1;

    if( isDirty )
    {
        updateTexture(&dirtyArea);
        SDL_RenderClear(renderer);
        r.x = 0; r.y = 0; r.w = WIDTH; r.h = HEIGHT;
        SDL_RenderCopy(renderer, texture, &r, &r);
        SDL_RenderPresent(renderer);
        isDirty = 0;
        dirtyArea.x = dirtyArea.y = dirtyArea.w = dirtyArea.h = 0;
    }

    hasEvent = (sleep > 0) ? SDL_WaitEventTimeout(&e, sleep) : SDL_PollEvent(&e);

    // Compress mouse movement: track final position
    int mouseMoved = 0;
    int finalX = mouseX, finalY = mouseY;

    while( hasEvent )
    {
        switch( e.type )
        {
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
            return -1;

        case SDL_WINDOWEVENT:
            if( e.window.event == SDL_WINDOWEVENT_CLOSE )
                return -1;
            break;

        case SDL_MOUSEMOTION:
            finalX = MAX(MIN(e.motion.x, WIDTH - 1), 0);
            finalY = MAX(MIN(e.motion.y, HEIGHT - 1), 0);
            mouseMoved = 1;
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            const int down = (e.button.state == SDL_PRESSED);
            const int btn = mapButton(e.button.button);
            if( btn )
            {
                if( btn == BTN_LEFT )   btnLeft  = down;
                if( btn == BTN_MIDDLE ) btnMid   = down;
                if( btn == BTN_RIGHT )  btnRight = down;
                evtEnqueue(packEvent(down ? EVT_MOUSE_DOWN : EVT_MOUSE_UP, btn));
                newEvents++;
            }
            break;
        }

        case SDL_TEXTINPUT:
        {
            const int ch = decodeUtf8char(e.text.text);
            if( ch > 0 )
            {
                evtEnqueue(packEvent(EVT_KEY_CHAR, ch));
                newEvents++;
            }
            break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            const int down = (e.key.state == SDL_PRESSED);
            SDL_Keycode sym = e.key.keysym.sym;

            // Track Ctrl for Ctrl+Q quit
            if( sym == SDLK_LCTRL || sym == SDLK_RCTRL )
                ctrlDown = down;
            if( sym == SDLK_q && down && ctrlDown )
                return -1;

            {
                const int code = mapKeyCode(sym);
                if( code )
                {
                    evtEnqueue(packEvent(down ? EVT_KEY_DOWN : EVT_KEY_UP, code));
                    newEvents++;
                }
            }
            scanKey(e.key.keysym.scancode, down);
            break;
        }
        } // switch

        hasEvent = SDL_PollEvent(&e);
    }

    // Push compressed mouse movement as a single event
    if( mouseMoved )
    {
        if( finalX != mouseX || finalY != mouseY )
        {
            evtEnqueue(packMouseMove(finalX, finalY));
            newEvents++;
        }
        mouseX = finalX;
        mouseY = finalY;
    }

    return newEvents;
}

DllExport int32_t Screen$NextEvent(void)
{
    return evtDequeue();
}

DllExport int32_t Screen$GetMouseState(int32_t* x, int32_t* y)
{
    if( x )
        *x = mouseX;
    if( y )
        *y = mouseY;
    return (btnLeft ? 4 : 0) | (btnMid ? 2 : 0) | (btnRight ? 1 : 0);
}

DllExport void Screen$SetCursor(uint8_t* bits, int32_t w, int32_t h,
                                 int32_t hotX, int32_t hotY)
{
    if( customCursor )
    {
        SDL_FreeCursor(customCursor);
        customCursor = NULL;
    }

    if( bits && w > 0 && h > 0 )
    {
        customCursor = SDL_CreateCursor(bits, bits, w, h, hotX, hotY);
        SDL_SetCursor(customCursor);
    } else
    {
        SDL_SetCursor(SDL_GetDefaultCursor());
    }
}

DllExport void Screen$SetCursorPos(int32_t x, int32_t y)
{
    if( window )
        SDL_WarpMouseInWindow(window, x, y);
}

DllExport void Screen$ShowCursor(int32_t on)
{
    SDL_ShowCursor(on ? SDL_ENABLE : SDL_DISABLE);
}

DllExport uint32_t Screen$GetTicks(void)
{
    return SDL_GetTicks();
}

DllExport int32_t Screen$NextScanCode(void)
{
    int32_t b;
    if( scanCount == 0 )
        return -1;
    b = scanQueue[scanTail];
    scanTail = (scanTail + 1) % QUEUE_LEN;
    scanCount--;
    return b;
}
