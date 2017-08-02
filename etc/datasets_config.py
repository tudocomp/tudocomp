small_sizes = [200, 100, 50, 10, 1]

# HashTag Datasets, see http://acube.di.unipi.it/datasets/
URL = "http://acube.di.unipi.it/repo/he-graph"
download_and_extract("hashtag", small_sizes, [
    URL + "/graph/he-graph.txt.gz",
    URL + "/relatedness/pairs.tsv",
    URL + "/classification/crowdflower.tsv",
])

DACIT_URL = "dacit.cs.uni-dortmund.de/datasets"

# ASCII-only, no inner null commoncrawl.org extract
download_and_extract("cc", small_sizes, [
    DACIT_URL + "/commoncrawl.ascii"
])

# Pizza&Chili Corpus text collection
download_and_extract("pc", small_sizes, [
    DACIT_URL + "/code/sources.gz",
    DACIT_URL + "/music/pitches.gz",
    DACIT_URL + "/protein/proteins.gz",
    DACIT_URL + "/dna/dna.gz",
    DACIT_URL + "/nlang/english.gz",
    DACIT_URL + "/xml/dblp.xml.gz",
])
download_and_extract("pcr", small_sizes, [
    DACIT_URL + "/pc-pseudo-real/english.001.2.7z",
    DACIT_URL + "/pc-pseudo-real/dblp.xml.00001.1.7z",
    DACIT_URL + "/pc-pseudo-real/dblp.xml.00001.2.7z",
    DACIT_URL + "/pc-pseudo-real/dblp.xml.0001.1.7z",
    DACIT_URL + "/pc-pseudo-real/dblp.xml.0001.2.7z",
    DACIT_URL + "/pc-pseudo-real/dna.001.1.7z",
    DACIT_URL + "/pc-pseudo-real/english.001.2.7z",
    DACIT_URL + "/pc-pseudo-real/proteins.001.1.7z",
    DACIT_URL + "/pc-pseudo-real/sources.001.2.7z",
    DACIT_URL + "/pc-real/cere.7z",
    DACIT_URL + "/pc-real/coreutils.7z",
    DACIT_URL + "/pc-real/einstein.de.txt.7z",
    DACIT_URL + "/pc-real/einstein.en.txt.7z",
    DACIT_URL + "/pc-real/Escherichia_Coli.7z",
    DACIT_URL + "/pc-real/influenza.7z",
    DACIT_URL + "/pc-real/kernel.7z",
    DACIT_URL + "/pc-real/para.7z",
    DACIT_URL + "/pc-real/world_leaders.7z",
    "http://pizzachili.dcc.uchile.cl/repcorpus/artificial/fib41.7z",
    "http://pizzachili.dcc.uchile.cl/repcorpus/artificial/rs.13.7z",
    "http://pizzachili.dcc.uchile.cl/repcorpus/artificial/tm29.7z",
])

# TAGME Datasets, see http://acube.di.unipi.it/datasets/
download_and_extract("tagme", small_sizes, [
    DACIT_URL + "/wiki-annot30.gz",
    DACIT_URL + "/wiki-disamb30.gz",
])

# TagMyNews Datasets, see http://acube.di.unipi.it/datasets/
download_and_extract("tagmynews", small_sizes, [
    DACIT_URL + "/news.gz",
    DACIT_URL + "/data-web-snippets.tar.gz",
])
