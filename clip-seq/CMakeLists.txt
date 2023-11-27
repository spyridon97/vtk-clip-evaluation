cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clip-seq)

set(CMAKE_CXX_STANDARD 14)

set(FETCHCONTENT_QUIET OFF)

include(FetchContent)

FetchContent_Declare(
  VTK
  GIT_REPOSITORY https://gitlab.kitware.com/vtk/vtk.git
  GIT_TAG 0bdef360b84d08415f6f31db54593e1dca172e11
  GIT_PROGRESS ON
  SOURCE_DIR VTK
)
FetchContent_MakeAvailable(VTK)

add_executable(clip-seq main.cxx)
target_link_libraries(clip-seq PUBLIC VTK::CommonCore VTK::IOXML VTK::FiltersGeneral)

# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS clip-seq
  MODULES VTK::CommonCore VTK::IOXML VTK::FiltersGeneral
)
