# Project Ideas - Google Summer of Code 2017

We maintain a list of medium size projects on this page.

These projects are self-contained and consist of ideas for enhancements and
novel compression algorithms for *tudocomp*. They range from projects targeted
at "nice-to-heave" features to projects related to scientific research in the
field of data compression.

For all these projects, participating students should be interested in data
compression in general, as well as in at least in one of the following fields
in particular: 

 - low-level programming (with C++),
 - string data structures,
 - algorithm engineering.

We encourage everyone (not just students) to take part in the development of the
*tudocomp* framework by participating at one of the listed projects.

Please have a look at the [tudocomp homepage](http://tudocomp.org) to get a
glimpse on what this framework is all about. *tudocomp*'s source code is
publicly available at [github](https://github.com/tudocomp/tudocomp).

## Overview

Here is an overview over our project ideas with a basic categorization. If you
are interested in any of the following projects, please also read the
[General Information](#general-information) section.

 1. [Review of Coding Strategies](#review-of-coding-strategies) (Encoding)
 1. [The AreaComp Compression Algorithm](#the-areacomp-compression-algorithm)
    (Compression)
 1. [Clever Tie Breaking for lcpcomp](#clever-tie-breaking-for-lcpcomp)
    (Algorithm Engineering)
 1. [k-maxsat for lcpcomp](#k-maxsat-for-lcpcomp) (Algorithm Engineering)
 1. [Efficient Integer Coders](#efficient-integer-coders) (Encoding)
 1. [LZ78 with a Compact Hash Table](#lz78-with-a-compact-hash-table)
    (Compression, Hashing)
 1. [Web Application for Visual String Analysis](#web-application-for-visual-string-analysis)
    (Visualization, Web Development)
 1. [7zip-compatible Output Format](#7zip-compatible-output-format)
    (Interoperability)
 1. [Graphical User Interface](#graphical-user-interface) (GUI Development)

## General Information

Participating as a *tudocomp* contributor involves the following:

 1. Get familiar with `git` in case you are not yet.
 1. Fork *tudocomp* on github.
 1. Write, evaluate and comment your code and get back to the *tudocomp*
    community when you are ready.
 1. Keep a diary documenting your project's evolution and progress and what
    issues you faced, solved or could not solve. This will also help yourself
    evaluating your project plan (see below).
 1. After our green light, file a pull request of your stable achievements.

Under *stable*, we understand that your code is

 - compilable,
 - well commented, to the extent that the code is easily understandable and
 - compliant to our coding standards.

### Communication

while you are working on your project, a steady communication between you and
the community is desirable. This is in order to give you the best possible
support and keep you up to date about potential changes of the framework's
architecture (e.g. due to new features being implemented or larger issues being
fixed).

We use the following forms of communication:

 - Our [mailing list](https://mailman.tu-dortmund.de/mailman/listinfo/tudocomp.cs)
 - The [issue tracker](https://github.com/tudocomp/tudocomp/issues) at github
 - The `#tudocomp` IRC channel at [euIRC](https://www.euirc.net/en/)
 - Regular exchange of e-mails directly with your mentors to discuss the state
   of the project
 - Further communication channels, e.g. instant messengers like Skype - ask your
   mentors about what suits everyone best

In general, do not hesitate to contact the community about any ideas or
concerns that you may have.

We invite anybody to join and start working on their own implementation of
compression algorithms.

### GSoC Timeline

The [general GSoC 2017 timeline](https://developers.google.com/open-source/gsoc/timeline)
has important _hard_ deadlines for all projects.

### Project Plan

Project planning is necessary to structure how and when parts of a project have
to be done. The first step of starting a project is to set up a detailed project
plan. We encourage to realize this plan using issue trackers, i.e., by
expressing single tasks as tickets, bundled in milestones. This can easily be
transferred into a Gantt diagram.

The project plan includes intermediate and secondary goals and deliveries with
clear deadlines, as well as reasonable goals for mid-term evaluation. Remember
to add slack times for the time ranges in which you might not be available.

While working on a project, the project plan should indicate where you are at
the moment, i.e., what has and has not yet been done at any point of time. In
case  your time management drifts apart from what you expected from the project
plan, you should get in touch with your mentors.

Keep a diary, documenting your project's evolution and progress and what
issues you faced, solved or could not solve, as well as what tasks you were able
to finish in time and what is behind schedule.

## GSoC 2017 Project Ideas

The following is a collection of our project ideas that we propose for
GSoC 2017. If you have further project ideas and/or want to discuss some of our
proposed projects please contact us via
[e-mail](mailto:tudocomp.cs@lists.tu-dortmund.de). Feel free to subscribe to our
[mailing list](https://mailman.tu-dortmund.de/mailman/listinfo/tudocomp.cs) and
follow the discussions to see what is going on. 

### 1. Review of Coding Strategies

*tudocomp* currently provides only a canonical Huffman coder and a customized
static low entropy coder.

Novel coding algorithms provide better precision than a Huffman coder and are
faster than arithmetic coders. It is interesting to integrate general codings
like ASM or FSE or specialized codings like ETDC into *tudocomp* and evaluate
all codings with respect to their compression ratio and coding/decoding time.

*References*:

 - [ETDC for natural language text compression](https://www.dcc.uchile.cl/~gnavarro/ps/ir06.pdf)
 - [Asymmetric Numeral Systems](https://arxiv.org/pdf/1311.2540.pdf)
 - [FSE Encoder](https://github.com/Cyan4973/FiniteStateEntropy)

*Category*: Encoding

### 2. The AreaComp Compression Algorithm

The idea of AreaComp is to substitute frequent large substrings of a text.
We search for the substring that maximizes the value of a cost function. The
cost function weights the number of occurrences and the length of a substring,
e.g., the multiplication of the length and the number of occurrences of a
substring is a natural choice.

Given the suffix array and the longest common prefix array, we can find the
number of occurrences of a substring in the text by looking at both arrays.

A naive approach would be to store all substrings of a certain length occurring
at least twice in a priority queue, with its cost function value as its key.
We pop the topmost element (i.e., the best substring w.r.t.\ the cost function)
off from the heap, substitute its occurrences, and update the suffix array and
the longest common prefix array. There is a similar method called
"greedy off-line textual substitution" that considers all non-overlapping
occurrences.

*Category*: Compression

*References*:

 - [Off-line compression by greedy textual substitution](http://ieeexplore.ieee.org/iel5/5/19320/00892709.pdf)
 - [Data structures and algorithms for the string statistics problem](http://link.springer.com/article/10.1007/BF01955046)

### 3. Clever Tie Breaking for lcpcomp

Our compressor [lcpcomp](https://github.com/tudocomp/tudocomp/blob/public/include/tudocomp/compressors/LCPCompressor.hpp)
(implemented in *tudocomp*) is a  longest-first greedy compression algorithm.

Given multiple longest substrings, there is no strict tie breaking rule that
states which of the substring to select for substitution. The focus of this
project is to enhance the lcpcomp compressors with a heuristic for which
substring lcpcomp should choose. The heuristic can be based on the selection of
a tie breaking rule with the best expectation in terms of compression ratio or
decompression speed.

*Category*: Algorithm Engineering

*References*:

 - This problem is similar to a [semi-greedy variant of LZ77](http://link.springer.com/chapter/10.1007/978-3-642-82456-2_11).

### 4. k-maxsat for lcpcomp

Decompressing an [lcpcomp](https://github.com/tudocomp/tudocomp/blob/public/include/tudocomp/compressors/LCPCompressor.hpp)
compressed file is a heavy task with respect to time. That is because references
of lcpcomp can be nested, i.e., a reference can refer to a substring that got
replaced with another reference.

The nesting of references can form long dependency chains that need to be
resolved before the actual decompression can take place. This project focuses
on a modification of the compression strategy, where we want to check whether it
is possible to cirumvent the production of long dependency chaines.

It can be shown that this problem is related to the k-maxsat problem.
The aim of this project is to devise an approximation algorithm for the k-maxsat
problem to prevent long dependency chains.

*Category*: Algorithm Engineering

*References*:

 - Mayr, Ernst W., Prömel, Hans Jürgen, Steger, Angelika:
   "Lectures on Proof Verification and Approximation Algorithms",
   the MaxEkSat-Problem


### 5. Efficient Integer Coders

The aim of this project is to implement and evaluate a fast Fibonacci encoding
algorithm.

Fibonacci coding is a universal code that represents integers succinctly. The
coding splits an integer into summands that are Fibonacci words. Although the
coding achieves a very compact representation, its decoding is slow. In this
project, the encoding shall be implemented an benchmarked in *tudocomp*, i-e.,
measure its speed and compare to currently avaiable Fibonacci coders.

*Category*: Encodings

*References*:

 - [Fast Fibonacci Encoding Algorithm](http://ceur-ws.org/Vol-567/paper14.pdf)
 - [Fibonacci Coder of the SDSL](https://github.com/simongog/sdsl-lite/tree/master/include/sdsl/coder_fibonacci.hpp)


### 6. LZ78 with a Compact Hash Table

Our [LZ78 compressor](https://github.com/tudocomp/tudocomp/blob/public/include/tudocomp/compressors/LZ78Compressor.hpp) 
can utilize different Lempel-Ziv-78 tries, e.g. a binary trie, a ternary trie,
or a trie based on a hash table. The latter is the fastest, but heaviest trie
implementation.

In the light of compact hash tables, we wonder whether we can drop the memory
footprint of hash table implementations while still being very fast. The goal
of this project is to research on this topic and develop a new, more efficient
hash table based LZ78 trie.

 - [Don't Thrash: How to Cache Your Hash on Flash](https://arxiv.org/abs/1208.0290)

*Category*: Compression, Hashing

### 7. Web Application for Visual String Analysis

While working with lossless compression algorithms on texts, we often experience
the lack of tools that visualize text index data structures.

There are some tools that provide limited insight to some data structures.
However, there is no powerful and easy-to-use tool that covers a majority of the
most frequently used data structures.

The aim of this project is to produce a web application (preferably based
on JavaScript) that interactively visualizes the most commonly used data
structures like suffix arrays, longest common prefix arrays, etc., with respect
to text compression.

*References*:

 - [Shinohara's Online Encyclopedia of Strings](http://www.shino.ecei.tohoku.ac.jp/stringology/)
 - [Strinalze - Analyze a string or a sequence of generated strings](https://github.com/koeppl/strinalyze)

*Category*: Visualization, Web Development

### 8. 7zip-compatible Output Format

To improve the usability of the *tudocomp* framework, we want the tudocomp
output format to become compatible to 7zip. The `7z` format supports various
compression techniques due to a versatile header, describing the exact used
compression technique.

This project's aim is to adapt the 7z format for the *tudocomp* command line
tool.

*Category*: Interoperability

*References*:

 - [7zip](http://www.7-zip.org)


### 9. Graphical User Interface

The *tudocomp* framework provides only a command-line tool as an interface to
the end user. An graphical user interface would benefit the project for
addressing users with antipathies to command-line tools.

The GUI should provide the selection of multiple files/directories and an easy
way to assemble a custom compression pipeline using what is available in
*tudocomp*. It should be as platform independent as possible and usable on any
platform on which *tudocomp* supports.

The GUI can be developed, for instance, using a framework like Qt.

*Category*: GUI Development

<!---
### Try to detect text type, and use appropriate compressor.
Modify existing 
Neutronal Network Detetects 
*Category* Machine Learning

Target field of application: Machine Learning
### Survey implementation of different LCP/SA construction algorithms

*Category* Algorithm Engineering

### Compressed Index with Repair

Sakamoto is working on an online index data structure. Online means that characters can be appended.
He uses ESP as grammar.
The grammar tree is stored in post-order to allow new elements on the right side.
Since the tree is a full binary tree, it can be stored in n bits if it contains n nodes (see technical report of Sadakane).
The DCC paper 2016 from Kida introduces an online Re-Pair-based grammar compressor.
It should be feasible to exchange ESP with the online Re-Pair-variant.

*Category*: Compressed Indices

### Serialization of Indices
A majority of compression algorithms use data structures that need a lot of time to process.
Add a facility to build them in a preliminary step, such that they can be loaded when the actual compression has to be done.
Some data structures are only needed for a linear scan such that they can be streamed from disk.

*Category*: External Memory Algorithms

--->
