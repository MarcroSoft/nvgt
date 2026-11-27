vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF d0f4fbdd3fc5e5366250d65edeba43b629e10069
    SHA512 510afdf98e79afa983de894df5582767d0598bd624d7a208bc0ebd3561febfc8a14d261f9679767f7c1362f9ad3c5e9cf68f136f241055e9c76f50e4a070e3df
    HEAD_REF master
    PATCHES
        mark-threads-private.patch
        fix-dependency.patch
        # samtupy/angelscript commits not yet in anjo76 (scoped reference type fixes, asEP_ENABLE_VALUE_TYPED_COMPOUND_PROPERTY_ACCESSORS), needed by the Jolt math types
        samtupy-scoped-reference-types.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/sdk/angelscript/projects/cmake"
    OPTIONS
        "-DCMAKE_CXX_STANDARD=11"
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/Angelscript)

# Copy the addon files
if("addons" IN_LIST FEATURES)
    file(INSTALL "${SOURCE_PATH}/sdk/add_on/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/angelscript" FILES_MATCHING PATTERN "*.h" PATTERN "*.cpp")
endif()
