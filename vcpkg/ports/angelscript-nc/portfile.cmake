vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF ac0efb107e40cdad36998a20e952d3a8c69c1a5b
    SHA512 26d603789f52ca8e932b6af177a45d4e8e79d6b65e1d3312ac66f24bdc89ee02d13d024503144bd2cd9a8a58c5757860b97d2a95f632aefa0724def2f1ca99dd
    HEAD_REF master
    PATCHES
        add-no-compiler.patch
        mark-threads-private.patch
        fix-dependency.patch
        fix-double-literal-precision.patch
        fix-msvc-target-platform.patch
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
