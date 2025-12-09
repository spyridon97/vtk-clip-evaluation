import argparse, math, os, subprocess, shutil

# Create an argument parser
parser = argparse.ArgumentParser(description="Run evaluation for the clip algorithm")

# Add the 'method' argument
parser.add_argument('--method', type=int, default=0,
                    help="Evaluation method: 0 - all, 1 - Ideal Batch size on CPU, 2 - Ideal Batch size on GPU, 3 - CPU time, 4 - speed-up, 5 - GPU time, 6 - memory footprint"
                         "Default: 0")
parser.add_argument("--iterations", type=int, default=10, help="Number of iterations for each evaluation. Default: 10")

# Parse the command-line arguments
args = parser.parse_args()

# Access the 'method' argument
method = args.method

# Iterations
iterations = args.iterations

# Directories
evaluation_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = os.path.dirname(evaluation_dir)
build_dir = os.path.join(src_dir, "build")  # Make sure this is the correct build directory
home_dir = os.path.expanduser("~")
datasets_dir = os.path.join(home_dir, "Data")  # Make sure this is the correct dataset directory

results_dir = os.path.join(evaluation_dir, "results")
# Make sure this is the correct configuration
# configuration = "testing"
configuration = f"frontier_rocm6.3.1_tbb2022.0.0_kokkos4.5.01"
config_dir = os.path.join(results_dir, configuration)

data_dir = os.path.join(config_dir, "data")
data_cpu_ideal_batch_size_dir = os.path.join(data_dir, "cpu_ideal_batch_size")
data_gpu_ideal_batch_size_dir = os.path.join(data_dir, "gpu_ideal_batch_size")
data_cpu_time_dir = os.path.join(data_dir, "cpu_time")
data_speed_up_dir = os.path.join(data_dir, "speed_up")
data_gpu_time_dir = os.path.join(data_dir, "gpu_time")
data_memory_footprint_dir = os.path.join(data_dir, "memory_footprint")

figures_dir = os.path.join(config_dir, "figures")
fig_cpu_ideal_batch_size_dir = os.path.join(figures_dir, "cpu_ideal_batch_size")
fig_gpu_ideal_batch_size_dir = os.path.join(figures_dir, "gpu_ideal_batch_size")
fig_cpu_time_dir = os.path.join(figures_dir, "cpu_time")
fig_speed_up_dir = os.path.join(figures_dir, "speed_up")
fig_gpu_time_dir = os.path.join(figures_dir, "gpu_time")
fig_memory_footprint_dir = os.path.join(figures_dir, "memory_footprint")

# Executables
executable = os.path.join(build_dir, "vtk-clip-evaluation")
time_executable = shutil.which("time")
memory_evaluator = f"{time_executable} -v"

# Datasets # Make sure these are the correct datasets ordered from smallest to biggest
datasets = [f"{datasets_dir}/Torso.vtu",
            f"{datasets_dir}/CX-1.vtu",
            f"{datasets_dir}/JSM.vtu",
            f"{datasets_dir}/F-15.vtu",
            f"{datasets_dir}/JSM-tet.vtu",
            f"{datasets_dir}/F-15-tet.vtu"]
speed_up_datasets = datasets[3:]
biggest_dataset = datasets[-1]

# Algorithms
algorithms_names = {"--s-clip": "S-Clip", "--p-batch-clip": "P-Batch-Clip", "--dp-clip": "DP-Clip",
                    "--dp-batch-clip": "DP-Batch-Clip"}
algorithms = ["--s-clip", "--p-batch-clip", "--dp-clip", "--dp-batch-clip"]
parallel_algorithms = algorithms[1:]
vtk_algorithms = algorithms[:2]
viskores_algorithms = algorithms[2:]
non_batch_algorithms = [algorithms[0], algorithms[2]]
batch_algorithms = [algorithms[1], algorithms[3]]
algorithms_joined = " ".join(algorithms)
parallel_algorithms_joined = " ".join(parallel_algorithms)
viskores_algorithms_joined = " ".join(viskores_algorithms)
batch_algorithms_joined = " ".join(batch_algorithms)

dp_batch_clip_steps = ["seconds-mark-kept-points",
                       "seconds-kept-points-mask-select",
                       "seconds-point-data-scan-exclusive",
                       "seconds-compute-point-maps",
                       "seconds-compute-cell-stats",
                       "seconds-kept-or-clipped-cells-mask-select",
                       "seconds-cell-data-scan-exclusive",
                       "seconds-clipped-cells-mask-select",
                       "seconds-extract-edges",
                       "seconds-duplicate-to-unique-edge-map",
                       "seconds-generate-cell-set"]
# Percentages values
percentages = [0.2, 0.5, 0.8]
largest_percentage = percentages[-1]

# batch sizes
# Define a(n) for n ≥ 0 as follows:
#
# - If n = 0:
#   a(0) = 1
#
# - If n is odd (n ≥ 1):
#   a(n) = 2^((n + 1) / 2)
#   (That is, raise 2 to the power of ((n + 1) divided by 2))
#
# - If n is even and n > 0:
#   a(n) = (3/2) * 2^(n / 2)
#   (Multiply 2 raised to the power of (n/2) by 3/2)
batch_sizes = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048]
cpu_ideal_batch_size = 1024
gpu_ideal_batch_size = 6
ideal_batch_sizes = [cpu_ideal_batch_size, gpu_ideal_batch_size]

# Number of threads
# max_number_of_threads_power_of_2 = int(math.log2(os.cpu_count()))  # Make sure this uses the correct number of threads
max_number_of_threads_power_of_2 = int(math.log2(128))  # Make sure this uses the correct number of threads
max_number_of_threads = int(2 ** max_number_of_threads_power_of_2)


def get_dataset_name(filename):
    """Get the name of the dataset from the filename"""
    return os.path.splitext(os.path.basename(filename))[0]
    # return os.path.basename(filename)


# Function to run subprocess commands
def run_command(command, output_file):
    """Run a command and save the output to a file"""
    with open(output_file, 'a') as f:
        subprocess.run(command, shell=True, stdout=f, stderr=f)
