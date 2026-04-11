# Filename: PackageProject.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

set(LUGIZMO_DF_INSTALL_CMAKEDIR
        "${CMAKE_INSTALL_LIBDIR}/cmake/${LUGIZMO_DF_PACKAGE_NAME}")

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${LUGIZMO_DF_PACKAGE_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${LUGIZMO_DF_PACKAGE_NAME}Config.cmake"
        INSTALL_DESTINATION "${LUGIZMO_DF_INSTALL_CMAKEDIR}"
)

export(
        EXPORT LuDataFrameTargets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${LUGIZMO_DF_PACKAGE_NAME}Targets.cmake"
        NAMESPACE "${LUGIZMO_DF_NAMESPACE}"
)

install(
        EXPORT LuDataFrameTargets
        FILE "${LUGIZMO_DF_PACKAGE_NAME}Targets.cmake"
        NAMESPACE "${LUGIZMO_DF_NAMESPACE}"
        DESTINATION "${LUGIZMO_DF_INSTALL_CMAKEDIR}"
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${LUGIZMO_DF_PACKAGE_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${LUGIZMO_DF_PACKAGE_NAME}ConfigVersion.cmake"
        DESTINATION "${LUGIZMO_DF_INSTALL_CMAKEDIR}"
)
