import os
import pandas as pd 
import matplotlib.pyplot as plt
import seaborn as sns
import json

def style_axes_grid(axes, log_y=False, labelsize=14, rotation=45, fontweight='bold'):
    for row in axes:
        for ax in row:
            if log_y:
                ax.set_yscale('log')
            ax.tick_params(axis='x', labelsize=labelsize, labelrotation=rotation)
            ax.tick_params(axis='y', labelsize=labelsize)
            for label in ax.get_xticklabels():
                label.set_fontweight(fontweight)
            for label in ax.get_yticklabels():
                label.set_fontweight(fontweight)

def style_single_plot(ax=None, log_y=False, labelsize=14, rotation=45, fontweight='bold', plot_type=None):
    if ax is None:
        ax = plt.gca()
    if log_y:
        ax.set_yscale('log')
    ax.tick_params(axis='x', labelsize=labelsize, labelrotation=rotation)
    ax.tick_params(axis='y', labelsize=labelsize)
    if plot_type != "throughput_gain":
        plt.legend(fontsize=17)
    for label in ax.get_xticklabels():
        label.set_fontweight(fontweight)
    for label in ax.get_yticklabels():
        label.set_fontweight(fontweight)


time_window_sizes = [50, 100, 200] 

csv_files = [os.path.join('benchmarks', file) for file in os.listdir('benchmarks') if file.endswith('.csv')]
performance_gain_path = os.path.join('benchmarks', 'performance_gain_by_tws.json')
performance_gain_dict = {}

