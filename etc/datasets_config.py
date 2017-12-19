small_sizes = [200, 100, 50, 10, 1]

# HashTag Datasets, see http://acube.di.unipi.it/datasets/
ACUBE_URL = "http://acube.di.unipi.it/repo/he-graph"
download_and_extract("hashtag", small_sizes, [
    ACUBE_URL + "/graph/he-graph.txt.gz",
    ACUBE_URL + "/relatedness/pairs.tsv",
    ACUBE_URL + "/classification/crowdflower.tsv",
])

TDC_URL = "dolomit.cs.uni-dortmund.de/"

# ASCII-only, no inner null commoncrawl.org extract
download_and_extract("cc", small_sizes, [
    TDC_URL + "/commoncrawl.ascii"
])

# Pizza&Chili Corpus text collection
download_and_extract("pc", small_sizes, [
    TDC_URL + "/code/sources.gz",
    TDC_URL + "/music/pitches.gz",
    TDC_URL + "/protein/proteins.gz",
    TDC_URL + "/dna/dna.gz",
    TDC_URL + "/nlang/english.gz",
    TDC_URL + "/xml/dblp.xml.gz",
])
download_and_extract("pcr", small_sizes, [
    TDC_URL + "/pc-pseudo-real/english.001.2.7z",
    TDC_URL + "/pc-pseudo-real/dblp.xml.00001.1.7z",
    TDC_URL + "/pc-pseudo-real/dblp.xml.00001.2.7z",
    TDC_URL + "/pc-pseudo-real/dblp.xml.0001.1.7z",
    TDC_URL + "/pc-pseudo-real/dblp.xml.0001.2.7z",
    TDC_URL + "/pc-pseudo-real/dna.001.1.7z",
    TDC_URL + "/pc-pseudo-real/english.001.2.7z",
    TDC_URL + "/pc-pseudo-real/proteins.001.1.7z",
    TDC_URL + "/pc-pseudo-real/sources.001.2.7z",
    TDC_URL + "/pc-real/cere.7z",
    TDC_URL + "/pc-real/coreutils.7z",
    TDC_URL + "/pc-real/einstein.de.txt.7z",
    TDC_URL + "/pc-real/einstein.en.txt.7z",
    TDC_URL + "/pc-real/Escherichia_Coli.7z",
    TDC_URL + "/pc-real/influenza.7z",
    TDC_URL + "/pc-real/kernel.7z",
    TDC_URL + "/pc-real/para.7z",
    TDC_URL + "/pc-real/world_leaders.7z",
    "http://pizzachili.dcc.uchile.cl/repcorpus/artificial/fib41.7z",
    "http://pizzachili.dcc.uchile.cl/repcorpus/artificial/rs.13.7z",
    "http://pizzachili.dcc.uchile.cl/repcorpus/artificial/tm29.7z",
])

# TAGME Datasets, see http://acube.di.unipi.it/datasets/
download_and_extract("tagme", small_sizes, [
    TDC_URL + "/wiki-annot30.gz",
    TDC_URL + "/wiki-disamb30.gz",
])

# TagMyNews Datasets, see http://acube.di.unipi.it/datasets/
download_and_extract("tagmynews", small_sizes, [
    TDC_URL + "/news.gz",
    TDC_URL + "/data-web-snippets.tar.gz",
])

# ASCII-only Wikipedia excerpt from the "10.000 most vital articles" project
download_and_extract("wiki", small_sizes, [
    TDC_URL + "/all_vital.txt.gz",
])
