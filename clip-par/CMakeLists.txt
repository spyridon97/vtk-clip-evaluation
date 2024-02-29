cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clip-par)

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

add_executable(clip-par main.cxx)
target_link_libraries(clip-par PUBLIC VTK::CommonCore VTK::IOXML VTK::FiltersGeneral)

# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS clip-par
  MODULES VTK::CommonCore VTK::IOXML VTK::FiltersGeneral
)
