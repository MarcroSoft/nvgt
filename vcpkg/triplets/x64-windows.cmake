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
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS_RELEASE -DCMAKE_POLICY_VERSION_MINIMUM=3.15 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded)
	if(NOT PORT MATCHES "^(openssl|flatbuffers)$")
		list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS_RELEASE -DCMAKE_BUILD_TYPE=MinSizeRel "-DCMAKE_CXX_FLAGS_MINSIZEREL=/O1 /Ob1 /DNDEBUG /Oi /GS-")
	endif()
endif()
# Prism only converts short strings between UTF-8 and UTF-16 with simdutf, so its scalar implementation is plenty and leaving out the SIMD ones saves around 300 KB.
if(PORT STREQUAL "simdutf")
	set(VCPKG_C_FLAGS "")
	set(VCPKG_CXX_FLAGS "/DSIMDUTF_IMPLEMENTATION_HASWELL=0 /DSIMDUTF_IMPLEMENTATION_WESTMERE=0 /DSIMDUTF_IMPLEMENTATION_ICELAKE=0")
endif()
# Poco uses Windows' own TLS (Schannel) rather than OpenSSL for HTTPS, keeping OpenSSL out of NVGT on Windows, where it was the largest part of the executable. This is enabled directly rather than through the POCO_ENABLE_NETSSL_WIN triplet variable, which needs the netssl feature that depends on Poco's OpenSSL based crypto library.
if(PORT STREQUAL "poco")
	list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS -DENABLE_NETSSL_WIN=ON)
endif()
