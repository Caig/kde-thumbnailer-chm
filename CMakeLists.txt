project(kde-thumbnailer-chm)

cmake_minimum_required(VERSION 2.6)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_path (CHM_INCLUDE_DIR NAMES chm_lib.h)
find_library (CHM_LIBRARY NAMES chm libchm REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${QT_INCLUDES} ${KDE4_INCLUDES} ${CHM_INCLUDE_DIR})

set(chmthumbnail_SRCS chmthumbnail.cpp)

kde4_add_plugin(chmthumbnail ${chmthumbnail_SRCS})
target_link_libraries(chmthumbnail ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${KDE4_KIO_LIBS} ${CHM_LIBRARY})

install(TARGETS chmthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES chmthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})