# Project Ideas List - Google Summer of Code 2017

We maintain an up-to-date list of medium size projects at this page.
The projects are self-contained, and consist of enhancements and improvements.
Our project ideas range from projects targeted at nice-to-heave features to projects related to scientific research in the field of data compression.
For all these projects, participating students should be interested in data compression in general, and in at least in one of the following fields in particular: 
low-level programming, string data structures, algorithm engineering.
We welcome everybody (not just students) to take part in the tudocomp framework by participating at one of the listed projects.

Please have a look at the [tudocomp homepage](http://tudocomp.org) to get a glimpse on what is this framework all about.
Also, tudocomp's sourcecode is publicly available at [github](https://github.com/tudocomp/tudocomp).

## General Info

For participating as a tudocomp contributor, the following steps should be made:

 - Get familiar with git
 - Create your own github branch
 - Program, evaluate and comment your code in your branch
 - On an overall agreement of the tudocomp community, push your stable achievements back to the main repository by pull requests. 

Under *stable* we understand that the code is

 - compilable
 - well commented to the extend that the code is easily understandable
 - compliant to our coding standards

Communication during coding is necessary:

- the architecture may be subject to changes
- there may be bugs or strange behavior in our project of which we either are unaware of, or which nobody has yet fixed

For communication:

- use the ticket system
- send an email on a regular basis for discussions
- keep an up-to-date diary about the current state of your project. 
  The dairy should show how the project evolves and progresses. 
- give regularly feedback to the community over the [mailing list](https://mailman.tu-dortmund.de/mailman/listinfo/tudocomp.cs)
- if needed, ask your mentors about further communication channels, e.g., instant messengers like Skype.

We advocate anybody to fork the project, and start their own implementation of compression algorithms.

## GSoC Timeline

The [general GSoC 2017 timeline](https://developers.google.com/open-source/gsoc/timeline) has important _hard_ deadlines for all projects.

## Project Plan
Project planning is necessary to structure how and when which parts of a project have to be done.
The first step of starting a project is to set up a detailed project plan.
The plan can be realized with tickets bundled in milestones, and a Gantt diagram.
The project plan includes intermediate and secondary goals and deliveries with clear deadlines, 
as well as reasonable goals for midterm evaluation. 
Add slack times for the time ranges were you might not be available.
While working on a project, the project plan should indicate where you are at the moment, i.e., 
what has (not yet) been done at any point of time.
If your time management drifts apart from what you expected by the project planning, you should get in touch with your mentors.
Your diary has to be updated on a regular basis (e.g., what problems have occurred / have been resolved, what is behind of schedule).


## GSoc 2017 Ideas

The following collection consists of our project ideas we propose for GSoC 2017.
If you have further project ideas and/or want to discuss some of our proposed projects please contact us by email (tudocomp.cs@lists.tu-dortmund.de). 
You can also subscribe and follow the discussions on the [mailing list](https://mailman.tu-dortmund.de/mailman/listinfo/tudocomp.cs) to see what is going on. 


### Review of Coding Strategies
tudocomp currently provides only a canonical Huffman coder and a customized static low entropy coder.
Novel coding algorithms provide better precision than a Huffman coder, and are faster than arithmetic coding.
It is interesting to integrate general codings like ASM or FSE or specialized codings like ETDC into tudocomp, and evaluate
all codings with respect to their compression ratio and compression/decompression time.

*References:*

 - [ETDC for natural language text compression](https://www.dcc.uchile.cl/~gnavarro/ps/ir06.pdf)
 - [Asymmetric Numeral Systems](https://arxiv.org/pdf/1311.2540.pdf)
 - [FSE Encoder](https://github.com/Cyan4973/FiniteStateEntropy)

*Category*: Encodings


### AreaComp
The idea of AreaComp is to substitute frequent large substrings of a text.
We search for the substring that maximizes the value of a cost function.
The cost function weights the number of occurrences and the length of a substring, e.g., the multiplication of the length and the number
of occurrences of a substring is a natural choice.

Given the suffix array and the longest common prefix array, 
we can find the number of occurrences of a substring in the text by looking at both arrays.
A naive approach would store all substrings of a certain length occurring at least twice in a priority queue, with its cost function value as its key.
We pop the top element (i.e., the best substring w.r.t.\ the cost function) off from the heap, substitute its occurrences, 
and update the suffix array and the longest common prefix array.
There is a similar method called ``greedy off-line textual substitution'' that considers all non-overlapping occurrences.

*Category*: Compressor

References

 - [Off-line compression by greedy textual substitution](http://ieeexplore.ieee.org/iel5/5/19320/00892709.pdf)
 - [Data structures and algorithms for the string statistics problem](http://link.springer.com/article/10.1007/BF01955046)

### Clever Tie Breaking for lcpcomp
Our compressor [lcpcomp](https://github.com/tudocomp/tudocomp/blob/public/include/tudocomp/compressors/LCPCompressor.hpp) (implemented in tudocomp) is a 
longest-first greedy compression algorithm.
Given multiple longest substrings, there is no strict tie breaking rule that states which longest substring to choose.
The focus of this project is to enhance the lcpcomp compressors with a heuristic for which substring lcpcomp should choose.
The heuristic can be based on the selection of a tie breaking rule with the best expectation in terms of compression ratio or
decompression speed.

*References*

 - this problem is similar to a [semi-greedy variant of LZ77](http://link.springer.com/chapter/10.1007/978-3-642-82456-2_11)

### k-maxsat for lcpcomp
Decompressing an [lcpcomp](https://github.com/tudocomp/tudocomp/blob/public/include/tudocomp/compressors/LCPCompressor.hpp) compressed file is a heavy task with respect to time.
That is because references of lcpcomp can be nested, i.e., a reference can refer to a substring that got replaced with another reference.
The nesting of references can form long dependency chains that need to be resolved before the actual decompression can take place.
This project focuses on a modification of the compression strategy, where we want to check whether it is possible to cirumvent the 
production of long dependency chaines. It can be shown that this problem is related to the k-maxsat problem.
The aim of this project is to devise an approximation algorithm for the k-maxsat problem to prevent long dependency chains.

*References*

 - Mayr, Ernst W., Prömel, Hans Jürgen, Steger, Angelika: "Lectures on Proof Verification and Approximation Algorithms", the MaxEkSat-Problem


### Efficient Integer Coders
Aim of this project is to implement and evaluate a fast Fibonacci encoding algorithm.
Fibonacci coding is a universal code that represents integers succinctly.
The coding splits an integer up to summands that are Fibonacci words.
Although the coding achieves a very compact representation, its decoding is slow.
The aim of this project is to implement a new encoding algorithm, and to measure its speed with currently avaiable Fibonacci coders.

*Category*: Encodings

*References*

 - [Fast Fibonacci Encoding Algorithm](http://ceur-ws.org/Vol-567/paper14.pdf)
 - [Fibonacci Coder of the SDSL](https://github.com/simongog/sdsl-lite/tree/master/include/sdsl/coder_fibonacci.hpp)


### LZ78 with a Compact Hash Table
Our [LZ78 compressor](https://github.com/tudocomp/tudocomp/blob/public/include/tudocomp/compressors/LZ78Compressor.hpp) 
can utilize different Lempel-Ziv-78 tries like a binary trie, a ternary trie, or a trie based on a hash table.
The latter is the faster, but yet heaviest trie implementation.
In the light of compact hash tables, we wonder whether we can drop the memory footprint of hash table implementations
while still being very fast.

 - [Don't Thrash: How to Cache Your Hash on Flash](https://arxiv.org/abs/1208.0290)

*Category*: Compressor, Hashing

### String Analyzing tools in JavaScript
While working with lossless compression algorithms on texts, we often experience the lack of 
tools that visualize text index data structures.
There are some tools that provide limited insight to some data structures.
Yet, there is no a powerful and easy-to-use tool that covers a majority of the most frequently used data structures.
The aim of this project is to produce a JavaScript written web platform that interactively visualizes the most commonly used data structures
like suffix arrays, longest common prefix array, etc., with respect to text compression.

*References*

 - [Shinohara's Online Encyclopedia of Strings](http://www.shino.ecei.tohoku.ac.jp/stringology/)
 - [Strinalze - Analyze a string or a sequence of generated strings](https://github.com/koeppl/strinalyze)

*Category*: Visualization

### 7zip compatible output format
To improve the usability of the tudocomp framework, we want the tudocomp output format to become compatible to 7zip.
The 7z format supports various compression techniques due to a versatile header describing the exact used compression technique.
This project's aim is to adapt the 7z format for the tudocomp command line tool.

*Category*: Usability

*References*

 - [7zip](http://www.7-zip.org)


### GUI file archiver
The tudocomp framework provides only a command line tool as an interface to the end user.
An end-user friendly GUI application can be beneficient for addressing command line antipathetic users.
The GUI should provide the selection of multiple files/directories and an easy way to 
assemble a custom compression pipeline. 
The GUI can be programmed, for instance, in Qt oder gtkmm.

*Category*: GUI

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
