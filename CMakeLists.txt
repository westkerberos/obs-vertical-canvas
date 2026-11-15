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

# TikTok's native LIVE integration (authentication, room lifecycle and signed packet metadata).
find_package(qrcodegencpp CONFIG QUIET)
if(NOT TARGET qrcodegencpp::qrcodegencpp)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(QRCODEGENCPP REQUIRED IMPORTED_TARGET qrcodegencpp)
  add_library(qrcodegencpp::qrcodegencpp ALIAS PkgConfig::QRCODEGENCPP)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE qrcodegencpp::qrcodegencpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

if(OS_WINDOWS)
  set(TIKTOK_CRYPTO_LIBRARY bcrypt)
elseif(NOT APPLE)
  find_package(OpenSSL REQUIRED COMPONENTS Crypto)
  set(TIKTOK_CRYPTO_LIBRARY OpenSSL::Crypto)
endif()
if(TIKTOK_CRYPTO_LIBRARY)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${TIKTOK_CRYPTO_LIBRARY})
endif()

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
	multi-canvas-source.h
	# TikTok integration lives inside this single Vertical Canvas module.
	src/tiktok-live-session.cpp
	src/tiktok-live-session.h
	src/tiktok-output-session.cpp
	src/tiktok-output-session.h
	src/tiktok-output-settings-dialog.cpp
	src/tiktok-output-settings-dialog.h
	src/streamlabs-api.cpp
	src/streamlabs-api.h
	src/streamlabs-info.cpp
	src/streamlabs-info.h
	src/streamlabs-desktop.cpp
	src/streamlabs-desktop.h)

set(TIKTOK_NATIVE_SOURCES
  src/tiktok-native/domain_routing.cpp
  src/tiktok-native/frame_signing.cpp
  src/tiktok-native/live_client.cpp
  src/tiktok-native/mobile_device.cpp
  src/tiktok-native/mobile_profile.cpp
  src/tiktok-native/rapidapi_quota.cpp
  src/tiktok-native/sei_metadata.cpp
  src/tiktok-native/tiktok_request_signer.cpp
  src/tiktok-native/tiktok_studio_device.cpp
  src/tiktok-native/tiktok_studio_game_tags.cpp
  src/tiktok-native/tiktok_studio_qr.cpp
  src/tiktok-native/tiktok_studio_session.cpp
  src/tiktok-native/tiktok_studio_topics.cpp
  src/tiktok-native/localization.cpp)
target_sources(${PROJECT_NAME} PRIVATE src/tiktok-live-settings.h ${TIKTOK_NATIVE_SOURCES})

if(BUILD_OUT_OF_TREE)
	set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
else()
	set_target_properties_obs(${PROJECT_NAME} PROPERTIES FOLDER "plugins/aitum" PREFIX "")
endif()
