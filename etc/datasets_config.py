# HashTag Datasets, see http://acube.di.unipi.it/datasets/
small_sizes = [200, 100, 50, 10, 1]

URL = "http://acube.di.unipi.it/repo/he-graph"
URLS = [
    URL + "/graph/he-graph.txt.gz",
    URL + "/relatedness/pairs.tsv",
    URL + "/classification/crowdflower.tsv",
]

download_and_extract("hashtag", URLS, small_sizes)
