# Use the official debian:trixie-slim image as a parent image
FROM debian:trixie-slim

# Update package lists and upgrade packages
RUN apt-get update && \
    apt-get upgrade -y

# Install necessary dependencies
RUN apt-get install -y \
    build-essential \
    wget \
    unzip \
    time \
    libboost-all-dev \
    libtbb-dev \
    git \
    cmake \
    ninja-build \
    libgl1-mesa-dev

# Set working directory to the mount so that the user can simly specify files
# without caring abou the actual container paths and to avoid permission issues
ARG data_mount_dir=/data

# Set the working directory
WORKDIR /usr/src/

# git clone the repo
RUN git clone https://github.com/spyridon97/vtk-clip-evaluation.git

# Build the project clip-seq
WORKDIR /usr/src/vtk-clip-evaluation/clip-seq
RUN mkdir build &&  \
    cd build &&  \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB .. &&  \
    cmake --build . --target clip-seq -j && \
    cp clip-seq /usr/local/bin/clip-seq

# Build the project clip-m-par
WORKDIR /usr/src/vtk-clip-evaluation/clip-m-par
RUN mkdir build && \
    cd build && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters=YES .. && \
    cd VTK && \
    git apply ../../deactivate-conversion-fromvtk.patch && \
    cd .. && \
    cmake --build . --target clip-m-par -j && \
    cp clip-m-par /usr/local/bin/clip-m-par

# Build the project clip-par
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN mkdir build && \
    cd build && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB .. && \
    cmake --build . --target clip-par -j && \
    cp clip-par /usr/local/bin/clip-par

# Build the project clip-par-pass-time
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN mkdir build-patch-time && \
    cd build-patch-time && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB .. && \
    cd VTK && \
    git apply ../../pass-time.patch && \
    cd .. && \
    cmake --build . --target clip-par -j && \
    mv clip-par clip-par-pass-time && \
    cp clip-par-pass-time /usr/local/bin/clip-par-pass-time

# Build the project clip-par-test-various-batchSizes-trimming-yes
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN git apply test-various-batchSizes.patch && \
    mkdir build-test-various-batchSizes-trimming-yes && \
    cd build-test-various-batchSizes-trimming-yes && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB .. && \
    cmake --build . --target clip-par -j && \
    mv clip-par clip-par-test-various-batchSizes-trimming-yes && \
    cp clip-par-test-various-batchSizes-trimming-yes /usr/local/bin/clip-par-test-various-batchSizes-trimming-yes

# Build the project clip-par-test-various-batchSizes-trimming-no
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN mkdir build-test-various-batchSizes-trimming-no && \
    cd build-test-various-batchSizes-trimming-no && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB .. && \
    cd VTK && \
    git apply ../../remove-trimming.patch && \
    cd .. && \
    cmake --build . --target clip-par -j && \
    mv clip-par clip-par-test-various-batchSizes-trimming-no && \
    cp clip-par-test-various-batchSizes-trimming-no /usr/local/bin/clip-par-test-various-batchSizes-trimming-no

WORKDIR ${data_mount_dir}

# Set the default command to run when starting the container
CMD ["bash"]
