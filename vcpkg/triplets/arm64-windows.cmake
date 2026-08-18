set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE static)
if(PORT MATCHES "libgit" OR PORT MATCHES "libplist" OR PORT MATCHES "libarchive")
	set(VCPKG_LIBRARY_LINKAGE dynamic)
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_POLICY_VERSION_MINIMUM=3.15)
else()
	set(VCPKG_LIBRARY_LINKAGE static)
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS_DEBUG -DCMAKE_POLICY_VERSION_MINIMUM=3.15 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug)
	set(VCPKG_CMAKE_CONFIGURE_OPTIONS_RELEASE -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_POLICY_VERSION_MINIMUM=3.15 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded "-DCMAKE_CXX_FLAGS=/Oi /GS-")
endif()

# Build Poco's NetSSL against SChannel rather than OpenSSL so that libcrypto/libssl do not have to be
# linked into nvgt at all. Windows already ships the TLS stack, and OpenSSL was contributing roughly
# 4.9 MB to every binary we produce, including the stub embedded into each compiled game.
set(POCO_ENABLE_NETSSL_WIN ON)
