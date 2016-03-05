project(kde-thumbnailer-chm)
cmake_minimum_required(VERSION 2.8.12)
set(QT_MIN_VERSION "5.2.0")

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Gui)
find_package(KF5 REQUIRED COMPONENTS KIO)

find_path (CHM_INCLUDE_DIR NAMES chm_lib.h)
find_library (CHM_LIBRARY NAMES chm libchm REQUIRED)

set(chmthumbnail_SRCS chmthumbnail.cpp)

add_library(chmthumbnail MODULE ${chmthumbnail_SRCS})
target_link_libraries(chmthumbnail Qt5::Gui KF5::KIOWidgets ${CHM_LIBRARY})

install(TARGETS chmthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES chmthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
