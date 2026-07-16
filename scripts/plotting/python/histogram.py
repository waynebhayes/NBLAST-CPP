import argparse
import numpy as np
import matplotlib.pyplot as plt

from file_io import load_scores

def main():
    parser = argparse.ArgumentParser(
        description="Plot overlaid histograms of two score files"
    )
    parser.add_argument("file1", help="First score file")
    parser.add_argument("file2", help="Second score file")
    parser.add_argument("--label1", default="Known")
    parser.add_argument("--label2", default="Experimental")
    parser.add_argument("--bins", type=int, default=50)
    parser.add_argument("--out", default="histogram_comparison.png")

    args = parser.parse_args()

    scores1 = load_scores(args.file1).values()
    scores2 = load_scores(args.file2).values()

    if not scores1 or not scores2:
        print("One of the files has no valid scores.")
        return

    print(f"{args.label1}: {len(scores1)} scores")
    print(f"{args.label2}: {len(scores2)} scores")

    plt.figure(figsize=(8, 6))

    plt.hist(scores1, bins=args.bins, alpha=0.5, density=True, label=args.label1)
    plt.hist(scores2, bins=args.bins, alpha=0.5, density=True, label=args.label2)

    plt.xlabel("Score")
    plt.ylabel("Density")
    plt.title("Score Distribution Comparison")
    plt.legend()

    plt.tight_layout()
    plt.savefig(args.out, dpi=300)
    plt.close()

    print(f"Histogram saved to: {args.out}")


if __name__ == "__main__":
    main()
