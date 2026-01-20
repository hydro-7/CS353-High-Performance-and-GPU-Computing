import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv("results.csv")

# -------------------------------
# 1) Time vs Threads (for each N)
# -------------------------------
for N in sorted(df["N"].unique()):
    sub = df[df["N"] == N]

    plt.figure(figsize=(8, 5))
    for method in sub["method"].unique():
        m = sub[sub["method"] == method]
        plt.plot(
            m["threads"],
            m["time"],
            marker="o",
            linewidth=2,
            label=method
        )

    plt.title(f"Matrix Multiplication Performance ({N} x {N})")
    plt.xlabel("Number of Threads")
    plt.ylabel("Execution Time (seconds)")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"matrix_size_{N}.png")
    plt.show()

# -----------------------------------
# 2) Time vs Matrix Size (for threads)
# -----------------------------------
for t in sorted(df["threads"].unique()):
    sub = df[df["threads"] == t]

    plt.figure(figsize=(8, 5))
    for method in sub["method"].unique():
        m = sub[sub["method"] == method]
        plt.plot(
            m["N"],
            m["time"],
            marker="o",
            linewidth=2,
            label=method
        )

    plt.title(f"Matrix Multiplication Scaling (Threads = {t})")
    plt.xlabel("Matrix Size (N x N)")
    plt.ylabel("Execution Time (seconds)")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"matrix_threads_{t}.png")
    plt.show()
