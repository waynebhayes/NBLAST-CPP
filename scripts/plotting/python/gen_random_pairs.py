#!/usr/bin/env python3
"""Generate random pairs of neurons from a directory of .swc files."""

import argparse
import os
import random

def main():
    parser = argparse.ArgumentParser(description="Generate random neuron pairs")
    parser.add_argument("--dir", default=os.path.expanduser("~/research/FlyCircuit"),
                        help="Directory containing .swc neuron files")
    parser.add_argument("--num-pairs", type=int, default=10000,
                        help="Number of random pairs to generate")
    parser.add_argument("--output", default="random_pairs.tsv",
                        help="Output TSV file")
    parser.add_argument("--seed", type=int, default=None,
                        help="Random seed for reproducibility")
    args = parser.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    neurons = sorted(
        os.path.splitext(f)[0]
        for f in os.listdir(args.dir)
        if f.endswith(".swc")
    )

    n = len(neurons)
    if n < 2:
        raise SystemExit(f"Error: need at least 2 neurons, found {n}")

    print(f"Found {n} neurons in {args.dir}")
    print(f"Generating {args.num_pairs} random pairs -> {args.output}")

    with open(args.output, "w") as f:
        for _ in range(args.num_pairs):
            i, j = random.sample(range(n), 2)
            f.write(f"{neurons[i]}\t{neurons[j]}\n")

    print(f"Done. Wrote {args.num_pairs} pairs to {args.output}")

if __name__ == "__main__":
    main()
