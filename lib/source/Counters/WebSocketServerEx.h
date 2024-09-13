#pragma once

#ifdef _WIN32
// HRingBuf class defined in hv has method with name "free". if _CRTDBG_MAP_ALLOC is defined, corecrt defined free 
// as _dbg_free for _DEBUG. It cause compile error. The code below forces inclusion of crtdbg to define free and then
// undefines it
#include <crtdbg.h>

#ifdef free
#undef free
#endif
#endif

#include <hv/WebSocketServer.h>
