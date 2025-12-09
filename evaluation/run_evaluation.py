from configuration import *

if method == 0 or method == 1:
    # Get Ideal Batch size on CPU
    print("Get Ideal Batch size on CPU")
    os.makedirs(data_cpu_ideal_batch_size_dir, exist_ok=True)
    for dataset in datasets:
        for batch_size in batch_sizes:
            for percentage in percentages:
                run_command(
                    f"{executable} -i {dataset} -d TBB -t {max_number_of_threads} {algorithms[-1]} -b {batch_size} -p {percentage} -n {iterations}",
                    f"{data_cpu_ideal_batch_size_dir}/{get_dataset_name(dataset)}_batch_{batch_size}_per_{percentage}.yaml")

if method == 0 or method == 2:
    # Get Ideal Batch size on GPU
    print("Get Ideal Batch size on GPU")
    os.makedirs(data_gpu_ideal_batch_size_dir, exist_ok=True)
    for dataset in datasets:
        for batch_size in batch_sizes:
            for percentage in percentages:
                run_command(
                    f"{executable} -i {dataset} -d KOKKOS -t {max_number_of_threads} {algorithms[-1]} -b {batch_size} -p {percentage} -n {iterations}",
                    f"{data_gpu_ideal_batch_size_dir}/{get_dataset_name(dataset)}_batch_{batch_size}_per_{percentage}.yaml")

if method == 0 or method == 3:
    # CPU time information
    print("Get CPU time information")
    os.makedirs(data_cpu_time_dir, exist_ok=True)
    for dataset in datasets:
        for percentage in percentages:
            run_command(
                f"{executable} -i {dataset} -d TBB -t 1 {algorithms_joined} -p {percentage} -b {cpu_ideal_batch_size} -n {iterations}",
                f"{data_cpu_time_dir}/{get_dataset_name(dataset)}_per_{percentage}_1_threads.yaml")
            run_command(
                f"{executable} -i {dataset} -d TBB -t {max_number_of_threads} {parallel_algorithms_joined} -p {percentage} -b {cpu_ideal_batch_size} -n {iterations}",
                f"{data_cpu_time_dir}/{get_dataset_name(dataset)}_per_{percentage}_{max_number_of_threads}_threads.yaml")

if method == 0 or method == 4:
    # Speed-up information
    print("Get speed-up information for parallel algorithms")
    os.makedirs(data_speed_up_dir, exist_ok=True)
    for dataset in speed_up_datasets:
        for power in range(0, max_number_of_threads_power_of_2 + 1):
            threads = int(math.pow(2, power))
            run_command(
                f"{executable} -i {dataset} -d TBB -t {threads} {parallel_algorithms_joined} -b {cpu_ideal_batch_size} -p {largest_percentage} -n {iterations}",
                f"{data_speed_up_dir}/{get_dataset_name(dataset)}_per_{largest_percentage}_{threads}_threads.yaml")

if method == 0 or method == 5:
    # GPU time information
    print("Get GPU time information")
    os.makedirs(data_gpu_time_dir, exist_ok=True)
    # Run the evaluation for each algorithm with each hash function, because memory allocation issues can arise
    for dataset in datasets:
        for algo in viskores_algorithms:
            for percentage in percentages:
                run_command(
                    f"{executable} -i {dataset} -d KOKKOS -t {max_number_of_threads} {algo} -b {gpu_ideal_batch_size} -p {percentage} -n {iterations}",
                    f"{data_gpu_time_dir}/{get_dataset_name(dataset)}_per_{percentage}_{algorithms_names[algo]}.yaml")

if method == 0 or method == 6:
    # Memory footprint information
    print("Get memory footprint information")
    os.makedirs(data_memory_footprint_dir, exist_ok=True)
    for dataset in datasets:
        for percentage in percentages:
            for algo in non_batch_algorithms:
                run_command(
                    f"{memory_evaluator} {executable} -i {dataset} -d TBB -t 1 {algo} -b {cpu_ideal_batch_size} -p {percentage} -n 0",
                    f"{data_memory_footprint_dir}/{get_dataset_name(dataset)}_per_{percentage}_{algorithms_names[algo]}.txt")
            for algo in batch_algorithms:
                for batch_size in ideal_batch_sizes:
                    run_command(
                        f"{memory_evaluator} {executable} -i {dataset} -d TBB -t 1 {algo} -b {batch_size} -p {percentage} -n 0",
                        f"{data_memory_footprint_dir}/{get_dataset_name(dataset)}_per_{percentage}_{algorithms_names[algo]}_batch_{batch_size}.txt")

print(f"All evaluations completed. Check results in {results_dir}/.")
