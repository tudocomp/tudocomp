sizes = [1, 10240]

DACIT_URL = "dacit.cs.uni-dortmund.de/datasets"

download_and_extract("large", sizes, [
    DACIT_URL + "/commoncrawl_10240.ascii",

    # http://acube.di.unipi.it/rlzap-dataset/
    # (array of 32bit values)
    DACIT_URL + "/rlzap_dlcp_female",

    # http://www.internationalgenome.org/data-portal/sample/NA19000
    DACIT_URL + "/1000Genome_JPT_NA19000_SRR099528_1.fastq.filtered4_no_N.dna",

    # https://panthema.net/2012/1119-eSAIS-Inducing-Suffix-and-LCP-Arrays-in-External-Memory/
    # Gutenberg Concatenation from September 2012
    DACIT_URL + "/gutenberg-201209.24090588160.xz",
])
