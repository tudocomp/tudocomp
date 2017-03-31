#!/usr/bin/python3

import collections
import hashlib
import itertools
import os
import re
import sys
import statistics
import subprocess
import tempfile
import time

# Usage
if len(sys.argv) < 2:
    print("Usage: ", sys.argv[0], " [file-to-compress]")
    quit()

# Ensure that the input file exists
# TODO: allow multiple input files
sourcefilename = sys.argv[1]
if not os.access(sourcefilename, os.R_OK):
    print("ERROR: Input file not found or not readable:", sourcefilename)
    quit()

# Check that valgrind is available for memory measurement
try:
    subprocess.check_call(["valgrind", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mem_available = True
except:
    mem_available = False
    print(sys.exc_info())
    print("WARNING: valgrind not found - memory measurement unavailable.")
    print()

# Define number of iterations
# TODO: make customizable via command-line
numIterations=1
print("Number of iterations: ", numIterations)

# Program execution definition
StdOut = 0
StdIn  = 0

Exec = collections.namedtuple('Exec', ['args', 'outp', 'inp'])
Exec.__new__.__defaults__ = (None, None) # args is required

# Compressor Pair definition
CompressorPair = collections.namedtuple('CompressorPair', ['name', 'compress', 'decompress'])

def Tudocomp(name, algorithm, tdc_binary='./tdc', cflags=[], dflags=[]):
    return CompressorPair(name,
        compress   = Exec(args=[tdc_binary, '-a', algorithm] + cflags, outp='--output'),
        decompress = Exec(args=[tdc_binary, '-d'] + dflags, outp='--output'))

def StdCompressor(name, binary, cflags=[], dflags=[]):
    return CompressorPair(name,
        compress   = Exec(args=[binary] + cflags, inp=StdIn, outp=StdOut),
        decompress = Exec(args=[binary] + dflags, inp=StdIn, outp=StdOut))

# Define suite
# TODO: read from file
suite = [
    # tudocomp examples
    Tudocomp(name='bwtzip',                          algorithm='bwt:rle:mtf:encode(huff)'),
    Tudocomp(name='lcpcomp(t=5,arrays,scans(a=25))', algorithm='lcpcomp(coder=sle,threshold=5,comp=arrays,dec=scan(25))'),
    Tudocomp(name='lzss_lcp(t=5,bit)',               algorithm='lzss_lcp(coder=bit,threshold=5)'),
    Tudocomp(name='lz78u(t=5,huff)',                 algorithm='lz78u(coder=bit,threshold=5,comp=buffering(huff))'),
    Tudocomp(name='lcpcomp(t=5,heap,compact)',       algorithm='lcpcomp(coder=sle,threshold="5",comp=heap,dec=compact)'),
    Tudocomp(name='sle',                             algorithm='encode(sle)'),
    Tudocomp(name='huff',                            algorithm='encode(huff)'),
    Tudocomp(name='lzw(ternary)',                    algorithm='lzw(coder=bit,lz78trie=ternary)'),
    Tudocomp(name='lz78(ternary)',                   algorithm='lz78(coder=bit,lz78trie=ternary)'),
    # Some standard Linux compressors
    StdCompressor(name='gzip -1',  binary='gzip',  cflags=['-1'], dflags=['-d']),
    StdCompressor(name='gzip -9',  binary='gzip',  cflags=['-9'], dflags=['-d']),
    StdCompressor(name='bzip2 -1', binary='bzip2', cflags=['-1'], dflags=['-d']),
    StdCompressor(name='bzip2 -9', binary='bzip2', cflags=['-9'], dflags=['-d']),
    StdCompressor(name='lzma -1',  binary='lzma',  cflags=['-1'], dflags=['-d']),
    StdCompressor(name='lzma -9',  binary='lzma',  cflags=['-9'], dflags=['-d']),
    ]

def memsize(num, suffix='B'):
    for unit in ['','Ki','Mi','Gi','Ti','Pi','Ei','Zi']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'Yi', suffix)

def timesize(num, suffix='s'):
    if(num < 1.0):
        for unit in ['','m','μ','n']:
            if num > 1.0:
                return "%3.1f%s%s" % (num, unit, suffix)
            num *= 1000
        return "%.1f%s%s" % (num, '?', suffix)
    else:
        if(num < 600):
            return "%3.1f%s" % (num, 's')
        elif(num < 3600):
            num /= 60
            return "%3.1f%s" % (num, 'min')
        elif(num > 3600):
            num /= 3600
            return "%3.1f%s" % (num, 'h')

def run_exec(x, infilename, outfilename):
    args = x.args

    # Delete existing output file
    if os.path.exists(outfilename):
        os.remove(outfilename)

    # Determine Output
    if(x.outp == StdOut):
        outfile = open(outfilename, "wb")
        pipe_out = outfile
    else:
        outfile = None
        pipe_out = logfile
        args += ([x.outp, outfilename] if x.outp != None else [outfilename])

    # Determine input
    if(x.inp == StdIn):
        infile = open(infilename, "rb")
        pipe_in = infile
    else:
        infile = None
        pipe_in = None
        args += ([x.inp, infilename]   if x.inp  != None else [infilename])

    # Call
    t0 = time.time()
    subprocess.check_call(args, stdin=pipe_in, stdout=pipe_out, stderr=logfile)

    # Close files
    outfile.close() if outfile else None
    infile.close()  if infile  else None

    # Yield time delta
    return(time.time() - t0)

def measure_time(x, infilename, outfilename):
    t=[]
    for _ in range(0, numIterations):
        t = t + [run_exec(x, infilename, outfilename)]

    return(statistics.median(t))

def measure_mem(x, infilename, outfilename):
    massiffilename=tempfile.mktemp()

    run_exec(
        Exec(args=['valgrind', '-q', '--tool=massif', '--pages-as-heap=yes',  '--massif-out-file=' + massiffilename] + x.args, inp=x.inp, outp=x.outp),
        infilename, outfilename)

    with open(massiffilename) as f:
        maxmem=0
        for line in f.readlines():
            match = re.match('^mem_heap_B=([0-9]+)', line)
            if match:
                maxmem = max(maxmem,int(match.group(1)))

    os.remove(massiffilename)
    return(maxmem)


sourcefilehash=hashlib.sha256(open(sourcefilename, 'rb').read()).hexdigest()
sourcefilesize=os.path.getsize(sourcefilename)

maxnicknamelength = len(max(suite, key=lambda p: len(p.name))[0] ) + 3

print("%s (%s, sha256=%s)" % (sourcefilename, memsize(sourcefilesize), sourcefilehash))

print()
print(("%"+ str(maxnicknamelength) + "s | %10s | %10s | %10s | %10s | %10s | %4s |") % ("Compressor", "C Time", "C Memory", "C Rate", "D Time", "D Memory", "chk"))
print('-'*(maxnicknamelength+5*10+6*3+4+2))

logfilename=tempfile.mktemp()
decompressedfilename=tempfile.mktemp()
outfilename=tempfile.mktemp()

def print_column(content, format="%11s", sep="|"):
    print((format + " " + sep) % content, end='',flush=True)

try:
    with open(logfilename,"wb") as logfile:
        for c in suite:
            # nickname
            print_column(c.name, "%"+ str(maxnicknamelength) +"s")

            # compress time
            try:
                comp_time=measure_time(c.compress, sourcefilename, outfilename)
                print_column(timesize(comp_time))
            except FileNotFoundError as e:
                print_column("(ERR)", sep=">")
                print(" " + e.strerror)
                continue

            # compress memory
            if mem_available:
                comp_mem=measure_mem(c.compress, sourcefilename, outfilename)
                print_column(memsize(comp_mem))
            else:
                print_column("(N/A)")

            # compress rate
            outputsize=os.path.getsize(outfilename)
            print_column(100*float(outputsize) / float(sourcefilesize), format="%10.4f%%")

            # decompress time
            dec_time=measure_time(c.decompress, outfilename, decompressedfilename)
            print_column(timesize(dec_time))

            # decompress memory
            if mem_available:
                dec_mem=measure_mem(c.decompress, outfilename, decompressedfilename)
                print_column(memsize(dec_mem))
            else:
                print_column("(N/A)")

            # decompress check
            decompressedhash=hashlib.sha256(open(decompressedfilename, 'rb').read()).hexdigest()
            if decompressedhash != sourcefilehash:
                #does a hash really help upon failure?
                #print("%11s |" % decompressedhash, end='',flush=True)
                print_column("FAIL", format="%5s")
            else:
                print_column("OK", format="%5s")

            # EOL
            print()
except:
    print()
    print("ERROR:", sys.exc_info()[0])
    print(sys.exc_info()[1])

with open(logfilename, 'r') as fin: print(fin.read())
os.remove(logfilename)

if os.path.exists(decompressedfilename):
    os.remove(decompressedfilename)

if os.path.exists(outfilename):
    os.remove(outfilename)

