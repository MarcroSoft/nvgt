vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF 5f687a387d2917c9db9b47119a625f6e5a5732d5
    SHA512 cb2470a727e6584e8023dc3b4a5e16e65dd16de1be09fca02c574feca656740620b7637d485c28ae8323b3b85d011ef989d1bb738941566dbe8cdfadc75113e3
    HEAD_REF master
    PATCHES
        add-no-compiler.patch
        mark-threads-private.patch
        fix-dependency.patch
        # samtupy/angelscript commits not yet in anjo76 (scoped reference type fixes, asEP_ENABLE_VALUE_TYPED_COMPOUND_PROPERTY_ACCESSORS), needed by the Jolt math types
        samtupy-scoped-reference-types.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/sdk/angelscript/projects/cmake"
    OPTIONS
        "-DAS_NO_COMPILER=ON" "-DCMAKE_CXX_STANDARD=11"
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/Angelscript)

# Copy the addon files
if("addons" IN_LIST FEATURES)
    file(INSTALL "${SOURCE_PATH}/sdk/add_on/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/angelscript" FILES_MATCHING PATTERN "*.h" PATTERN "*.cpp")
endif()
file(REMOVE "${CURRENT_PACKAGES_DIR}/include/angelscript.h")
