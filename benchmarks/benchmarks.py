import os
import pandas as pd 
import matplotlib.pyplot as plt
import seaborn as sns

time_window_sizes = [50, 100, 200] 

csv_files = [os.path.join('benchmarks', file) for file in os.listdir('benchmarks') if file.endswith('.csv')]

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
        
        # ! diagram with all tws: lineplot default confidence intervall is .95

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle(f"Pattern: {pattern} | Time Window Size: {time_window if time_window else 'All'}", fontsize=16, fontweight='bold')

        # --- Delta Latency ---
        sns.lineplot(ax=axes[0, 0], data=dbrex_average, x='total_elements', y='delta_latency', marker='o', label='DBrex', color='blue')
        sns.lineplot(ax=axes[0, 0], data=self_joins_average, x='total_elements', y='delta_latency', marker='s', label='Self JOINS', color='red')
        axes[0, 0].set_xlabel("Total Elements")
        axes[0, 0].set_ylabel("Delta Latency in s")
        axes[0, 0].set_title("Delta Latency")
        axes[0, 0].legend()
        axes[0, 0].grid(True)

        # --- Total Latency ---
        sns.lineplot(ax=axes[0, 1], data=dbrex_average, x='total_elements', y='latency', marker='o', label='DBrex', color='blue')
        sns.lineplot(ax=axes[0, 1], data=self_joins_average, x='total_elements', y='latency', marker='s', label='Self JOINS', color='red')
        axes[0, 1].set_xlabel("Total Elements")
        axes[0, 1].set_ylabel("Total Latency in s")
        axes[0, 1].set_title("Total Latency")
        axes[0, 1].legend()
        axes[0, 1].grid(True)

        # --- Throughput (DBrex) ---
        sns.lineplot(ax=axes[1, 0], data=dbrex_average, x='total_elements', y='throughput', marker='o', label='DBrex', color='green')
        axes[1, 0].set_xlabel("Total Elements")
        axes[1, 0].set_ylabel("Throughput in elements/s")
        axes[1, 0].set_title("Throughput (DBrex)")
        axes[1, 0].legend()
        axes[1, 0].grid(True)

        # --- Memory Usage (DBrex) ---
        sns.lineplot(ax=axes[1, 1], data=dbrex_average, x='total_elements', y='memory', marker='o', label='DBrex', color='purple')
        axes[1, 1].set_xlabel("Total Elements")
        axes[1, 1].set_ylabel("Memory Usage in Bytes")
        axes[1, 1].set_title("Memory Usage (DBrex)")
        axes[1, 1].legend()
        axes[1, 1].grid(True)

        plt.tight_layout(rect=[0, 0, 1, 0.96])
        plt.savefig(output_image_path, dpi=300)
        plt.close(fig)
