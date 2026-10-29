cmake_minimum_required (VERSION 3.20)

# ArchiCAD_LookUp solution: read-only inspector for Archicad data (like RevitLookup), two projects:
#   ArchiCAD_LookUp  native add-on (.apx): menu + ACAPI providers. No Qt (the DevKit needs /Zc:wchar_t-).
#   LookUp_Qt        Qt Widgets window (Fluent-style QSS) in its own DLL (normal /Zc:wchar_t), loaded by the add-on.
# They only talk through the C interface in ArchiCAD_LookUp/Shared/LookUpUi.h.
#
# Visual Studio solution (v142): cmake --preset ac27-vs  ->  ArchiCAD_LookUp.slnx at this folder's root.
# Ninja (portable MSVC): cmake --preset ac27 ; cmake --build --preset release-ac27
# Qt 6 (msvc2019_64): set QT_ROOT in CMakeUserPresets.json (e.g. C:/Qt/6.5.3/msvc2019_64); without it LookUp_Qt is skipped.
# Output: out/Release-AC27/ArchiCAD_LookUp_AC27.apx  and  out/Release-AC27/LookUp_Qt/LookUp_Qt.dll (+ Qt DLLs)

set (AC_ADDON_NAME     "ArchiCAD_LookUp" CACHE STRING   "Add-On name.")
set (AC_ADDON_LANGUAGE "INT"             CACHE STRING   "Add-On language code.")
set (Python_EXE        "python"          CACHE FILEPATH "Python used by CompileResources.py")
set (QT_ROOT           ""                CACHE PATH     "Qt 6 kit folder (msvc2019_64), contains lib/cmake/Qt6")

include (${CMAKE_CURRENT_LIST_DIR}/cmake/AcCommon.cmake)
project (ArchiCAD_LookUp CXX)

add_subdirectory (ArchiCAD_LookUp)
add_subdirectory (LookUp_Qt)
ac_write_root_solution (ArchiCAD_LookUp LookUp_Qt)
