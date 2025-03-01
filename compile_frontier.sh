#!/bin/bash

#set -x
set -e

base_dir=`pwd`

module load git
module load ninja
module load cmake
module load gcc-native/13.2
module load rocm/6.3.1 # Use it with Kokkos 4.5.01

#cmake_generator="Unix Makefiles"
cmake_generator="Ninja"

export CRAYPE_LINK_TYPE=dynamic

kokkos_repo=https://github.com/kokkos/kokkos.git
kokkos_branch=4.5.01

kokkos_src_dir=${base_dir}/kokkos/src
kokkos_build_dir=${base_dir}/kokkos/build
kokkos_install_dir=${base_dir}/kokkos/install

if true; then
rm -rf "${kokkos_src_dir}"
rm -rf "${kokkos_build_dir}"
rm -rf "${kokkos_install_dir}"
if [[ ! -d ${kokkos_src_dir} ]] ; then
  git clone -b ${kokkos_branch} ${kokkos_repo} "${kokkos_src_dir}"
fi
cmake -G "${cmake_generator}" -S "${kokkos_src_dir}" -B "${kokkos_build_dir}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_FLAGS="--offload-arch=gfx90a" \
  -DCMAKE_CXX_COMPILER=hipcc \
  -DCMAKE_INSTALL_PREFIX="${kokkos_install_dir}" \
  -DKokkos_ARCH_VEGA90A=ON \
  -DKokkos_ENABLE_HIP=ON \
  -DKokkos_ENABLE_HIP_RELOCATABLE_DEVICE_CODE=OFF \
  -DKokkos_ENABLE_SERIAL=ON

time cmake --build "${kokkos_build_dir}" -j
cmake --install "${kokkos_build_dir}"
fi

tbb_repo=https://github.com/oneapi-src/oneTBB.git
tbb_branch=v2022.0.0

tbb_src_dir=${base_dir}/tbb/src
tbb_build_dir=${base_dir}/tbb/build
tbb_install_dir=${base_dir}/tbb/install

if true; then
rm -rf "${tbb_src_dir}"
rm -rf "${tbb_build_dir}"
rm -rf "${tbb_install_dir}"
if [[ ! -d ${tbb_src_dir} ]] ; then
  git clone -b ${tbb_branch} ${tbb_repo} "${tbb_src_dir}"
fi
cmake -G "${cmake_generator}" -S "${tbb_src_dir}" -B "${tbb_build_dir}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_COMPILER=amdclang++ \
  -DCMAKE_C_COMPILER=amdclang \
  -DCMAKE_INSTALL_PREFIX="${tbb_install_dir}" \
  -DTBB_TEST=OFF

time cmake --build "${tbb_build_dir}" -j
cmake --install "${tbb_build_dir}"
fi

vtkce_src_dir=${base_dir}
vtkce_build_dir=${base_dir}/build
vtkce_install_dir=${base_dir}/install

if true; then
rm -rf "${vtkce_build_dir}"
rm -rf "${vtkce_install_dir}"
cmake -G "${cmake_generator}" -S "${vtkce_src_dir}" -B "${vtkce_build_dir}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_COMPILER=amdclang++ \
  -DCMAKE_C_COMPILER=amdclang \
  -DCMAKE_HIP_ARCHITECTURES="gfx90a" \
  -DCMAKE_PREFIX_PATH="${kokkos_install_dir}" \
  -DCMAKE_INSTALL_PREFIX="${vtkce_install_dir}" \
  -DKokkos_CXX_COMPILER="$(which hipcc)" \
  -DTBB_ROOT="${tbb_install_dir}" \
  -DVTK_USE_KOKKOS=ON \
  -DVTK_KOKKOS_BACKEND=HIP \
  -DVTKm_USE_DOUBLE_PRECISION=ON

time cmake --build "${vtkce_build_dir}" -j
cmake --install "${vtkce_build_dir}"
fi
