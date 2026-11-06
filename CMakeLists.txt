# Built as a core module inside CTD OBS (plugins/ctd-vertical-canvas in the ctd-obs repo), using OBS's own targets,
# compiler settings and install rules. The module keeps the name 'vertical-canvas'. No version check against Aitum's
# server: the module ships and updates with CTD OBS. Everything below this block is the upstream build.
if(CMAKE_PROJECT_NAME STREQUAL "obs-studio")
  find_package(Qt6 REQUIRED Core Widgets)
  find_package(CURL REQUIRED)

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" _vc_buildspec)
  string(JSON _vc_version GET "${_vc_buildspec}" version)
  # version.h.in reads these; they are set for this folder only
  set(PROJECT_VERSION "${_vc_version}")
  string(REPLACE "." ";" _vc_version_parts "${_vc_version}")
  list(GET _vc_version_parts 0 PROJECT_VERSION_MAJOR)
  list(GET _vc_version_parts 1 PROJECT_VERSION_MINOR)
  list(GET _vc_version_parts 2 PROJECT_VERSION_PATCH)
  configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h")

  add_library(vertical-canvas MODULE)
  add_library(OBS::vertical-canvas ALIAS vertical-canvas)
  target_sources(
    vertical-canvas
    PRIVATE
      audio-wrapper-source.c
      audio-wrapper-source.h
      config-dialog.cpp
      config-dialog.hpp
      display-helpers.hpp
      file-updater.c
      file-updater.h
      hotkey-edit.cpp
      hotkey-edit.hpp
      multi-canvas-source.c
      multi-canvas-source.h
      name-dialog.cpp
      name-dialog.hpp
      obs-websocket-api.h
      projector.cpp
      projector.hpp
      qt-display.cpp
      qt-display.hpp
      resources.qrc
      scenes-dock.cpp
      scenes-dock.hpp
      source-tree.cpp
      source-tree.hpp
      sources-dock.cpp
      sources-dock.hpp
      transitions-dock.cpp
      transitions-dock.hpp
      vertical-canvas.cpp
      vertical-canvas.hpp
  )
  # The generated version.h comes first, ahead of any stale one left in the source folder by an upstream build
  target_include_directories(
    vertical-canvas
    BEFORE
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated" "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  target_compile_definitions(vertical-canvas PRIVATE VERTICAL_CANVAS_NO_UPDATE_CHECK)
  target_link_libraries(vertical-canvas PRIVATE OBS::frontend-api OBS::libobs Qt::Core Qt::Widgets CURL::libcurl)
  if((OS_LINUX OR OS_FREEBSD OR OS_OPENBSD) AND Qt6_VERSION VERSION_LESS "6.9.0")
    find_package(Qt6 REQUIRED GuiPrivate)
    target_link_libraries(vertical-canvas PRIVATE Qt::GuiPrivate)
  endif()
  set_target_properties_obs(
    vertical-canvas
    PROPERTIES FOLDER plugins/ctd-vertical-canvas PREFIX "" AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
  return()
endif()

# --- Detect if the plugin is build out of tree or not ---
if(CMAKE_PROJECT_NAME STREQUAL "obs-studio")
  set(BUILD_OUT_OF_TREE OFF)
else()
  set(BUILD_OUT_OF_TREE ON)
  cmake_minimum_required(VERSION 3.16...3.26)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${PROJECT_NAME} MODULE)

target_link_libraries(${PROJECT_NAME} PRIVATE OBS::libobs)

if(BUILD_OUT_OF_TREE)
  find_package(libobs REQUIRED)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE OBS::frontend-api)
endif()

find_package(Qt6 COMPONENTS Widgets Core)
if(BUILD_OUT_OF_TREE)
  if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
    find_package(Qt6 REQUIRED Gui)
  endif()
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Core Qt::Widgets)

if((OS_LINUX OR OS_FREEBSD OR OS_OPENBSD) AND Qt6_VERSION VERSION_LESS "6.9.0")
  find_package(Qt6 COMPONENTS GuiPrivate)
  target_link_libraries(${PROJECT_NAME} PRIVATE Qt::GuiPrivate)
endif()

target_compile_options(
${PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
                                -Wno-comma>)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set_target_properties(
${PROJECT_NAME}
PROPERTIES AUTOMOC ON
            AUTOUIC ON
            AUTORCC ON)

if(BUILD_OUT_OF_TREE)
  find_package(CURL REQUIRED)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)

if(OS_WINDOWS)
  configure_file(cmake/windows/resources/installer-Windows.iss.in "${CMAKE_CURRENT_BINARY_DIR}/installer-Windows.generated.iss")
endif()

target_sources(${PROJECT_NAME} PRIVATE
	vertical-canvas.cpp
	scenes-dock.cpp
	sources-dock.cpp
	source-tree.cpp
	transitions-dock.cpp
	qt-display.cpp
	projector.cpp
	config-dialog.cpp
	hotkey-edit.cpp
	name-dialog.cpp
	audio-wrapper-source.c
	file-updater.c
	multi-canvas-source.c
	resources.qrc
	vertical-canvas.hpp
	scenes-dock.hpp
	sources-dock.hpp
	source-tree.hpp
	transitions-dock.hpp
	qt-display.hpp
	projector.hpp
	display-helpers.hpp
	config-dialog.hpp
	hotkey-edit.hpp
	name-dialog.hpp
	audio-wrapper-source.h
	obs-websocket-api.h
	file-updater.h
	multi-canvas-source.h)

if(BUILD_OUT_OF_TREE)
	set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
else()
	set_target_properties_obs(${PROJECT_NAME} PROPERTIES FOLDER "plugins/aitum" PREFIX "")
endif()
