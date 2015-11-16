# Refactor

- somehow merge sa_compressor with concept of general/specific compressor
- change driver options to -a for "algorithm", remove existing c and e options

# Outstanding

- show amount of rules on stat page
- lz77, lz78, lzw impl
- Include esacomp java impl in that comparison tool
- Add some kind of coverage support
- Evaluate whether compressed Rule vector is improvement
- Experiment with making the tudomp library be usable as a streaming interface
- Generate nice massif visualized graphs
- Optimize DecodeBuffer
- bitvector einbinden
- 64 bits "rawphrase" auf N bits erweitern
- benchmark: % compression, zeit, speicher, dauer
- LZW: Lempel Ziv Welch
- factor out common code between code{0,1,2}.cpp
- Add testsuite for driver
- force make options to be registerable
- mmap input file

# Revisit at later point

- Optimize Rulelists
- Automatically choose threshold

# Waiting on more clarification

- Support for new BAs
- Make repo more accessible for other developers

# Done

- make threshold settable
- Add and maintain docs on actual source files
- Automatically generate html docs
- Change sdsl dependency to be automatically downloaded
- Add comparison tool

# Motivation

Ziel ist es, ein Kompressionsframework zu erschaffen, das modular aufgebaut ist.
Module sollen sein:
  * Regelwerk
    - LZ77, LZ78, ESAComp, Tunstall, ESP
  * Codierer
    - Dictionary, Grammar, InPlace (ESAComp)
  * Decodierer

Eingaben:
  * Texte
  * Bit-Sequenzen

Die komprimierten Bit-Sequenzen sollen dann mit der SDSL Bibliothek bedienbar sein
(z.B. einen Wavelet-Tree auf einer komprimierten Bit-Sequenz;
Der Code dafür ist in hocbits bereits vorhanden)
