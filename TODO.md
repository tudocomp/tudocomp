# Refactor

- move lz77rule impl to subproject
- create lz78rule impl
- move lz78 rule impl
- somehow merge sa_compressor with concept of general/specific compressor

# Outstanding

for lz77, there are sevaral types of rulesets and compression algorithms:

LZ77 Original
- uses rule (source,length,new-character) - the new-character is similar to LZ78

LZSS (https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski)
- uses rule (source,length)
- LZSS does not create a rule if it costs more than the original

- rename lz77rule::Rules::nums into lengths or add a comment
- test on debian stable
  - test c++14 support on debian stable
- template-generalize the rule buffers
  - make growable bit vector use one allocation by distributing bits over N indices
  - make bit-and-size grow more exact by manually expanding bits
  - grow to absolute byte-count increments, rather than relative to last value
- integrate second BA
  => wait till it compiles
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
- make compression algorithms chainable
  - normalize input/output handles
  - include notion of algorithm header

# Revisit at later point

- Optimize Rulelists
- Automatically choose threshold

# Waiting on more clarification

- Support for new BAs
- Make repo more accessible for other developers

# Done

- make algo register a global function that recusively calls register methods
- show amount of rules on stat page
- change driver options to -a for "algorithm", remove existing c and e options
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
