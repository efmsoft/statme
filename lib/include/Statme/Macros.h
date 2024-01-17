#pragma once

#if defined(_WIN32) && !defined(_STATME_STATIC_BUILD_)
  #ifdef _STATME_DLL_BUILD_
    #define STATMELNK __declspec(dllexport)
  #else
    #define STATMELNK __declspec(dllimport)
  #endif
#else
  #define STATMELNK
#endif
