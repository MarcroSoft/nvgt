vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF c9e74722a58c470143d40b2542dcc3af9b1ccc7f
    SHA512 c7bb8a71e2ea68e039d2b545a93c4ecc46bb14dba9eb8b6123b01a7d689f0aeda1089d89a1226c4fd21982658015ceec7c507c3cffe0c50014b214097e20f770
    HEAD_REF master
    PATCHES
        add-no-compiler.patch
        mark-threads-private.patch
        fix-dependency.patch
        fix-double-literal-precision.patch
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
