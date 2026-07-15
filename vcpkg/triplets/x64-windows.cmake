set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
if(PORT MATCHES "libgit" OR PORT MATCHES "libplist" OR PORT MATCHES "libarchive")
	set(VCPKG_LIBRARY_LINKAGE dynamic)
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_POLICY_VERSION_MINIMUM=3.15)
	# These ship as dlls next to nvgt, where the YY-Thunks object linked into our own binaries cannot help them, so
	# compile them targeting Windows 7 directly. Without this the SDK defaults to the newest Windows and libarchive
	# picks CreateFile2 (Windows 8+), which made archive.dll fail to load on Windows 7 with a missing entry point.
	set(VCPKG_C_FLAGS "/D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06010000")
	set(VCPKG_CXX_FLAGS "/D_WIN32_WINNT=0x0601 /DNTDDI_VERSION=0x06010000")
else()
	set(VCPKG_LIBRARY_LINKAGE static)
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS_DEBUG -DCMAKE_POLICY_VERSION_MINIMUM=3.15 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug)
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS_RELEASE -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_POLICY_VERSION_MINIMUM=3.15 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded "-DCMAKE_CXX_FLAGS=/Oi /GS-")
endif()
