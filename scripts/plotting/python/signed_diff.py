from file_io import load_scores
from statistics import mean, stdev


def signed_difference(first, second):
    res = []
    for key, value in first.items():
        if key in second:
            diff = value - second[key]
            res.append(diff)
    return res

if __name__ == "__main__":
    nbpp = load_scores("exp/query-out.tsv")
    nb = load_scores("data/costa-2016/fctraces20-nblast_scores.tsv")
    print(len(nb))
    sdiff = signed_difference(nbpp, nb)
    print(mean(sdiff))
    print(stdev(sdiff))