# VTK Clip Evaluation

Repository to evaluate the performance of clip filters in VTK

## Build Instructions

There are 3 directories in this repository: clip-seq, clip-m-par, and clip-par

### clip-seq

```bash
cd clip-seq
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
cmake --build . --target clip-seq -j
``` 

### clip-m-par

```bash
cd clip-m-par
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters=YES ..
cmake --build . --target clip-m-par -j
```

### clip-par

```bash
cd clip-par
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB ..
cd VTK && \
git apply ../../deactivate-conversion-fromvtk.patch && \
cd .. && \
cmake --build . --target clip-par -j
```

## Datasets

https://drive.google.com/drive/folders/1RfmTb2kLGVUuX2pHDQN9lg2eYAszVdBs

## Execution Instructions

### clip-seq

```bash
/usr/bin/time -vv ./clip-seq/build/clip-seq inputFile percentage numberOfIterations
```

### clip-m-par

```bash
/usr/bin/time -vv ./clip-m-par/build/clip-m-par inputFile percentage numberOfIterations numberOfThreads
```

### clip-par

```bash
/usr/bin/time -vv ./clip-par/build/clip-par inputFile percentage numberOfIterations numberOfThreads
```
