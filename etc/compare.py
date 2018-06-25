#!/usr/bin/env python3

import argparse
import collections
import hashlib
import itertools
import os
import re
import sys
import statistics
import string
import subprocess
import time
import pprint
import json
import random
import traceback
from collections import namedtuple

# Argument parser
parser = argparse.ArgumentParser(description="""
    Compare running times and memory usage of a set of compressors.
""")

parser.add_argument('--suite', '-s', type=str, default='',
                    help='the comparison suite to execute')
parser.add_argument('--iterations', '-n', type=int, default=1,
                    help='the amount of iterations for each input file')
parser.add_argument('files', metavar='FILE', type=str, nargs='+',
                    help='the input files to use for comparison')
parser.add_argument('--format', type=str, default='stdout',
                    help='Format to output')
parser.add_argument('--nomem', action="store_true",
                    help='Don\'t measure memory')
parser.add_argument('--nodec', action="store_true",
                    help='Only compress, don\'t decompress')
parser.add_argument('--nolog', action="store_true",
                    help='Don\'t print log when done')
parser.add_argument('--tmp', type=str, default='/tmp',
                    help='Directory for temporary files')

args = parser.parse_args()

def randstr(n):
    return(''.join(random.choice(string.ascii_uppercase) for _ in range(n)))

def mktemp():
    while(True):
        tmp = args.tmp + '/' + randstr(8)
        if(not os.path.isfile(tmp)):
            break
	
    return(tmp)


class StdOutTable:
    def __init__(self):
        pass
    def print(self, *s):
        print(*s)
    def file(self, srcfname, srcsize, srchash):
        print()
        print("File: %s (%s, sha256=%s)" % (srcfname, memsize(srcsize), srchash))
    def header(self, tup):
        print()
        print(("%"+ str(maxnicknamelength) + "s | %10s | %10s | %10s | %10s | %10s | %4s |") % tup)
        print('-'*(maxnicknamelength+5*10+6*3+4+2))
    def cell(self, content, format, sep, f):
        print((format + " " + sep) % f(content), end='',flush=True)
    def end_row(self):
        print()
    def flush(self):
        pass

class JsonTable:
    def __init__(self):
        self.messages = []
        self.files = {}
    def print(self, *s):
        self.messages.append(" ".join(map(str, iter(s))))
    def file(self, srcfname, srcsize, srchash):
        self.files[srcfname] = {}
        self.currentfile = self.files[srcfname]
        self.currentfile["cols"] = {}
        self.currentfile["size"] = srcsize
        self.currentfile["hash"] = srchash
    def header(self, tup):
        self.headings = tup
        self.current_heading = 0
        for heading in tup:
            self.currentfile["cols"][heading] = []
    def cell(self, content, format, sep, f):
        self.currentfile["cols"][self.headings[self.current_heading]].append(content)
        self.current_heading += 1
    def end_row(self):
        self.current_heading = 0

    def flush(self):
        print(json.dumps(self.__dict__, sort_keys=True, indent=4))

sot = StdOutTable()
if args.format == "json":
    sot = JsonTable()

# Ensure that the input files are readable
for srcfname in args.files:
    if not os.access(srcfname, os.R_OK):
        sot.print("ERROR: Input file not found or not readable:", srcfname)
        quit()

