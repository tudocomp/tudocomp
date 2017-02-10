# Text Collections

tudocomp provides a tool that automatically downloads or generates all below described text collections.
Of each collection, the tool generates prefixes of the size 1MiB, 10MiB, 50MiB, and 200MiB.

We use the following text collections from the [Lab of Advanced Algorithms and Applications Website](http://acube.di.unipi.it/datasets):

 - `tagme`: [http://acube.di.unipi.it/tagme-dataset](http://acube.di.unipi.it/tagme-dataset)
 - `hashtag`: [http://acube.di.unipi.it/hashtag-datasets](http://acube.di.unipi.it/hashtag-datasets)

Text collections with a prefix `pc` or `pcr` belong to the [Pizza&Chili Corpus](pizzachili.dcc.uchile.cl).

Tudocomp can automatically generate the following collections:

 - `commoncrawl`: composed of a random subset of [common crawl](http://commoncrawl.org).
  this subset contains only the plain texts (i.e., without header and HTML tags) of web sites with ASCII characters.
 - `wiki-all-vital`: consists of the plain text of about 10000 [most vital Wikipedia articles](https://en.wikipedia.org/wiki/Wikipedia:Vital_articles/Expanded)

The table below shows important key figures of all datasets.


| collection        | σ   | max lcp | avg lcp | bwt-runs | z      | max LZ77 factor | H_0  | H_1  | H_2  | H_3
| ---               | --- | ---     | ---     | ---      | ---    | ---             | ---  | ---  | ---  | ---
| `hashtag        ` | 179 | 54075   | 84      | 63014K   | 13721K | 54056           | 4.59 | 3.06 | 2.69 | 2.46
| `pc-dblp.xml    ` | 97  | 1084    | 44      | 29585K   | 7035K  | 1060            | 5.26 | 3.48 | 2.17 | 1.43
| `pc-dna         ` | 17  | 97979   | 60      | 128863K  | 13970K | 97966           | 1.97 | 1.93 | 1.92 | 1.92
| `pc-english     ` | 226 | 987770  | 9390    | 72032K   | 13971K | 987766          | 4.52 | 3.62 | 2.95 | 2.42
| `pc-proteins    ` | 26  | 45704   | 278     | 108459K  | 20875K | 45703           | 4.20 | 4.18 | 4.16 | 4.07
| `pcr-cere       ` | 6   | 175655  | 3541    | 10422K   | 1447K  | 175643          | 2.19 | 1.81 | 1.81 | 1.80
| `pcr-einstein.en` | 125 | 935920  | 45983   | 153K     | 496K   | 906995          | 4.92 | 3.66 | 2.61 | 1.63
| `pcr-kernel     ` | 161 | 2755550 | 149872  | 2718K    | 775K   | 2755550         | 5.38 | 4.03 | 2.93 | 2.05
| `pcr-para       ` | 6   | 72544   | 2268    | 13576K   | 1927K  | 70680           | 2.12 | 1.88 | 1.88 | 1.87
| `pc-sources     ` | 231 | 307871  | 373     | 47651K   | 11542K | 307871          | 5.47 | 4.08 | 3.10 | 2.34
| `tagme          ` | 206 | 1281    | 26      | 65195K   | 13841K | 1279            | 4.90 | 3.77 | 3.20 | 2.60
| `wiki-all-vital ` | 205 | 8607    | 15      | 80609K   | 16274K | 8607            | 4.56 | 3.62 | 3.03 | 2.45
| `commoncrawl    ` | 115 | 246266  | 1727    | 45899K   | 10791K | 246266          | 5.37 | 4.30 | 3.55 | 2.78

where

 - σ is the alphabet size
 - max lcp is the maximum LCP value
 - avg lcp is the average of all LCP values
 - bwt-runs is the number of runs in the BWT (counting how often BWT[i] != BWT[i-1] occurs)
 - z is the number of LZ77 factors (without window)
 - H_k is the k-th order entropy
