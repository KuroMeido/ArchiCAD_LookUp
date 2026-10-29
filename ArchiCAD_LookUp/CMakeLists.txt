# ArchiCAD_LookUp project: App (entry points), Panels (menu), Lib/Lookup (ACAPI -> tree), Shared (C interface to the Qt DLL).

file (GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.h *.hpp *.cpp)
file (GLOB RESOURCES Resources/RINT/*.grc Resources/RFIX/*.grc Resources/RFIX.win/*.rc2)
source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${RESOURCES})

add_library (ArchiCAD_LookUp SHARED ${SOURCES} ${RESOURCES})
set_source_files_properties (${RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_target_properties (ArchiCAD_LookUp PROPERTIES SUFFIX ".apx")
target_include_directories (ArchiCAD_LookUp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} Shared)

# entry points Archicad looks up by ordinal (they come from ACAP_STAT.lib)
target_link_options (ArchiCAD_LookUp PUBLIC
	/export:GetExportedFuncAddrs,@1
	/export:SetImportedFuncAddrs,@2)

ac_apply_versions (ArchiCAD_LookUp ${AC_ADDON_NAME})

# resources + RES linking, with the DevKit of the active configuration
set (ResourceObjectsDir ${CMAKE_CURRENT_BINARY_DIR}/ResourceObjects)
set (AC_KIT_LIST "")
foreach (ver ${AC_VERSIONS})
	list (APPEND AC_KIT_LIST "${ver}=${AC_DEVKIT_${ver}}")
	set (isAC "$<OR:$<STREQUAL:$<CONFIG>,Debug-AC${ver}>,$<STREQUAL:$<CONFIG>,Release-AC${ver}>>")
	target_link_options (ArchiCAD_LookUp PUBLIC "$<${isAC}:${ResourceObjectsDir}/AC${ver}/${AC_ADDON_NAME}.res>")
endforeach ()
add_custom_command (TARGET ArchiCAD_LookUp PRE_BUILD
	COMMAND ${CMAKE_COMMAND} "-DCFG=$<CONFIG>" "-DKITS=${AC_KIT_LIST}" "-DPYTHON=${Python_EXE}"
		"-DLANG=${AC_ADDON_LANGUAGE}" "-DSRC=${CMAKE_CURRENT_SOURCE_DIR}" "-DRES=${CMAKE_CURRENT_SOURCE_DIR}/Resources"
		"-DOUT=${ResourceObjectsDir}" "-DNAME=${AC_ADDON_NAME}" -P "${CMAKE_SOURCE_DIR}/cmake/CompileRes.cmake"
	COMMENT "Compiling resources with the DevKit of the active configuration"
	VERBATIM)