# Check that valgrind is available for memory measurement
mem_available = False
if not args.nomem:
    try:
        subprocess.check_call(["valgrind", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        mem_available = True
    except:
        mem_available = False
        sot.print("WARNING: valgrind not found - memory measurement unavailable.")
        sot.print()

# Program execution definition
StdOut = 0
StdIn  = 0

Exec = collections.namedtuple('Exec', ['cmd', 'outp', 'inp'])
Exec.__new__.__defaults__ = (None, None) # args is required

# Compressor Pair definition
CompressorPair = collections.namedtuple('CompressorPair', ['name', 'compress', 'measure', 'decompress', 'stats'])
CompressorPair.__new__.__defaults__ = (None, None, None, None, dict())

def Tudocomp(name, algorithm, tdc_binary='./tdc', cflags=[], dflags=[]):
    return CompressorPair(name,
        compress   = Exec(args=[tdc_binary, '-a', algorithm] + cflags, outp='--output'),
        decompress = Exec(args=[tdc_binary, '-d'] + dflags, outp='--output'))

def StdCompressor(name, binary, cflags=[], dflags=[]):
    return CompressorPair(name,
        compress   = Exec(args=[binary] + cflags, inp=StdIn, outp=StdOut),
        decompress = Exec(args=[binary] + dflags, inp=StdIn, outp=StdOut))

# Define suite
if args.suite:
    # Evaluate suite as Python
    try:
        with open(args.suite, "r") as f:
            suite = eval(f.read())

        # sanity checks
        if not type(suite) is list:
            raise(Exception(
                "Suite evaluated to " + str(type(suite)) +
                ", but should be a list of CompressorPair objects"))

        if len(suite) == 0:
            raise(Exception("Suite is empty"))

        for c in suite:
            if not isinstance(c, CompressorPair):
                raise(Exception("Suite must only contain CompressorPair objects" +
                    ", found " + str(type(c))))
    except:
        sot.print("ERROR: Failed to load suite '" + args.suite + "'")
        sot.print(sys.exc_info()[1])
        quit()

    sot.print("Using suite '" + args.suite + "'")

else:
    # default
    suite = [
        # tudocomp examples
		Tudocomp(name='lfs_simst',                       algorithm='lfs_comp(sim_st)'),
		Tudocomp(name='lfs_esa',                       algorithm='lfs_comp(esa)'),
	#	Tudocomp(name='lfs_st',                       algorithm='lfs_comp(st)'),
		Tudocomp(name='lfs2',                       	 algorithm='lfs2'),
		Tudocomp(name='lz78(ternary)',                   algorithm='lz78(coder=bit,lz78trie=ternary)'),
		Tudocomp(name='lz78',                   		 algorithm='lz78'),
		Tudocomp(name='lzw',                   		 	 algorithm='lzw'),
		Tudocomp(name='repair(min=50)',                  algorithm='repair(bit,50)'),
		Tudocomp(name='lzw',                   		 	 algorithm='lzw'),
		Tudocomp(name='lzss',                   		 algorithm='lzss(bit)'),
        Tudocomp(name='bwtzip',                          algorithm='bwt:rle:mtf:encode(huff)'),
        Tudocomp(name='lcpcomp(t=5,arrays,scans(a=25))', algorithm='lcpcomp(coder=sle,threshold=5,comp=arrays,dec=scan(25))'),
        Tudocomp(name='lzss_lcp(t=5,bit)',               algorithm='lzss_lcp(coder=bit,threshold=5)'),
        Tudocomp(name='lz78u(t=5,huff)',                 algorithm='lz78u(coder=bit,threshold=5,comp=buffering(huff))'),
        Tudocomp(name='lcpcomp(t=5,heap,compact)',       algorithm='lcpcomp(coder=sle,threshold="5",comp=heap,dec=compact)'),
        Tudocomp(name='sle',                             algorithm='encode(sle)'),
        Tudocomp(name='huff',                            algorithm='encode(huff)'),
        Tudocomp(name='lzw(ternary)',                    algorithm='lzw(coder=bit,lz78trie=ternary)'),


        # Some standard Linux compressors
        StdCompressor(name='gzip -1',  binary='gzip',  cflags=['-1'], dflags=['-d']),
        StdCompressor(name='gzip -9',  binary='gzip',  cflags=['-9'], dflags=['-d']),
        StdCompressor(name='bzip2 -1', binary='bzip2', cflags=['-1'], dflags=['-d']),
        StdCompressor(name='bzip2 -9', binary='bzip2', cflags=['-9'], dflags=['-d']),
        StdCompressor(name='lzma -1',  binary='lzma',  cflags=['-1'], dflags=['-d']),
        StdCompressor(name='lzma -9',  binary='lzma',  cflags=['-9'], dflags=['-d']),
		#StdCompressor(name='lcpcompress',  binary='lcpcompress',  cflags=[''], dflags=['-d']),
    ]
    sot.print("Using built-in default suite")

def memsize(num, suffix='B'):
    for unit in ['','Ki','Mi','Gi','Ti','Pi','Ei','Zi']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'Yi', suffix)

def timesize(num, suffix='s'):
    if(num < 1.0):
        for unit in ['','m','mu','n']:
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

def run_exec(x, infilename, outfilename, logging):
    cmd = x.cmd.replace('@IN@', infilename).replace('@OUT@', outfilename);

    # Delete existing output file
    if os.path.exists(outfilename):
        os.remove(outfilename)

    # Determine input
    if(x.inp == StdIn):
        infile = open(infilename, "rb")
        pipe_in = infile
    else:
        infile = None
        pipe_in = None

    # Determine Output
    if(x.outp == StdOut):
        outfile = open(outfilename, "wb")
        pipe_out = outfile
    else:
        outfile = None
        pipe_out = (current_logfile if logging else subprocess.DEVNULL)

    # Call
    t0 = time.time()
    proc = subprocess.Popen(cmd,
                            shell=True,
                            stdin=pipe_in,
                            stdout=pipe_out,
                            stderr=pipe_out)
    result = proc.wait()

    # Close files
    outfile.close() if outfile else None
    infile.close()  if infile  else None

    # result
    if(result != 0):
        raise Exception('execution failed: ' + cmd);

    # Yield time delta
    return(time.time() - t0)

def measure_time(x, infilename, outfilename):
    t=[]
    first=True
    for _ in range(0, args.iterations):
        t = t + [run_exec(x, infilename, outfilename, first)]
        first = False

    return(statistics.median(t))

def measure_mem(x, infilename, outfilename):
    massiffilename=mktemp()

    run_exec(
        Exec(args=['valgrind', '-q', '--tool=massif', '--pages-as-heap=yes',  '--massif-out-file=' + massiffilename] + x.args, inp=x.inp, outp=x.outp),
        infilename, outfilename, False)

    with open(massiffilename) as f:
        maxmem=0
        for line in f.readlines():
            match = re.match('^mem_heap_B=([0-9]+)', line)
            if match:
                maxmem = max(maxmem,int(match.group(1)))

    os.remove(massiffilename)
    return(maxmem)

def measure_size(x, filename):
    if not x:
        return(os.path.getsize(filename))
    else:
        proc = subprocess.Popen(x.replace('@OUT@', filename), shell=True,
                           stdout=subprocess.PIPE, 
                           stderr=subprocess.PIPE)

        out, err = proc.communicate()
        if(proc.returncode == 0):
            return(int(out))
        else:
            raise Exception('failed to measure output file size', out, err)

maxnicknamelength = max(10, len(max(suite, key=lambda p: len(p.name))[0] ) + 3)

sot.print("Number of iterations per file: ", args.iterations)

for srcfname in args.files:
    if(not args.nodec):
        srchash = hashlib.sha256(open(srcfname, 'rb').read()).hexdigest()
    else:
        srchash = 'dontcare'

    srcsize = os.path.getsize(srcfname)

    sot.file(srcfname, srcsize, srchash)

    sot.header(("Compressor", "C Time", "C Memory", "C Rate", "D Time", "D Memory", "chk"));

    log = ''
    current_logfile = subprocess.DEVNULL # will be set for each compressor
    decompressedfilename = mktemp()
    outfilename = mktemp()

    def print_column(content, format="%11s", sep="|", f=lambda x:x):
        sot.cell(content, format, sep, f)
    def end_row():
        sot.end_row()

    try:
        for c in suite:
            # nickname
            print_column(c.name, "%"+ str(maxnicknamelength) +"s")

            logfilename = mktemp()
            with open(logfilename, 'a+') as current_logfile:
                # compress time
                try:
                    comp_time=measure_time(c.compress, srcfname, outfilename)
                    print_column(comp_time*1000, f=lambda x: timesize(x/1000))
                except FileNotFoundError as e:
                    print_column("(ERR)", sep=">")
                    sot.print(" " + e.strerror)
                    continue

                current_logfile.seek(0)
                curlog = current_logfile.read()

            # compress memory
            if mem_available:
                comp_mem=measure_mem(c.compress, srcfname, outfilename)
                print_column(comp_mem,f=memsize)
            else:
                print_column("(N/A)")


            
            log += "\n### output of " + c.name + " ###\n"
            log += curlog
            log += "stats:\n"

            # compress rate
            outputsize = measure_size(c.measure, outfilename)
            print_column(float(outputsize) / float(srcsize), format="%10.4f%%", f=lambda x: 100*x)

            if not args.nodec:
                # decompress time
                dec_time = measure_time(c.decompress, outfilename, decompressedfilename)
                print_column(dec_time*1000,f=lambda x: timesize(x/1000))

                # decompress memory
                if mem_available:
                    dec_mem = measure_mem(c.decompress, outfilename, decompressedfilename)
                    print_column(dec_mem,f=memsize)
                else:
                    print_column("(N/A)")

                # decompress check
                decompressedhash = hashlib.sha256(
                    open(decompressedfilename, 'rb').read()).hexdigest()

                if decompressedhash != srchash:
                    print_column("FAIL", format="%5s")
                else:
                    print_column("OK", format="%5s")

            else:
                print_column("-")
                print_column("-")
                print_column("-", format="%5s")

            num_stats = 0
            stats = ' '
            for name, regex in c.stats.items():
                p = re.compile(regex)
                m = p.search(curlog.replace('\n', ''))
                if m:
                    if(num_stats > 0):
                        stats += ", "

                    stats += name + " = " + m.group(1)
                    num_stats += 1

            if(num_stats > 0):
                print_column(stats, format="%" + str(len(stats)) + "s")

            # EOL
            end_row()
    except:
        sot.print()
        sot.print("ERROR:", sys.exc_info()[0])
        sot.print(sys.exc_info()[1])
        sot.print(traceback.print_tb(sys.exc_info()[2]))

if not args.nolog:
    sot.print()
    sot.print("Log output (use --nolog to disable):")
    sot.print(log);

if os.path.exists(decompressedfilename):
    os.remove(decompressedfilename)

if os.path.exists(outfilename):
    os.remove(outfilename)

sot.flush()

