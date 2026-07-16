import argparse
import sys
import numpy as np
import matplotlib.pyplot as plt

from file_io import load_scores, load_matrix


def main():
    parser = argparse.ArgumentParser(
        description="Compare two files and generate scatter plot"
    )
    parser.add_argument("file1", help="First score file (X-axis)")
    parser.add_argument("file2", help="Second score file (Y-axis)")
    parser.add_argument(
        "-m",
        "--matrix",
        action="store_true",
        default=False,
        help="Parse files as matrix format instead of list format",
    )
    parser.add_argument("--label1", default="Known", help="Label for the X-axis")
    parser.add_argument("--label2", default="Experimental", help="Label for the Y-axis")
    parser.add_argument("--title", default="Score Comparison", help="Plot title")
    parser.add_argument("--out", default="scatter_plot.png", help="Output filename")

    # Appearance options
    parser.add_argument(
        "--alpha",
        type=float,
        default=0.5,
        help="Scatter point transparency (0.0 to 1.0)",
    )
    parser.add_argument("--size", type=float, default=8, help="Scatter point size")
    parser.add_argument("--color", default="#1f77b4", help="Scatter point color")
    parser.add_argument(
        "--grid", action="store_true", default=True, help="Enable background grid"
    )

    # Axis limits
    parser.add_argument(
        "--xlim", type=float, nargs=2, default=None, help="X-axis limits (min max)"
    )
    parser.add_argument(
        "--ylim", type=float, nargs=2, default=None, help="Y-axis limits (min max)"
    )

    args = parser.parse_args()

    if args.matrix:
        scores1 = load_matrix(args.file1)
        scores2 = load_matrix(args.file2)
    else:
        scores1 = load_scores(args.file1)
        scores2 = load_scores(args.file2)

    common_keys = sorted(set(scores1.keys()) & set(scores2.keys()))

    if not common_keys:
        print("No overlapping (query, target) pairs found.", file=sys.stderr)
        sys.exit(1)

    x = np.array([scores1[k] for k in common_keys])
    y = np.array([scores2[k] for k in common_keys])

    mean_x, mean_y = np.mean(x), np.mean(y)
    std_x, std_y = np.std(x), np.std(y)
    correlation_matrix = np.corrcoef(x, y)
    pearson_r = correlation_matrix[0, 1]

    print(f"Matched pairs: {len(common_keys)}")
    print(f"X ({args.label1}) - Mean: {mean_x:.6f}, Std: {std_x:.6f}")
    print(f"Y ({args.label2}) - Mean: {mean_y:.6f}, Std: {std_y:.6f}")
    print(f"Pearson r correlation: {pearson_r:.6f}")

    # Professional Plotting Style
    plt.style.use("seaborn-v0_8-whitegrid") if hasattr(
        plt.style, "library"
    ) and "seaborn-v0_8-whitegrid" in plt.style.library else plt.style.use("ggplot")

    fig, ax = plt.subplots(figsize=(8, 8))
    # ax.set_xscale('log')
    # ax.set_yscale('log')
    ax.scatter(x, y, s=args.size, alpha=args.alpha, color=args.color, edgecolors="none")

    min_val = min(x.min(), y.min())
    max_val = max(x.max(), y.max())

    # Identity line
    ax.plot(
        [min_val, max_val],
        [min_val, max_val],
        linestyle="--",
        color="gray",
        alpha=0.7,
        label="y = x",
    )

    if args.xlim:
        ax.set_xlim(args.xlim)
    if args.ylim:
        ax.set_ylim(args.ylim)

    ax.set_xlabel(args.label1, fontsize=12, fontweight="bold")
    ax.set_ylabel(args.label2, fontsize=12, fontweight="bold")
    ax.set_title(args.title, fontsize=14, fontweight="bold")

    if args.grid:
        ax.grid(True, linestyle=":", alpha=0.6)

    stats_text = (
        f"N = {len(common_keys):,}\n"
        f"Pearson r = {pearson_r:.4f}\n"
        f"Mean X = {mean_x:.4f}\n"
        f"Mean Y = {mean_y:.4f}"
    )

    # Place a clean text box with statistics
    props = dict(boxstyle="round", facecolor="white", alpha=0.9, edgecolor="lightgray")
    ax.text(
        0.05,
        0.95,
        stats_text,
        transform=ax.transAxes,
        verticalalignment="top",
        fontsize=10,
        bbox=props,
        family="monospace",
    )

    ax.legend(loc="lower right")

    # Outlier detection
    diffs = np.abs(y - x)
    threshold = np.mean(diffs) + (2 * np.std(diffs))
    outlier_indices = np.where(diffs > threshold)[0]

    print(f"\nFound {len(outlier_indices)} outliers (> 2 std devs from mean diff)")
    x_keys = []
    y_keys = []
    for idx in outlier_indices:
        n1, n2 = common_keys[idx]
        x_keys.append(n1)
        y_keys.append(n2)
    print(f"Unique Query Outliers: {len(set(x_keys))}")
    print(f"Unique Target Outliers: {len(set(y_keys))}")

    plt.tight_layout()
    plt.savefig(args.out, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"Scatterplot saved to: {args.out}")


if __name__ == "__main__":
    main()
