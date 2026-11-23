vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF b02d6c9bf12aa23ed7231a05e0549748cc9d2942
    SHA512 f2e3fa2eb2c4b2f00272b7d4bc36ccb290cabd0407dbed245053805243f1f395763580009363372bfd0684911d4366c537b71df16e74796fda5b7b411613ebea
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
