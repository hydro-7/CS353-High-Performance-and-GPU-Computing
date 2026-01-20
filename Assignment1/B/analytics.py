import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# 1. Load the Data
try:
    df = pd.read_csv("results_full_scaling.csv")
except FileNotFoundError:
    print("Error: 'results_full_scaling.csv' not found. Please run the C code or the Generator script first.")
    exit()

df['Speedup'] = 0.0


for size in df['Size'].unique():
    for method in df['Method'].unique():
   
        base_row = df[(df['Size'] == size) & (df['Method'] == method) & (df['Threads'] == 1)]
        
        if not base_row.empty:
            t1 = base_row['Time_Sec'].values[0]
         
            mask = (df['Size'] == size) & (df['Method'] == method)
            df.loc[mask, 'Speedup'] = t1 / df.loc[mask, 'Time_Sec']

idx_min = df.groupby(['Size', 'Method'])['Time_Sec'].idxmin()
optimal_df = df.loc[idx_min].sort_values(by=['Size', 'Time_Sec'])

print("="*80)
print(f"{'OPTIMAL THREAD CONFIGURATIONS':^80}")
print("="*80)
print(f"{'Size':<10} | {'Method':<15} | {'Opt Threads':<12} | {'Min Time (s)':<12} | {'Max Speedup':<12}")
print("-" * 80)
for _, row in optimal_df.iterrows():
    print(f"{int(row['Size']):<10} | {row['Method']:<15} | {int(row['Threads']):<12} | {row['Time_Sec']:.6f}     | {row['Speedup']:.2f}x")
print("="*80)

# 4. Visualization
sns.set_theme(style="whitegrid", context="talk")
fig, axes = plt.subplots(1, 2, figsize=(20, 8))


plot_size = df['Size'].max() 
subset = df[df['Size'] == plot_size]

sns.lineplot(ax=axes[0], data=subset, x="Threads", y="Time_Sec", hue="Method", style="Method", linewidth=3)
axes[0].set_yscale("log")
axes[0].set_title(f"Execution Time vs Threads (Size: {plot_size}x{plot_size})")
axes[0].set_ylabel("Time (Seconds) [Log Scale]")
axes[0].set_xlabel("Number of Threads")
axes[0].grid(True, which="both", ls="--", alpha=0.5)


axes[0].axvline(x=12, color='red', linestyle='--', alpha=0.3)
axes[0].text(13, subset['Time_Sec'].min(), "Diminishing Returns Start", color='red', fontsize=10)


sns.lineplot(ax=axes[1], data=subset, x="Threads", y="Speedup", hue="Method", style="Method", linewidth=3)


max_th = subset['Threads'].max()
axes[1].plot([1, max_th], [1, max_th], 'k--', alpha=0.3, label="Ideal Linear (Perfect)")

axes[1].set_title(f"Speedup Factor vs Threads (Higher is Better)")
axes[1].set_ylabel("Speedup (X times faster than 1 thread)")
axes[1].set_xlabel("Number of Threads")
axes[1].set_xlim(1, 100) # Zoom in on 1-100 as 100-200 usually flattens
axes[1].legend()

plt.tight_layout()
plt.savefig("matrix_analysis_plot.png") 
plt.show()
