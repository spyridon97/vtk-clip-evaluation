from enum import Enum
import numpy as np
import re, yaml
import pandas as pd
import matplotlib.pyplot as plt
from configuration import *

# Generate figures using the results

# Figure size
fig_width = 8
fig_height = 6.5

legend_fontsize = 11
axis_label_fontsize = 14
title_fontsize = axis_label_fontsize


# Set column width and spacing x-axis positions for datasets
def get_width_and_spacing(num_algorithms):
    max_num_algorithms = len(algorithms)
    ratio = max_num_algorithms / num_algorithms
    width = 0.2 * ratio
    spacing = 1.2
    return width, spacing


def load_gpl_palette(file_path):
    with open(file_path, 'r') as file:
        colors = []
        for line in file:
            # Skip lines that start with '#' or are empty
            if line.startswith('#') or not line.strip():
                continue

            # Split the line into components
            parts = line.split()
            if len(parts) >= 4:
                # Extract RGB values and name (the last part)
                r, g, b = map(float, parts[:3])  # Convert RGB to integers
                r /= 255
                g /= 255
                b /= 255
                # name = ' '.join(parts[3:])  # Join the remaining parts as the color name
                colors.append((r, g, b))  # Store as a tuple (R, G, B, Name)
        return colors


# Load palette
palette = load_gpl_palette(f'{evaluation_dir}/Set2_6.gpl')

# Create a dictionary to map algorithms to colors
algorithm_colors = {}
for i, algo in enumerate(algorithms):
    algorithm_name = algorithms_names[algo]
    algorithm_colors[algorithm_name] = palette[i]
algorithm_colors[f'{algorithms_names["--p-batch-clip"]}_batch_{cpu_ideal_batch_size}'] = algorithm_colors[
    algorithms_names["--p-batch-clip"]]
algorithm_colors[f'{algorithms_names["--p-batch-clip"]}_batch_{gpu_ideal_batch_size}'] = palette[5]
algorithm_colors[f'{algorithms_names["--dp-batch-clip"]}_batch_{cpu_ideal_batch_size}'] = palette[4]
algorithm_colors[f'{algorithms_names["--dp-batch-clip"]}_batch_{gpu_ideal_batch_size}'] = algorithm_colors[
    algorithms_names["--dp-batch-clip"]]


def print_improvement_ratio(df):
    print()
    # find me ratios of all algorithms over the minimum value for each dataset
    for dataset in df.columns:
        min = df[dataset].min()
        min = float(1) if math.isnan(min) else min
        for algo in df.index:
            value = df.loc[algo][dataset]
            value = float(1) if value is None else value
            ratio = value / min
            print(f"Dataset: {dataset}, Algorithm: {algo}, Value/Min Ratio: {ratio:.2f}")
        print()
    algorithm_ratios = {}
    for dataset in df.columns:
        min = df[dataset].min()
        min = float(1) if math.isnan(min) else min
        for algorithm in df.index:
            value = df.loc[algorithm][dataset]
            value = float(1) if value is None else value
            ratio = value / min
            if algorithm not in algorithm_ratios:
                algorithm_ratios[algorithm] = []
            algorithm_ratios[algorithm].append(ratio)
    for algorithm, ratios in algorithm_ratios.items():
        print(f"Algorithm: {algorithm}, Min - Max Ratios: {np.min(ratios):.2f}x - {np.max(ratios):.2f}x")
    print()


class NormalizeType(Enum):
    MIN = 0
    MAX = 1
    AVERAGE = 2
    NONE = 3


