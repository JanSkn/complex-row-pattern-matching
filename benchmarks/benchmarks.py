import os
import pandas as pd 
import matplotlib.pyplot as plt
import seaborn as sns

csv_files = [os.path.join('benchmarks', file) for file in os.listdir('benchmarks') if file.endswith('.csv')]

for csv_file in csv_files:
    pattern_to_whitespace = os.path.basename(csv_file).replace('.csv', '').replace('%20', ' ')
    pattern = f"<{pattern_to_whitespace}>"

    output_image_path = os.path.join('benchmarks/output', f"{os.path.basename(csv_file).replace('.csv', '')}.png")

    df = pd.read_csv(csv_file)
    
    # --- DBrex results ---
    dbrex_data = df[df['algorithm'] == 'dbrex']

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
    # ---------------------

    # --- MATCH_RECOGNIZE results ---
    match_recognize_data = df[df['algorithm'] == 'MATCH_RECOGNIZE']

    match_recognize_average = match_recognize_data.groupby('#test', sort=False).agg({
        'latency': 'mean',
        'delta_latency': 'mean',          
        'total_elements': 'first',      
        'delta_elements': 'first',      
        'timestamp': 'first'
    })
    # -------------------------------

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f"Pattern: {pattern}", fontsize=16, fontweight='bold')

    # --- Delta Latency ---
    sns.lineplot(ax=axes[0, 0], data=dbrex_average, x='total_elements', y='delta_latency', marker='o', label='DBrex', color='blue')
    sns.lineplot(ax=axes[0, 0], data=match_recognize_average, x='total_elements', y='delta_latency', marker='s', label='MATCH_RECOGNIZE', color='red')

    axes[0, 0].set_xlabel("Total Elements")
    axes[0, 0].set_ylabel("Delta Latency in s")
    axes[0, 0].set_title("Delta Latency")
    axes[0, 0].legend()
    axes[0, 0].grid(True)

    # --- Total Latency ---
    sns.lineplot(ax=axes[0, 1], data=dbrex_average, x='total_elements', y='latency', marker='o', label='DBrex', color='blue')
    sns.lineplot(ax=axes[0, 1], data=match_recognize_average, x='total_elements', y='latency', marker='s', label='MATCH_RECOGNIZE', color='red')

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