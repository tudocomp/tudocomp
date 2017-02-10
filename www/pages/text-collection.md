# Text Collection

For testing and benchmarking purposes, we have compiled a text collection that
is described on this page.

*tudocomp* provides a utility to automatically download or generate this
collection. For convenience, it also creates several prefixes of the included
texts (200 MiB, 100 MiB 50 MiB, ...).

The collection includes the [`tagme`](http://acube.di.unipi.it/tagme-dataset)
and [`hashtag`](http://acube.di.unipi.it/hashtag-datasets) datasets from the
[Lab of Advanced Algorithms and Applications](http://acube.di.unipi.it/datasets),
as well as the [Pizza&Chili Corpus](pizzachili.dcc.uchile.cl) (identified by
the prefixes `pc` and `pcr`).

Furthermore, we generate the following collections:

 - `commoncrawl` - composition of a random subset of
   [common crawl](http://commoncrawl.org), containing only the plain text
   (i.e., excluding header and markup) of websites in ASCII format.
 - `wiki-all-vital` - plain text composition of about 10,000 of the
    [most vital Wikipedia articles](https://en.wikipedia.org/wiki/Wikipedia:Vital_articles/Expanded).

The following table contains information relevant for text compression on each
of the included texts:

| Text              | σ   | max lcp   | avg lcp | bwt runs  | z       | max LZ77 factor | H~0~ | H~1~ | H~2~ | H~3~
| ---               | --- | ---       | ---     | ---       | ---     | ---             | ---  | ---  | ---  | ---
| `hashtag        ` | 179 | 54,075    | 84      | 63,014K   | 13,721K | 54,056          | 4.59 | 3.06 | 2.69 | 2.46
| `pc-dblp.xml    ` | 97  | 1,084     | 44      | 29,585K   | 7,035K  | 1,060           | 5.26 | 3.48 | 2.17 | 1.43
| `pc-dna         ` | 17  | 97,979    | 60      | 128,863K  | 13,970K | 97,966          | 1.97 | 1.93 | 1.92 | 1.92
| `pc-english     ` | 226 | 987,770   | 9390    | 72,032K   | 13,971K | 987,766         | 4.52 | 3.62 | 2.95 | 2.42
| `pc-proteins    ` | 26  | 45,704    | 278     | 108,459K  | 20,875K | 45,703          | 4.20 | 4.18 | 4.16 | 4.07
| `pcr-cere       ` | 6   | 175,655   | 3541    | 10,422K   | 1,447K  | 175,643         | 2.19 | 1.81 | 1.81 | 1.80
| `pcr-einstein.en` | 125 | 935,920   | 45983   | 153K      | 496K    | 906,995         | 4.92 | 3.66 | 2.61 | 1.63
| `pcr-kernel     ` | 161 | 2,755,550 | 149872  | 2,718K    | 775K    | 2,755,550       | 5.38 | 4.03 | 2.93 | 2.05
| `pcr-para       ` | 6   | 72,544    | 2268    | 13,576K   | 1,927K  | 70,680          | 2.12 | 1.88 | 1.88 | 1.87
| `pc-sources     ` | 231 | 307,871   | 373     | 47,651K   | 11,542K | 307,871         | 5.47 | 4.08 | 3.10 | 2.34
| `tagme          ` | 206 | 1,281     | 26      | 65,195K   | 13,841K | 1,279           | 4.90 | 3.77 | 3.20 | 2.60
| `wiki-all-vital ` | 205 | 8,607     | 15      | 80,609K   | 16,274K | 8,607           | 4.56 | 3.62 | 3.03 | 2.45
| `commoncrawl    ` | 115 | 246,266   | 1727    | 45,899K   | 10,791K | 246,266         | 5.37 | 4.30 | 3.55 | 2.78

*Legend*:

 - *σ* is the alphabet size, i.e., the amount of distinct symbols occuring in
   the text.
 - *max lcp* is the maximum value in the text's longest common prefix array.
 - *avg lcp* is the average of all longest common prefix array values.
 - *bwt runs* is the number of runs in the Burrows-Wheeler transform of the text
   (i.e., how often `BWT[i] != BWT[i-1]` occurs).
 - *z* is the number of LZ77 factors (n window) for the text.
 - *max LZ77 factor* is the length of the longest LZ77 factor (no window).
 - *H~k~* is the k-th order entropy of the text.