def create_box_plot_chart(df, x_label, y_label, figure_filename, normalize_type=NormalizeType.NONE):
    # Create the figure
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))

    # Get the batch sizes (column names)
    batch_sizes = df.columns

    # Normalize the data if requested
    if normalize_type == NormalizeType.MIN:
        denominator = np.nanmin(df.values)
    elif normalize_type == NormalizeType.MAX:
        denominator = np.nanmax(df.values)
    elif normalize_type == NormalizeType.AVERAGE:
        denominator = np.nanmean(df.values)
    else:
        denominator = 1

    # Prepare the data for boxplot
    data_for_boxplot = [df[batch].values / denominator for batch in batch_sizes]

    # Create the boxplot
    bp = ax.boxplot(data_for_boxplot, patch_artist=True)

    # Customize boxplot appearance
    for box in bp['boxes']:
        box.set(facecolor='lightblue', alpha=0.7)

    # Change the mean line color to red
    for mean in bp['means']:
        mean.set(color='red', linewidth=2)

    # Set the x-tick labels to batch sizes
    ax.set_xticklabels(batch_sizes, rotation=45, ha='right')

    # Add labels and title
    ax.set_xlabel(x_label, fontsize=axis_label_fontsize)
    ax.set_ylabel(y_label, fontsize=axis_label_fontsize)
    # ax.set_title('Title', fontsize=title_fontsize)

    # Show grid lines for clarity
    ax.grid(axis='x', linestyle='--')

    plt.tight_layout()
    plt.savefig(figure_filename, dpi=300, bbox_inches='tight')


def create_bar_chart(df, add_min_offset, x_label, y_label, legend_title, legend_loc, figure_filename):
    # Number of datasets and algorithms
    num_datasets = len(df.columns)
    num_algorithms = len(df.index)

    # Create the figure
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))

    width, spacing = get_width_and_spacing(num_algorithms)

    # Set the x-axis positions for datasets
    dataset_positions = np.arange(num_datasets) * spacing

    # Loop through each algorithm and plot its values across datasets
    for j, algorithm in enumerate(df.index):
        # Extract values for the algorithm
        values = df.loc[algorithm].values
        print(f"Algorithm: {algorithm}, Values: {values}")

        # Plot each algorithm's memory footprint as a horizontal bar for each dataset
        ax.barh(y=dataset_positions + ((num_algorithms - 1) - j - (num_algorithms - 1) / 2) * width, width=values,
                height=width, label=algorithm, color=algorithm_colors[algorithm])

    # Loop through each dataset to find and plot its minimum value
    for i, dataset in enumerate(df.columns):
        # Calculate the minimum value for the current dataset
        min_value = df[dataset].min()
        if add_min_offset:
            # add a small offset to the minimum value to avoid overlapping with the vertical line
            min_value *= 0.99

        # Plot a horizontal line at the correct position for each dataset's minimum value
        ax.vlines(x=min_value,
                  ymin=dataset_positions[i] - width * num_algorithms / 2,  # Start of line
                  ymax=dataset_positions[i] + width * num_algorithms / 2,  # End of line
                  color='grey', linestyle='--', linewidth=1)

    # Set the y-ticks to represent datasets
    ax.set_yticks(dataset_positions)
    ax.set_yticklabels(df.columns)

    # Add labels and title
    ax.set_xlabel(x_label, fontsize=axis_label_fontsize)
    ax.set_ylabel(y_label, fontsize=axis_label_fontsize)
    # ax.set_title('Title', fontsize=title_fontsize)

    # Add legend
    ax.legend(title=legend_title, loc=legend_loc, fontsize=legend_fontsize)

    # Show grid lines for clarity
    ax.grid(axis='x', linestyle='--')

    plt.tight_layout()
    plt.savefig(figure_filename, dpi=300, bbox_inches='tight')


def get_experiment_times_of_algorithm(experiment, step='seconds-total'):
    algorithm_name = experiment['algorithm-name']
    trials = experiment.get('trials', [])
    trial_times = [trial[step] for trial in trials]
    return algorithm_name, trial_times


def get_experiment_average_time_of_algorithm(experiment, step='seconds-total'):
    algorithm_name, trial_times = get_experiment_times_of_algorithm(experiment, step)
    avg_time = sum(trial_times) / len(trial_times) if trial_times else 0
    return algorithm_name, avg_time


def get_run_time_info(filename, average=True, step='seconds-total'):
    try:
        # Read the yaml file
        with open(filename, "r") as file:
            content = yaml.load(file, Loader=yaml.FullLoader)[0]
            experiments = content["experiments"]
            # print(content)
            time_data_per_algorithm = {}
            for experiment in experiments:
                if average:
                    algorithm_name, avg_time = get_experiment_average_time_of_algorithm(experiment, step)
                    time_data_per_algorithm[algorithm_name] = avg_time
                else:
                    algorithm_name, trial_times = get_experiment_times_of_algorithm(experiment, step)
                    time_data_per_algorithm[algorithm_name] = trial_times
            return time_data_per_algorithm
    except (yaml.YAMLError, IndexError) as e:
        # print(f"Error parsing YAML file: {e}")
        return None  # Or handle the error differently, e.g., raise an exception


