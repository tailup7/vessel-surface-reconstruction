cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(TubeFromCenterline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(VTK COMPONENTS 
  CommonColor
  CommonCore
  CommonDataModel
  FiltersCore
  FiltersModeling
  IOGeometry
  InteractionStyle
  RenderingContextOpenGL2
  RenderingCore
  RenderingFreeType
  RenderingGL2PSOpenGL2
  RenderingOpenGL2
)

if (NOT VTK_FOUND)
  message(FATAL_ERROR "TubeFromCenterline: Unable to find the VTK build folder.")
endif()

# Prevent a "command line is too long" failure in Windows.
set(CMAKE_NINJA_FORCE_RESPONSE_FILE "ON" CACHE BOOL "Force Ninja to use response files.")

add_executable(TubeFromCenterline MACOSX_BUNDLE TubeFromCenterline.cpp)
target_link_libraries(TubeFromCenterline PRIVATE ${VTK_LIBRARIES})

# Windows のファイルダイアログ用ライブラリ
if (WIN32)
  target_link_libraries(TubeFromCenterline PRIVATE comdlg32)
endif()

# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS TubeFromCenterline
  MODULES ${VTK_LIBRARIES}
)
