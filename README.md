# vtk-clip-evaluation

## Introduction

Repository for the evaluation of the external facelist calculation algorithms in VTK/Viskores.

The algorithms that are evaluated are the following:

1. VTK's S-Clip found in src/vtkTableBasedClipDataSetSClip
2. VTK's P-Batch-Clip found in src/vtkTableBasedClipDataSetPBatchClip
3. Viskores' DP-Clip found in src/ClipDPClip
4. Viskores' DP-Batch-Clip found in src/ClipDPBatchClip

## Compilation

To compile the executable on the frontier supercomputer, you can use the script `compile_frontier.sh`.
If you are compiling locally, you can get inspiration from the `compile_frontier.sh` script, and remove or change
what you do or do not need depending on your system.

## Executable

These algorithms can be used through the compiled executable named `vtk-clip-evaluation`, with the
following options:

```
./vtk-clip-evaluation -h
Clip Evaluation
Usage: ./vtk-clip-evaluation [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -i,--input TEXT REQUIRED    Input file name
  -d,--device TEXT            Device name. Available: "Any" "Serial" "TBB" . (Default: TBB).
  -t,--threads UINT:UINT in [1 - 16]
                              Number of threads (Default: 1)
  -p,--percentage FLOAT:FLOAT in [0 - 1]
                              Percentage
  -b,--batch-size UINT        Batch size (Default: 1000)
  -n,--trials UINT            Number of trials (Default: 1)
  --s-clip                    Run the S-Clip algorithm
  --p-batch-clip              Run the P-Batch-Clip algorithm
  --dp-clip                   Run the DP-Clip algorithm
  --dp-batch-clip             Run the DP-Batch-Clip algorith
```

## Python Evaluation scripts

The ```vtk-clip-evaluation``` executable can be used to evaluate the algorithms.
In the `evaluation` directory, you can find the following scripts:

1. `configuration.py` is used to define information regarding where data, executable(s) and
   results should be located. Be sure to check it out.
2. `run_evaluation.py` is used to run the ```vtk-clip-evaluation``` executable. It will run the
   executable with the specified options and store the different kinds of results. For different segments of the
   evaluation, you can use the ``--method`` option to part of the evaluation you want to run.
3. `generate_figures.py` is used to generate the figures based on the results obtained from the evaluation. For
   different segments of the evaluation, you can use the ``--method`` option to generate figures for a specific part of
   the evaluation.

## Data

The datasets used for the evaluations of the algorithms can be downloaded from the following
[Google Drive folder](https://drive.google.com/drive/folders/1RfmTb2kLGVUuX2pHDQN9lg2eYAszVdBs).

Be sure to update the `configuration.py` file with the correct path to the data.

## Results

The evaluation data is stored in different folders in the `evaluation/results` directory. The results that are generated
are:

1. cpu-ideal-batch-size: The ideal batch size for the CPU as a whole and per step of the algorithm.
2. gpu-ideal-batch-size: The ideal batch size for the GPU as a whole and per step of the algorithm.
3. cpu-time: The CPU time of all algorithms with 1 thread and of all parallel algorithms using max number of threads.
4. speed-up: The speed-up of the parallel algorithms using max number of threads.
5. gpu-time: The GPU time of the Viskores algorithms.
6. memory-footprint: The memory footprint of the algorithms.