if method == 0 or method == 1:
    print("Print Ideal Batch size on CPU")
    os.makedirs(fig_cpu_ideal_batch_size_dir, exist_ok=True)

    # Get ideal batch as a whole
    cpu_time_data_per_batch = {}
    for dataset in datasets:
        dataset_name = get_dataset_name(dataset)
        cpu_time_data_per_batch[dataset_name] = {}
        for batch_size in batch_sizes:
            batch_trial_times = []
            for percentage in percentages:
                cpu_time_file = f"{data_cpu_ideal_batch_size_dir}/{dataset_name}_batch_{batch_size}_per_{percentage}.yaml"
                cpu_run_time = get_run_time_info(cpu_time_file, average=False)
                dp_batch_clip_times = cpu_run_time[
                    algorithms_names[algorithms[-1]]] if cpu_run_time is not None else [np.nan] * iterations
                for trial_time in dp_batch_clip_times:
                    batch_trial_times.append(trial_time)
            cpu_time_data_per_batch[dataset_name][batch_size] = batch_trial_times

    for dataset, batch_data in cpu_time_data_per_batch.items():
        filename_prefix = f"{fig_cpu_ideal_batch_size_dir}/{dataset}"
        print(f"Processing file: {filename_prefix}")
        # Convert the data to a pandas DataFrame
        df_times_per_batch = pd.DataFrame(batch_data)
        df_times_per_batch.index.name = 'Trial'
        df_times_per_batch.columns.name = 'Batch size'

        df_times_per_batch.to_csv(f"{filename_prefix}.csv", index=True, header=True)
        print(df_times_per_batch)

        # Create bar chart
        create_box_plot_chart(df_times_per_batch, 'Batch size', 'CPU time (seconds)', f"{filename_prefix}.png")

    # Get ideal batch per step
    for step_id, step in enumerate(dp_batch_clip_steps):
        step_id_str = f"{step_id:02d}"  # Format as a two-digit number
        step_name = step.replace("seconds-", "")

        gpu_time_data_per_batch = {}
        for dataset in datasets:
            dataset_name = get_dataset_name(dataset)
            gpu_time_data_per_batch[dataset_name] = {}
            for batch_size in batch_sizes:
                batch_trial_times = []
                for percentage in percentages:
                    gpu_time_file = f"{data_cpu_ideal_batch_size_dir}/{dataset_name}_batch_{batch_size}_per_{percentage}.yaml"
                    gpu_run_time = get_run_time_info(gpu_time_file, average=False, step=step)
                    dp_batch_clip_times = gpu_run_time[
                        algorithms_names[algorithms[-1]]] if gpu_run_time is not None else [np.nan] * iterations
                    for trial_time in dp_batch_clip_times:
                        batch_trial_times.append(trial_time)
                gpu_time_data_per_batch[dataset_name][batch_size] = batch_trial_times

        for dataset, batch_data in gpu_time_data_per_batch.items():
            # remove "seconds-" from the step name
            filename_prefix = f"{fig_cpu_ideal_batch_size_dir}/{dataset}_{step_id_str}_{step_name}"
            print(f"Processing file: {filename_prefix}")
            # Convert the data to a pandas DataFrame
            df_times_per_batch = pd.DataFrame(batch_data)
            df_times_per_batch.index.name = 'Trial'
            df_times_per_batch.columns.name = 'Batch size'

            # Convert the data to numeric
            df_times_per_batch = df_times_per_batch.apply(pd.to_numeric, errors='coerce')

            df_times_per_batch.to_csv(f"{filename_prefix}.csv", index=True, header=True)
            print(df_times_per_batch)

            # Create bar chart
            create_box_plot_chart(df_times_per_batch, 'Batch size', 'GPU time (seconds)', f"{filename_prefix}.png")

