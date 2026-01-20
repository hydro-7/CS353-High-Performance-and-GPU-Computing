import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read the results
df = pd.read_csv('matrix_mult_single_thread_results.csv')

# Create plot
plt.figure(figsize=(12, 8))

patterns = ['Pattern1_ijk', 'Pattern2_ikj', 'Pattern3_jik', 
            'Pattern4_Blocked', 'Pattern5_SIMD', 'Pattern6_RegBlock']
pattern_names = ['Pattern 1 (ijk)', 'Pattern 2 (ikj)', 'Pattern 3 (jik)',
                 'Pattern 4 (Blocked)', 'Pattern 5 (SIMD)', 'Pattern 6 (Reg Block)']

colors = ['red', 'blue', 'green', 'orange', 'purple', 'brown']
markers = ['o', 's', '^', 'D', 'v', '<']

for i, pattern in enumerate(patterns):
    plt.plot(df['MatrixSize'], df[pattern], 
             label=pattern_names[i], 
             color=colors[i], 
             marker=markers[i],
             linewidth=2,
             markersize=8)

plt.xlabel('Matrix Size (n x n)', fontsize=12)
plt.ylabel('Execution Time (seconds)', fontsize=12)
plt.title('Single-Threaded Matrix Multiplication Performance\n(5 Different Data Access Patterns)', fontsize=14, fontweight='bold')
plt.xscale('log', base=2)
plt.yscale('log')
plt.xticks(df['MatrixSize'], df['MatrixSize'])
plt.grid(True, which='both', linestyle='--', alpha=0.7)
plt.legend(fontsize=10)
plt.tight_layout()

# Save the plot
plt.savefig('matrix_mult_single_thread_performance.png', dpi=300, bbox_inches='tight')
plt.show()

# Create speedup comparison bar chart
plt.figure(figsize=(14, 8))

# Calculate speedup relative to Pattern 1
speedup_data = {}
for pattern in patterns[1:]:
    speedup_data[pattern] = df['Pattern1_ijk'] / df[pattern]

x = np.arange(len(df['MatrixSize']))
width = 0.15

for i, (pattern, speedup) in enumerate(speedup_data.items()):
    offset = (i - len(patterns[1:])/2) * width
    plt.bar(x + offset, speedup, width, 
            label=pattern_names[i+1].split('(')[0].strip())

plt.xlabel('Matrix Size', fontsize=12)
plt.ylabel('Speedup (Relative to Pattern 1)', fontsize=12)
plt.title('Speedup Comparison of Different Access Patterns\n(Relative to Pattern 1 - ijk)', fontsize=14, fontweight='bold')
plt.xticks(x, df['MatrixSize'])
plt.axhline(y=1, color='red', linestyle='--', alpha=0.5, label='Baseline (Pattern 1)')
plt.legend(fontsize=10)
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig('matrix_mult_single_thread_speedup.png', dpi=300, bbox_inches='tight')
plt.show()

print("Performance Analysis Report:")
print("="*60)
for i, size in enumerate(df['MatrixSize']):
    print(f"\nMatrix Size: {size}x{size}")
    print("-"*40)
    for j, pattern in enumerate(patterns):
        time_val = df.loc[i, pattern]
        if j == 0:
            print(f"{pattern_names[j]}: {time_val:.4f}s (Baseline)")
        else:
            speedup = df.loc[i, 'Pattern1_ijk'] / time_val
            print(f"{pattern_names[j]}: {time_val:.4f}s (Speedup: {speedup:.2f}x)")