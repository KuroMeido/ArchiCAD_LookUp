# LookUp_Qt project: Qt Widgets window with Fluent-style QSS. Plain C++17 + Qt only, no Archicad headers
# (Qt needs the native wchar_t, the DevKit needs /Zc:wchar_t-: the two cannot share a binary).

if (NOT QT_ROOT OR NOT EXISTS "${QT_ROOT}/lib/cmake/Qt6/Qt6Config.cmake")
	message (WARNING "LookUp_Qt skipped: set QT_ROOT to a Qt 6 msvc2019_64 kit (folder with lib/cmake/Qt6).")
	return ()
endif ()

list (PREPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
find_package (Qt6 REQUIRED COMPONENTS Widgets)

file (GLOB SOURCES CONFIGURE_DEPENDS *.h *.hpp *.cpp)
add_library (LookUp_Qt SHARED ${SOURCES})
target_include_directories (LookUp_Qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/ArchiCAD_LookUp/Shared)
target_compile_features (LookUp_Qt PRIVATE cxx_std_17)
target_compile_definitions (LookUp_Qt PRIVATE LOOKUPUI_EXPORTS UNICODE _UNICODE)
target_compile_options (LookUp_Qt PRIVATE /W3 /utf-8)
target_link_libraries (LookUp_Qt PRIVATE Qt6::Widgets)
set_target_properties (LookUp_Qt PROPERTIES
	VS_PLATFORM_TOOLSET v142
	MAP_IMPORTED_CONFIG_DEBUG-AC27 RELEASE
	MAP_IMPORTED_CONFIG_RELEASE-AC27 RELEASE)
set (CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

foreach (ver ${AC_VERSIONS})
	foreach (cfg Debug-AC${ver} Release-AC${ver})
		string (TOUPPER "${cfg}" CFG_UPPER)
		set_target_properties (LookUp_Qt PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} "${AC_OUT_DIR}/${cfg}/LookUp_Qt")
	endforeach ()
endforeach ()

# copy the Qt runtime next to the DLL (platform plugin, styles) so LoadLibrary finds it
find_program (WINDEPLOYQT windeployqt PATHS "${QT_ROOT}/bin" NO_DEFAULT_PATH)
if (WINDEPLOYQT)
	add_custom_command (TARGET LookUp_Qt POST_BUILD
		COMMAND "${WINDEPLOYQT}" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "$<TARGET_FILE:LookUp_Qt>"
		COMMENT "windeployqt"
		VERBATIM)
endif ()