if method == 0 or method == 2:
    print("Print Ideal Batch size on GPU")
    os.makedirs(fig_gpu_ideal_batch_size_dir, exist_ok=True)

    gpu_time_data_per_batch = {}
    for dataset in datasets:
        dataset_name = get_dataset_name(dataset)
        gpu_time_data_per_batch[dataset_name] = {}
        for batch_size in batch_sizes:
            batch_trial_times = []
            for percentage in percentages:
                gpu_time_file = f"{data_gpu_ideal_batch_size_dir}/{dataset_name}_batch_{batch_size}_per_{percentage}.yaml"
                gpu_run_time = get_run_time_info(gpu_time_file, average=False)
                dp_batch_clip_times = gpu_run_time[
                    algorithms_names[algorithms[-1]]] if gpu_run_time is not None else [np.nan] * iterations
                for trial_time in dp_batch_clip_times:
                    batch_trial_times.append(trial_time)
            gpu_time_data_per_batch[dataset_name][batch_size] = batch_trial_times

    for dataset, batch_data in gpu_time_data_per_batch.items():
        filename_prefix = f"{fig_gpu_ideal_batch_size_dir}/{dataset}"
        print(f"Processing file: {filename_prefix}")
        # Convert the data to a pandas DataFrame
        df_times_per_batch = pd.DataFrame(batch_data)
        df_times_per_batch.index.name = 'Trial'
        df_times_per_batch.columns.name = 'Batch size'

        df_times_per_batch.to_csv(f"{filename_prefix}.csv", index=True, header=True)
        print(df_times_per_batch)

        # Create bar chart
        create_box_plot_chart(df_times_per_batch, 'Batch size', 'GPU time (seconds)', f"{filename_prefix}.png")

    # Get ideal batch per step
    for step_id, step in enumerate(dp_batch_clip_steps):
        step_id_str = f"{step_id:02d}"  # Format as a two-digit number
        step_name = step.replace("seconds-", "")

        gpu_time_data_per_batch = {}
        for dataset in datasets:
            dataset_name = get_dataset_name(dataset)
            gpu_time_data_per_batch[dataset_name] = {}
            for batch_size in batch_sizes:
                batch_trial_times = []
                for percentage in percentages:
                    gpu_time_file = f"{data_gpu_ideal_batch_size_dir}/{dataset_name}_batch_{batch_size}_per_{percentage}.yaml"
                    gpu_run_time = get_run_time_info(gpu_time_file, average=False, step=step)
                    dp_batch_clip_times = gpu_run_time[
                        algorithms_names[algorithms[-1]]] if gpu_run_time is not None else [np.nan] * iterations
                    for trial_time in dp_batch_clip_times:
                        batch_trial_times.append(trial_time)
                gpu_time_data_per_batch[dataset_name][batch_size] = batch_trial_times

        for dataset, batch_data in gpu_time_data_per_batch.items():
            # remove "seconds-" from the step name
            filename_prefix = f"{fig_gpu_ideal_batch_size_dir}/{dataset}_{step_id_str}_{step_name}"
            print(f"Processing file: {filename_prefix}")
            # Convert the data to a pandas DataFrame
            df_times_per_batch = pd.DataFrame(batch_data)
            df_times_per_batch.index.name = 'Trial'
            df_times_per_batch.columns.name = 'Batch size'

            # Convert the data to numeric
            df_times_per_batch = df_times_per_batch.apply(pd.to_numeric, errors='coerce')

            df_times_per_batch.to_csv(f"{filename_prefix}.csv", index=True, header=True)
            print(df_times_per_batch)

            # Create bar chart
            create_box_plot_chart(df_times_per_batch, 'Batch size', 'GPU time (seconds)', f"{filename_prefix}.png")

