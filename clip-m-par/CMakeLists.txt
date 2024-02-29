cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clip-m-par)

set(CMAKE_CXX_STANDARD 17)

set(FETCHCONTENT_QUIET OFF)

include(FetchContent)

FetchContent_Declare(
  VTK
  GIT_REPOSITORY https://gitlab.kitware.com/vtk/vtk.git
  GIT_TAG a64b3715143bd0248aa58bfc128f8435dcee08a0
  GIT_PROGRESS ON
  SOURCE_DIR VTK
)
FetchContent_MakeAvailable(VTK)

add_executable(clip-m-par main.cxx)
target_link_libraries(clip-m-par PUBLIC VTK::CommonCore VTK::IOXML VTK::AcceleratorsVTKmFilters)

# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS clip-m-par
  MODULES VTK::CommonCore VTK::AcceleratorsVTKmFilters
)
