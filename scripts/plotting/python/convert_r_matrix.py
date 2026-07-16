"""Convert an R-format scoring matrix to the C++ nblast++ TSV format.

R format (space-separated, interval row/col labels):
    <empty>       (0,0.1]  (0.1,0.2]  ...
    (0.25,0.5]    0.766    0.895      ...

C++ format (tab-separated, upper-bound numeric labels):
    dist/angle\tcos_0.1\tcos_0.2\t...
    0.5\t0.766\t0.895\t...
"""

import re
import sys


def upper_bound(label: str) -> float:
    m = re.match(r'\(.*?,([0-9.]+)\]', label.strip())
    if m:
        return float(m.group(1))
    return float(label.strip())


def main():
    if len(sys.argv) not in (2, 3):
        print(f"usage: {sys.argv[0]} input.tsv [output.tsv]", file=sys.stderr)
        sys.exit(1)

    infile = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) == 3 else None

    with open(infile) as f:
        lines = [l.rstrip('\n') for l in f if l.strip()]

    angle_labels = [f"cos_{upper_bound(h)}" for h in lines[0].split()]
    header = ['dist/angle'] + angle_labels

    rows = []
    for line in lines[1:]:
        parts = line.split()
        dist = upper_bound(parts[0])
        values = [float(v) for v in parts[1:]]
        rows.append((dist, values))

    out = sys.stdout if outfile is None else open(outfile, 'w')
    try:
        out.write('\t'.join(header) + '\n')
        for dist, values in rows:
            out.write(f'{dist}\t' + '\t'.join(str(v) for v in values) + '\n')
    finally:
        if outfile is not None:
            out.close()


if __name__ == '__main__':
    main()