if method == 0 or method == 3:
    print("Print CPU Time information")
    os.makedirs(fig_cpu_time_dir, exist_ok=True)

    cpu_time_data_1_threads = {}
    cpu_time_data_max_threads = {}
    for dataset in datasets:
        dataset_name = get_dataset_name(dataset)

        for percentage in percentages:
            output_dataset_name = f"{dataset_name}_per_{percentage}"

            time_file_1_threads = f"{data_cpu_time_dir}/{output_dataset_name}_1_threads.yaml"
            run_time_1_threads = get_run_time_info(time_file_1_threads)
            cpu_time_data_1_threads[
                output_dataset_name] = run_time_1_threads if run_time_1_threads is not None else np.nan

            time_file_max_threads = f"{data_cpu_time_dir}/{output_dataset_name}_{max_number_of_threads}_threads.yaml"
            run_time_max_threads = get_run_time_info(time_file_max_threads)
            cpu_time_data_max_threads[
                output_dataset_name] = run_time_max_threads if run_time_max_threads is not None else np.nan

    filename_prefixes = [f"{fig_cpu_time_dir}/cpu_time_1_threads",
                         f"{fig_cpu_time_dir}/cpu_time_{max_number_of_threads}_threads"]
    cpu_time_data_all = [cpu_time_data_1_threads,
                         cpu_time_data_max_threads]
    for filename_prefix, cpu_time_data in zip(filename_prefixes, cpu_time_data_all):
        print(f"Processing file: {filename_prefix}")
        # Convert the data to a pandas DataFrame
        df_cpu_time = pd.DataFrame(cpu_time_data)
        df_cpu_time.index.name = 'Algorithm'
        df_cpu_time.to_csv(f"{filename_prefix}.csv", index=True, header=True)
        print(df_cpu_time)

        # Print improvement ratios
        print_improvement_ratio(df_cpu_time)

        # Create bar chart
        create_bar_chart(df_cpu_time, True, 'CPU time (seconds)', 'Datasets', 'Algorithms', 'lower right',
                         f"{filename_prefix}.png")

if method == 0 or method == 4:
    print("Print Speed-up information")
    os.makedirs(fig_speed_up_dir, exist_ok=True)

    cpu_time = {}
    speed_up_datasets_names = [get_dataset_name(dataset) for dataset in speed_up_datasets]
    for dataset_name in speed_up_datasets_names:
        cpu_time[dataset_name] = {}
        for power in range(0, max_number_of_threads_power_of_2 + 1):
            threads = int(math.pow(2, power))
            cpu_time[dataset_name][threads] = get_run_time_info(
                f"{data_speed_up_dir}/{dataset_name}_per_{largest_percentage}_{threads}_threads.yaml")

    filename_prefixes = [f"{fig_speed_up_dir}/{dataset_name}_speed_up" for dataset_name in speed_up_datasets_names]
    for dataset_name, filename_prefix in zip(speed_up_datasets_names, filename_prefixes):
        print(f"Processing file: {filename_prefix}")

        df_speed_up = pd.DataFrame(cpu_time[dataset_name]).transpose()
        df_speed_up.index.name = 'Threads'
        df_speed_up.to_csv(f"{filename_prefix}.csv", index=True, header=True)
        print(df_speed_up)

        # Create the figure
        fig, ax = plt.subplots(figsize=(fig_width, fig_height))

        # Loop through each dataset and plot its values across threads
        for algorithm_name in df_speed_up.columns:
            # Extract time for the dataset
            values = df_speed_up[algorithm_name].values
            # speed_up_values = values[0] / values # speed-up over itself
            speed_up_values = df_speed_up.iloc[0].min() / values  # speed-up over the best sequential time

            # # Plot each dataset's speed-up as a line for each thread
            # ax.plot(df_speed_up.index, speed_up_values, marker='o', label=algorithm_name, alpha=0.8,
            #         color=algorithm_colors[algorithm_name])

            # Plot the solid line up to the second-to-last point, for actual threads
            ax.plot(df_speed_up.index[:-1], speed_up_values[:-1], marker='o', label=algorithm_name,
                    alpha=0.8, color=algorithm_colors[algorithm_name])

            # Plot the last point with a dashed line, for hyper threads
            ax.plot(df_speed_up.index[-2:], speed_up_values[-2:], marker='o', linestyle='--',
                    color=algorithm_colors[algorithm_name])

        # Set the x-axis to a logarithmic scale
        ax.set_xscale('log', base=2)
        ax.set_xticks(df_speed_up.index)
        ax.set_xticklabels(df_speed_up.index)

        # Add labels and title
        ax.set_ylabel('Speed-up', fontsize=axis_label_fontsize)
        ax.set_xlabel('Threads', fontsize=axis_label_fontsize)
        # ax.set_title('Speed-Up by Threads', fontsize=title_fontsize)

        # Add legend
        ax.legend(title='Algorithms', loc='upper left', fontsize=legend_fontsize)

        # Show grid lines for clarity
        ax.grid()

        filename = f"{filename_prefix}.png"
        plt.tight_layout()
        plt.savefig(filename, dpi=300, bbox_inches='tight')

