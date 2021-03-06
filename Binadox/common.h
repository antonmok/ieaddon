
#ifndef __COMMON_H__
#define __COMMON_H__

#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <tchar.h>

// CLSID in string format
#define CLSID_IEPlugin_Str _T("{3543619C-D563-43f7-95EA-4DA7E1CC396A}")
extern const CLSID CLSID_IEPlugin;
extern volatile LONG DllRefCount;
extern HINSTANCE hInstance;

#endif // __COMMON_H__
