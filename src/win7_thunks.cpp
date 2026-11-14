/* win7_thunks.cpp - runtime fallbacks for Windows 8+ APIs that YY-Thunks doesn't cover, so nvgt and compiled games still load on Windows 7
 *
 * NVGT - NonVisual Gaming Toolkit
 * Copyright (c) 2022-2025 Sam Tupy
 * https://nvgt.dev
 * This software is provided "as-is", without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/

#ifdef _WIN32 // This file can be lazy wildcarded into builds.
// Defining both the function and its __imp_ pointer in an object file means the linker never pulls the matching member out of the import library, so the executable has no static import of the API and the loader can't refuse to start on a Windows version without it. This is the same technique YY-Thunks uses.
#include <windows.h>
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

// Prism's UIA backend calls UiaDisconnectProvider when it shuts down, which only exists from Windows 8. On Windows 7 there is nothing to disconnect from, so reporting success is the correct fallback.
extern "C" HRESULT WINAPI UiaDisconnectProvider(IRawElementProviderSimple* pProvider) {
	using fn_t = HRESULT(WINAPI*)(IRawElementProviderSimple*);
	static fn_t fn = []() -> fn_t {
		HMODULE mod = LoadLibraryExW(L"uiautomationcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		return mod ? reinterpret_cast<fn_t>(GetProcAddress(mod, "UiaDisconnectProvider")) : nullptr;
	}();
	return fn ? fn(pProvider) : S_OK;
}
extern "C" decltype(&UiaDisconnectProvider) __imp_UiaDisconnectProvider = &UiaDisconnectProvider;

#endif // _WIN32
