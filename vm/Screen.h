#ifndef _MIC_SCREEN_
#define _MIC_SCREEN_

#include <stdint.h>

#ifdef _WIN32
#define DllExport __declspec(dllexport)
#else
#define DllExport
#endif

DllExport void Screen$begin$();
DllExport int32_t Screen$Open(uint8_t* buf, int32_t bLen, int32_t w, int32_t h, unsigned int fl);
DllExport void Screen$Close();
DllExport void Screen$UpdateArea(int32_t x, int32_t y, int32_t w, int32_t h);
DllExport int32_t Screen$ProcessEvents(int32_t sleep);
DllExport int32_t Screen$NextEvent();
DllExport int32_t Screen$GetMouseState(int32_t* x, int32_t* y);
DllExport void Screen$SetCursor(uint8_t* bits, int32_t w, int32_t h, int32_t hotX, int32_t hotY);
DllExport void Screen$SetCursorPos(int32_t x, int32_t y);
DllExport void Screen$ShowCursor(int32_t on);
DllExport uint32_t Screen$GetTicks();
DllExport int32_t Screen$NextScanCode();

#endif // _MIC_SCREEN_
