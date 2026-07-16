def load_matrix(filename: str) -> dict[tuple[float, float], float]:
    matrix_values = {}
    with open(filename, "r") as f:
        header = f.readline().strip().split()
        for i, line in enumerate(f):
            if not line.strip():
                continue
            fields = line.strip().split()
            for j in range(1, len(fields)):
                value = float(fields[j])
                matrix_values[(i, j)] = value
    return matrix_values

def load_scores(filename: str) -> dict[tuple[str, str], float]:
    scores = {}

    with open(filename, "r") as f:
        header = f.readline().strip().split()

        score_idx = header.index("score") if "score" in header else 2

        for line in f:
            if not line.strip():
                continue

            fields = line.strip().split()
            if len(fields) <= score_idx:
                continue

            query = fields[0]
            target = fields[1]

            try:
                score = float(fields[score_idx])
                scores[(query, target)] = score
                scores[(target, query)] = score
            except ValueError:
                continue

    return scores
