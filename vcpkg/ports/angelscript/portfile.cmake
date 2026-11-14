vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO anjo76/angelscript
    REF ba9f9075319fa8b360e54122134d9ac9d163278f
    SHA512 bcc8aa91c1f698ef3a236113622322c4ce47ddd8a417fa87352991cd48e2dc0b6efbe36162d64e09e8c089f08d1ca5107b61895f671c646666d5cc6211ef7895
    HEAD_REF master
    PATCHES
        mark-threads-private.patch
        fix-dependency.patch
        fix-double-literal-precision.patch
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