for csv_file in csv_files:
    pattern_to_whitespace = os.path.basename(csv_file).replace('.csv', '').replace('%20', ' ')
    pattern = f"<{pattern_to_whitespace}>"
    
    output_folder = os.path.join('benchmarks/output', os.path.basename(csv_file).replace('.csv', ''))
    os.makedirs(output_folder, exist_ok=True)
    
    df = pd.read_csv(csv_file)
    
    for time_window in [None] + time_window_sizes:  # None for the diagram with all tws
        filtered_df = df if time_window is None else df[df['time_window_size'] == time_window]
        suffix = "all" if time_window is None else f"tws_{time_window}"
        output_image_path = os.path.join(output_folder, f"{suffix}.png")
        
        # --- DBrex results ---
        dbrex_data = filtered_df[filtered_df['algorithm'] == 'dbrex']
        dbrex_average = dbrex_data.groupby('#test', sort=False).agg({
            'latency': 'mean',
            'delta_latency': 'mean',          
            'throughput': 'mean',      
            'memory': 'mean',          
            'time_window_size': 'first', 
            'total_elements': 'first',      
            'delta_elements': 'first',      
            'timestamp': 'first'
        })
        
        # --- Self JOINS results ---
        self_joins_data = filtered_df[filtered_df['algorithm'] == 'self_joins']
        self_joins_average = self_joins_data.groupby('#test', sort=False).agg({
            'latency': 'mean',
            'delta_latency': 'mean',          
            'total_elements': 'first',      
            'delta_elements': 'first',      
            'timestamp': 'first'
        })
        
        # # --- 2x2 Plot ---
        # fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        # style_axes_grid(axes)

        # # --- Delta Latency ---
        # sns.lineplot(ax=axes[0, 0], data=dbrex_average, x='total_elements', y='delta_latency', marker='o', label='DBrex', color='blue')
        # sns.lineplot(ax=axes[0, 0], data=self_joins_average, x='total_elements', y='delta_latency', marker='s', label='Self JOINS', color='red')
        # axes[0, 0].set_xlabel("Total Elements")
        # axes[0, 0].set_ylabel("Delta Latency in s")
        # axes[0, 0].set_title("Delta Latency")
        # axes[0, 0].legend()
        # axes[0, 0].grid(True)

        # # --- Total Latency ---
        # sns.lineplot(ax=axes[0, 1], data=dbrex_average, x='total_elements', y='latency', marker='o', label='DBrex', color='blue')
        # sns.lineplot(ax=axes[0, 1], data=self_joins_average, x='total_elements', y='latency', marker='s', label='Self JOINS', color='red')
        # axes[0, 1].set_xlabel("Total Elements")
        # axes[0, 1].set_ylabel("Total Latency in s")
        # axes[0, 1].set_title("Total Latency")
        # axes[0, 1].legend()
        # axes[0, 1].grid(True)

        # # --- Throughput (DBrex) ---
        # sns.lineplot(ax=axes[1, 0], data=dbrex_average, x='total_elements', y='throughput', marker='o', label='DBrex', color='green')
        # axes[1, 0].set_xlabel("Total Elements")
        # axes[1, 0].set_ylabel("Throughput in elements/s")
        # axes[1, 0].set_title("Throughput (DBrex)")
        # axes[1, 0].legend()
        # axes[1, 0].grid(True)

        # # --- Memory Usage (DBrex) ---
        # sns.lineplot(ax=axes[1, 1], data=dbrex_average, x='total_elements', y='memory', marker='o', label='DBrex', color='purple')
        # axes[1, 1].set_xlabel("Total Elements")
        # axes[1, 1].set_ylabel("Storage Usage in Bytes")
        # axes[1, 1].set_title("Storage Usage (DBrex)")
        # axes[1, 1].legend()
        # axes[1, 1].grid(True)

        # Save 2x2 plot
        # plt.tight_layout(rect=[0, 0, 1, 0.96])
        # plt.savefig(output_image_path, dpi=300)
        # plt.close(fig)

        # --- Save each plot separately ---
        # Save Delta Latency Plot
        delta_latency_image_path = os.path.join(output_folder, f"delta_latency_{suffix}.png")
        plt.figure(figsize=(8, 6))
        sns.lineplot(data=dbrex_average, x='total_elements', y='delta_latency', marker='o', label='DBrex', color='blue', errorbar=('ci', 100))
        sns.lineplot(data=self_joins_average, x='total_elements', y='delta_latency', marker='s', label='Self JOINS', color='red', errorbar=('ci', 100))
        plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        plt.ylabel("Delta Latency in s", fontsize=16, fontweight='bold', labelpad=14)
        plt.legend()
        plt.grid(True)
        style_single_plot()
        plt.subplots_adjust(left=0.2)
        plt.subplots_adjust(bottom=0.2)
        plt.savefig(delta_latency_image_path, dpi=300)
        plt.close()

        # Save Delta Latency Plot (log y)
        delta_latency_log_image_path = os.path.join(output_folder, f"delta_latency_log_{suffix}.png")
        plt.figure(figsize=(8, 6))
        sns.lineplot(data=dbrex_average, x='total_elements', y='delta_latency', marker='o', label='DBrex', color='blue', errorbar=('ci', 100))
        sns.lineplot(data=self_joins_average, x='total_elements', y='delta_latency', marker='s', label='Self JOINS', color='red', errorbar=('ci', 100))
        plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        plt.ylabel("Delta Latency in s", fontsize=16, fontweight='bold', labelpad=14)
        plt.legend()
        plt.grid(True)
        style_single_plot(log_y=True)
        plt.subplots_adjust(left=0.2)
        plt.subplots_adjust(bottom=0.2)
        plt.savefig(delta_latency_log_image_path, dpi=300)
        plt.close()

        # Save Total Latency Plot
        total_latency_image_path = os.path.join(output_folder, f"total_latency_{suffix}.png")
        plt.figure(figsize=(8, 6))
        sns.lineplot(data=dbrex_average, x='total_elements', y='latency', marker='o', label='DBrex', color='blue', errorbar=('ci', 100))
        sns.lineplot(data=self_joins_average, x='total_elements', y='latency', marker='s', label='Self JOINS', color='red', errorbar=('ci', 100))
        plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        plt.ylabel("Total Latency in s", fontsize=16, fontweight='bold')
        plt.legend()
        plt.grid(True)
        style_single_plot()
        plt.subplots_adjust(left=0.2)
        plt.subplots_adjust(bottom=0.2)
        plt.savefig(total_latency_image_path, dpi=300)
        plt.close()

        # Save Total Latency Plot (log y)
        total_latency_log_image_path = os.path.join(output_folder, f"total_latency_log_{suffix}.png")
        plt.figure(figsize=(8, 6))
        sns.lineplot(data=dbrex_average, x='total_elements', y='latency', marker='o', label='DBrex', color='blue', errorbar=('ci', 100))
        sns.lineplot(data=self_joins_average, x='total_elements', y='latency', marker='s', label='Self JOINS', color='red', errorbar=('ci', 100))
        plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        plt.ylabel("Total Latency in s", fontsize=16, fontweight='bold')
        plt.legend()
        plt.grid(True)
        style_single_plot(log_y=True)
        plt.subplots_adjust(left=0.2)
        plt.subplots_adjust(bottom=0.2)
        plt.savefig(total_latency_log_image_path, dpi=300)
        plt.close()


        # Save Throughput Plot
        # throughput_image_path = os.path.join(output_folder, f"throughput_{suffix}.png")
        # plt.figure(figsize=(8, 6))
        # sns.lineplot(data=dbrex_average, x='total_elements', y='throughput', marker='o', label='DBrex', color='green')
        # plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        # plt.ylabel("Throughput in elements/s", fontsize=16, fontweight='bold')
        # plt.legend()
        # plt.grid(True)
        # style_single_plot()
        # plt.subplots_adjust(bottom=0.2)
        # plt.savefig(throughput_image_path, dpi=300)
        # plt.close()

        # Save Storage Usage Plot
        memory_image_path = os.path.join(output_folder, f"memory_{suffix}.png")
        plt.figure(figsize=(8, 6))
        sns.lineplot(data=dbrex_average, x='total_elements', y='memory', marker='o', label='DBrex', color='purple', errorbar=('ci', 100))
        plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        plt.ylabel("Storage Usage in Bytes", fontsize=16, fontweight='bold')
        plt.legend()
        plt.grid(True)
        style_single_plot()
        plt.gca().yaxis.offsetText.set_fontsize(17)
        plt.subplots_adjust(left=0.2)
        plt.subplots_adjust(bottom=0.2)
        plt.savefig(memory_image_path, dpi=300)
        plt.close()

        # --- Throughput Gain Plot ---
        throughput_gain_df = pd.DataFrame()
        throughput_gain_df['total_elements'] = dbrex_average['total_elements']
        throughput_gain_df['time_window_size'] = dbrex_average['time_window_size']
        throughput_gain_df['throughput_gain'] = self_joins_average['latency'] / dbrex_average['latency']

        throughput_gain_image_path = os.path.join(output_folder, f"throughput_gain_{suffix}.png")
        plt.figure(figsize=(8, 6))
        sns.lineplot(data=throughput_gain_df, x='total_elements', y='throughput_gain', marker='o', color='darkorange', errorbar=('ci', 100))
        plt.xlabel("Total Elements", fontsize=16, fontweight='bold')
        plt.ylabel("Throughput Gain (Self Joins / DBrex)", fontsize=16, fontweight='bold')
        plt.grid(True)
        style_single_plot(plot_type="throughput_gain")
        plt.subplots_adjust(left=0.2)
        plt.subplots_adjust(bottom=0.2)
        plt.savefig(throughput_gain_image_path, dpi=300)
        plt.close()

        if time_window is None: # all values
            last_values = throughput_gain_df.groupby('time_window_size', sort=False).tail(1)
            gain_50 = last_values.loc[last_values['time_window_size'] == 50, 'throughput_gain'].values[0]

            gain_200_rows = last_values[last_values['time_window_size'] == 200]
            if not gain_200_rows.empty: # due to missing tws 200 in one experiment
                gain_200 = gain_200_rows['throughput_gain'].values[0]
            else:
                gain_200 = last_values.loc[last_values['time_window_size'] == 100, 'throughput_gain'].values[0]
            performance_gain_by_tws = (gain_200-gain_50)/gain_50
            
            performance_gain_dict[pattern_to_whitespace] = performance_gain_by_tws

total_performance_gain = 0

for _, performance_gain in performance_gain_dict.items():
    total_performance_gain += performance_gain

performance_gain_dict["avg"] = total_performance_gain/len(performance_gain_dict)

with open(performance_gain_path, "w") as f:
    json.dump(performance_gain_dict, f, indent=4)