if method == 0 or method == 5:
    print("Print GPU Time information")
    os.makedirs(fig_gpu_time_dir, exist_ok=True)

    gpu_time_data = {}
    for dataset in datasets:
        dataset_name = get_dataset_name(dataset)
        for percentage in percentages:
            output_dataset_name = f"{dataset_name}_per_{percentage}"
            gpu_time_data[output_dataset_name] = {}
            for algo in vtkm_algorithms:
                algorithm_name = algorithms_names[algo]
                output_file = f"{data_gpu_time_dir}/{output_dataset_name}_{algorithm_name}.yaml"
                run_time = get_run_time_info(output_file)
                gpu_time_data[output_dataset_name][algorithm_name] = run_time[
                    algorithm_name] if run_time is not None else np.nan

    filename_prefix = f"{fig_gpu_time_dir}/gpu_time"
    print(f"Processing file: {filename_prefix}")
    # Convert the data to a pandas DataFrame
    df_gpu_time = pd.DataFrame(gpu_time_data)
    df_gpu_time.index.name = 'Algorithm'
    df_gpu_time.to_csv(f"{filename_prefix}.csv", index=True, header=True)
    print(df_gpu_time)

    # Print improvement ratios
    print_improvement_ratio(df_gpu_time)

    # Create bar chart
    create_bar_chart(df_gpu_time, False, 'GPU time (seconds)', 'Datasets', 'Algorithms', 'lower right',
                     f"{filename_prefix}.png")

if method == 0 or method == 6:
    print("Print Memory Footprint information")
    os.makedirs(fig_memory_footprint_dir, exist_ok=True)


    def get_memory_footprint_info(filename):
        # Read the file
        with open(filename, "r") as file:
            content = file.read()
            # Use regular expressions to find the values
            dataset_memory_used_match = re.search(r"dataset-memory-used:\s*(\d+)", content)
            max_resident_set_size_match = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", content)
            # Extract the values if found
            if dataset_memory_used_match and max_resident_set_size_match:
                dataset_memory_used = int(dataset_memory_used_match.group(1))
                max_resident_set_size = int(max_resident_set_size_match.group(1))
                algorithm_memory_used = (max_resident_set_size - dataset_memory_used) / (1024 * 1024)  # Convert to GB
                return algorithm_memory_used
            else:
                print("Error while reading the file")
                return 0


    memory_footprint_data = {}
    for dataset in datasets:
        dataset_name = get_dataset_name(dataset)
        for percentage in percentages:
            output_dataset_name = f"{dataset_name}_per_{percentage}"
            memory_footprint_data[output_dataset_name] = {}
            for algo in non_batch_algorithms:
                algorithm_name = algorithms_names[algo]
                output_file = f"{data_memory_footprint_dir}/{output_dataset_name}_{algorithm_name}.txt"
                memory_footprint_data[output_dataset_name][algorithm_name] = get_memory_footprint_info(output_file)
            for algo in batch_algorithms:
                for batch_size in ideal_batch_sizes:
                    algorithm_name = f'{algorithms_names[algo]}_batch_{batch_size}'
                    output_file = f"{data_memory_footprint_dir}/{output_dataset_name}_{algorithm_name}.txt"
                    memory_footprint_data[output_dataset_name][algorithm_name] = get_memory_footprint_info(output_file)

    # Convert the data to a pandas DataFrame
    df_memory_footprint = pd.DataFrame(memory_footprint_data)
    df_memory_footprint.index.name = 'Algorithm'
    df_memory_footprint.to_csv(f"{fig_memory_footprint_dir}/memory_footprint.csv", index=True, header=True)
    print(df_memory_footprint)

    # Print improvement ratios
    print_improvement_ratio(df_memory_footprint)

    # Create bar chart
    create_bar_chart(df_memory_footprint, True, 'Memory footprint (gigabytes)', 'Datasets', 'Algorithms', 'lower right',
                     f"{fig_memory_footprint_dir}/memory_footprint.png")