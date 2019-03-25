@page compress Using tudocomp for data compression

# Using tudocomp for data compression
The `tdc` executable of tudocomp is a viable tool for data compression like
`gzip` or `bzip2`.

For example, the file `file.txt` can be Huffman-encoded using the following
command line:
@code{.sh}
tdc -a "encode(huff)" -fo file.txt.huff file.txt
@endcode

The `-a` parameter is used to select the compression algorithm. For example,
a more sophisticated compression can be achieved using:
@code{.sh}
tdc -a "lzss_lcp(left(rice(5),rice(5),huff))" -0 file.txt -fo file.txt.comp
@endcode

This will compress the file using a LZSS parsing found via its LCP array, using
Rice codes for references and Huffman codes for literal factors.
More information on algorithm selection can be found on the @ref driver
page.

To decompress a file, use the `-d` parameter:
@code{.sh}
tdc -d file.txt.huff -fo file.txt.orig
@endcode

#### Sentinels.
You may encounter the following message for some algorithms:

```
Error: The input is missing a sentinel! If you are running tudocomp from the command line, pass the -0 parameter.
```

This means that the selected algorithm relies on the input having a sentinel
(essentially a zero-byte) at the end, which is typically the case if it uses
the input's suffix array. In this case, the `-0` parameter needs to be given,
which transforms the input accordingly.

Note that when decompressing,
`-0` must be given as well in order to undo the transformation. For example, to
restore the exact original input file for the LZSS example above, the following
command line must be used:
@code{.sh}
tdc -d -0 file.txt.comp -fo file.txt.orig
@endcode
