cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(vtk-clip-evaluation LANGUAGES C CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Ensure the standard is enforced

if (DEFINED CMAKE_CUDA_ARCHITECTURES AND NOT CMAKE_CUDA_ARCHITECTURES STREQUAL "")
  # Enable CUDA language
  enable_language(CUDA)
  message(STATUS "CUDA language enabled.")
endif ()
if (DEFINED CMAKE_HIP_ARCHITECTURES AND NOT CMAKE_HIP_ARCHITECTURES STREQUAL "")
  # Enable HIP language
  enable_language(HIP)
  message(STATUS "HIP language enabled.")
endif ()

# VTK flags
set(VTK_SMP_IMPLEMENTATION_TYPE TBB CACHE STRING "" FORCE)
#set(VTK_USE_KOKKOS ON CACHE BOOL "" FORCE)
#set(VTK_KOKKOS_BACKEND HIP CACHE STRING "" FORCE)
set(VTK_MODULE_ENABLE_VTK_vtkvtkm YES CACHE BOOL "" FORCE)
set(VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmCore YES CACHE BOOL "" FORCE)
set(VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel YES CACHE BOOL "" FORCE)
set(VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters YES CACHE BOOL "" FORCE)

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

FetchContent_Declare(
  VTK
  GIT_REPOSITORY https://gitlab.kitware.com/vtk/vtk.git
  GIT_TAG 5bf50f45079447ddbc598b43c9a4697513d85846 # Merge topic 'clang-tidy-x3d'
  GIT_PROGRESS ON
  SOURCE_DIR VTK
)
FetchContent_MakeAvailable(VTK)

# After VTK is fetched, manually update submodules
set(VTKm_SOURCE_DIR ${VTK_SOURCE_DIR}/ThirdParty/vtkm/vtkvtkm/vtk-m)
execute_process(
  COMMAND git checkout 4be31777cd5e47a9f417075914487ffaf34db0ef # Merge topic 'add-array-set-values'
  WORKING_DIRECTORY ${VTKm_SOURCE_DIR}
  RESULT_VARIABLE GIT_CHECKOUT_RESULT
  ERROR_VARIABLE GIT_CHECKOUT_ERROR
)
message(STATUS "VTK-m checkout: ${GIT_CHECKOUT_ERROR}")

FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.4.2
  GIT_PROGRESS ON
  SOURCE_DIR CLI11
)
FetchContent_MakeAvailable(CLI11)

set(headers
  src/ClipTablesDPBatchClip.h
  src/ClipDPBatchClip.h
  src/ClipDPClip.h
  src/vtkTableBasedClipDataSetPBatchClip.h
  src/vtkTableBasedClipDataSetSClip.h

  src/YamlWriter.h
  src/Arguments.h
)

set(sources
  src/vtkTableBasedClipDataSetPBatchClip.cxx
  src/vtkTableBasedClipCasesSClip.cxx
  src/vtkTableBasedClipDataSetSClip.cxx

  src/Arguments.cxx
)

set(device_sources
  src/ClipEvaluation.cxx)

add_executable(${PROJECT_NAME} ${headers} ${sources} ${device_sources})

target_include_directories(${PROJECT_NAME} PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC
  VTK::CommonCore VTK::IOXML VTK::FiltersGeneral VTK::vtkvtkm VTK::AcceleratorsVTKmDataModel CLI11::CLI11)
vtkm_add_target_information(${PROJECT_NAME}
  DROP_UNUSED_SYMBOLS
  MODIFY_CUDA_FLAGS
  DEVICE_SOURCES ${device_sources}
)

set_source_files_properties(${headers}
  PROPERTIES HEADER_FILE_ONLY TRUE
)
