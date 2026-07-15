vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/Chuyu-Team/YY-Thunks/releases/download/v${VERSION}/YY-Thunks-Objs.zip"
    FILENAME "YY-Thunks-Objs-${VERSION}.zip"
    SHA512 31b3c383b49ccba0d8a2fc2bc581ec4503ea0269a87d1c6fd61f3a6f1c772864c4db95f67ed7661762e6b243555287c75e205c9e320526918a31ea19c1dfb2ec
)
vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    NO_REMOVE_ONE_LEVEL
)
# Install both architectures: the x64 object links into nvgt and the stubs, and the x86 object links into the 32 bit SAPI host helper which is built as part of the x64 windows build.
file(INSTALL "${SOURCE_PATH}/objs/x64/YY_Thunks_for_Win7.obj" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(INSTALL "${SOURCE_PATH}/objs/x86/YY_Thunks_for_Win7.obj" DESTINATION "${CURRENT_PACKAGES_DIR}/lib" RENAME "YY_Thunks_for_Win7_x86.obj")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
