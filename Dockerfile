# Use the official GCC 13 base image
FROM gcc:13

# Update package lists and install necessary dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    wget \
    unzip \
    libboost-all-dev \
    libtbb-dev \

# Set the working directory
WORKDIR /usr/src/

# git clone the repo
RUN git clone https://github.com/spyridon97/vtk-clip-evaluation.git

# Build the project clip-seq
WORKDIR /usr/src/vtk-clip-evaluation/clip-seq
RUN mkdir build
RUN cd build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
RUN cmake --build . --target clip-seq -j

# Build the project clip-m-par
WORKDIR /usr/src/vtk-clip-evaluation/clip-m-par
RUN mkdir build
RUN cd build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DVTK_MODULE_ENABLE_VTK_vtkvtkm=YES -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmCore=YES -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel=YES -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters=YES ..
RUN cmake --build . --target clip-m-par -j

# Build the project clip-par
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN mkdir build
RUN cd build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
RUN cmake --build . --target clip-par -j

# Build the project clip-par-pass-time
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN git apply pass-time.patch
RUN mkdir build-patch-time
RUN cd build-patch-time
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
RUN cmake --build . --target clip-par -j
RUN mv clip-par clip-par-pass-time

# Build the project clip-par-configurable-batch-size-trimming-yes
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN git apply configurable-batch-size.patch
RUN mkdir build-configurable-batch-size-trimming-yes
RUN cd build-configurable-batch-size-trimming-yes
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
RUN cmake --build . --target clip-par -j
RUN mv clip-par clip-par-configurable-batch-size-trimming-yes

# Build the project clip-par-configurable-batch-size-trimming-no
WORKDIR /usr/src/vtk-clip-evaluation/clip-par
RUN git apply configurable-batch-size.patch
RUN mkdir build-configurable-batch-size-trimming-no
RUN cd build-configurable-batch-size-trimming-no
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
RUN cd VTK
RUN git apply ../../remove-trimming.patch
RUN cd ..
RUN cmake --build . --target clip-par -j
RUN mv clip-par clip-par-configurable-batch-size-trimming-no

# Set entrypoint for clip-seq, clip-m-par, clip-par, clip-par-pass-time, clip-par-configurable-batch-size-trimming-yes, clip-par-configurable-batch-size-trimming-no
ENTRYPOINT ["/usr/src/vtk-clip-evaluation/clip-seq/build/clip-seq", "/usr/src/vtk-clip-evaluation/clip-m-par/build/clip-m-par", "/usr/src/vtk-clip-evaluation/clip-par/build/clip-par", "/usr/src/vtk-clip-evaluation/clip-par/build-patch-time/clip-par-pass-time", "/usr/src/vtk-clip-evaluation/clip-par/build-configurable-batch-size-trimming-yes/clip-par-configurable-batch-size-trimming-yes", "/usr/src/vtk-clip-evaluation/clip-par/build-configurable-batch-size-trimming-no/clip-par-configurable-batch-size-trimming-no"]
