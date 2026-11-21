vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF 84cb262387fc797a5f67814b2821001da60a78f6
    SHA512 e64df2d8432740da42bd5b201ee211a5289a9860306dab230eab7fd56d6a505c6c8f6d173f4f4742cb66ebdac2223cc62da9e2043842a189fe3016a1e2bfc00c
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
