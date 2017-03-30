#!/usr/bin/python3

import re
import os
import sys
import itertools
import subprocess
import time
import statistics
import hashlib
import tempfile

if len(sys.argv) < 2:
    print("Usage: ", sys.argv[0], " [file-to-compress]")
    quit()

sourcefilename = sys.argv[1]
if not os.access(sourcefilename, os.R_OK):
    print("ERROR: Input file not found or not readable:", sourcefilename)
    quit()

# Check that valgrind is available for memory measurement
try:
    subprocess.check_call("valgrind --version")
    mem_available = True
except:
    mem_available = False
    print("WARNING: valgrind not found - memory measurement unavailable.")
    print()

numIterations=1
print("Number of iterations: ", numIterations)

compressor_pairs = [
        #a pair consists of a nickname, a compressor list, a decompressor list
        #for each list:
        #first output flag, if STDOUT, then 1
        #second input flag, if STDIN, then 1
        #third is program name
        #other parameters are program arguments
       ('bwtzip',['--output','','./tdc','-a','chain(chain(chain(bwt,rle),mtf),encode(huff))','--force'], ['--output','','./tdc', '--decompress']),
       ('lcpcomp(t=5,arrays,scans(a=25))',['--output','','./tdc','-a','lcpcomp(coder=sle,threshold="5",comp=arrays,dec=scan("25"))','--force'], ['--output','','./tdc', '--decompress']),
       ('lzss_lcp(t=5,bit)'  , ['--output' , '' , './tdc' , '-a' , 'lzss_lcp(coder=bit,threshold=5)' , '--force'] , ['--output' , '' , './tdc' , '--decompress']) ,
       ('lz78u(t=5,huff)',['--output','','./tdc','-a','lz78u(coder=bit,threshold=5,comp=buffering(huff))','--force'], ['--output','','./tdc', '--decompress']),
       ('lcpcomp(t=5,heap,compact)',['--output','','./tdc','-a','lcpcomp(coder=sle,threshold="5",comp=heap,dec=compact)','--force'], ['--output','','./tdc', '--decompress']),
       ('sle' , ['--output' , '' , './tdc' , '-a' , 'encode(sle)' , '--force'] , ['--output' , '' , './tdc' , '--decompress']) ,
       ('huff'  , ['--output' , '' , './tdc' , '-a' , 'encode(huff)'  , '--force'] , ['--output' , '' , './tdc' , '--decompress']) ,
       ('lzw(ternary)'   , ['--output' , '' , './tdc' , '-a' , 'lzw(coder=bit,lz78trie=ternary)'           , '--force'] , ['--output' , '' , './tdc' , '--decompress']) ,
       ('lz78(ternary)'  , ['--output' , '' , './tdc' , '-a' , 'lz78(coder=bit,lz78trie=ternary)'          , '--force'] , ['--output' , '' , './tdc' , '--decompress']) ,
       ('gzip -1'  , [1 , 1 , "gzip"  , "-1"] , [1 , 1 , 'gzip'  , '-d']) ,
       ('gzip -9'  , [1 , 1 , "gzip"  , "-9"] , [1 , 1 , 'gzip'  , '-d']) ,
       ('bzip2 -1' , [1 , 1 , "bzip2" , "-1"] , [1 , 1 , 'bzip2' , '-d']) ,
       ('bzip2 -9' , [1 , 1 , "bzip2" , "-9"] , [1 , 1 , 'bzip2' , '-d']) ,
       ('lzma -1'  , [1 , 1 , "lzma"  , "-1"] , [1 , 1 , 'lzma'  , '-d']) ,
       ('lzma -9'  , [1 , 1 , "lzma"  , "-9"] , [1 , 1 , 'lzma'  , '-d']) ,
       # ([1,1,'gz9',"gzip", "-9"],
       # ([1,1,'bz9',"bzip2","-9"],
       # (['a','', '7z', '7z'],
       # (['--output','','lz78u','./tdc', '-a', 'lz78u(streaming(ascii),ascii)'],
#        ["echo", 'a ', 'b']
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


def run_compress(compressor,infilename,outfilename):
    if os.path.exists(outfilename):
        os.remove(outfilename)
    if compressor[0] == 1:
        with open(outfilename,"wb") as outfile, open(infilename,"rb") as infile:
            args=compressor[2:]
            t = time.time()

            subprocess.check_call(args, stdin=infile, stdout=outfile, stderr=logfile)
            #subprocess.call(args, stdin=infile, stdout=outfile, stderr=logfile)  # do not break on error
            elapsed_time = time.time() - t
        return(elapsed_time)
    else:
        args=compressor[2:]
        if not compressor[0]:
            args=args+outfilename
        else:
            args=args+[compressor[0],outfilename]
        if not compressor[1]:
            args=args+[infilename]
        else:
            args=args+[compressor[1],infilename]
        t = time.time()
        #subprocess.call(args, stdout=logfile, stderr=logfile)
        subprocess.check_call(args, stdout=logfile, stderr=logfile)

        elapsed_time = time.time() - t
        return(elapsed_time)

def measure_time(compressor, infilename, outfilename):
    t=[]
    for _ in range(0, numIterations):
        t=t+[run_compress(compressor,infilename,outfilename)]
    return(statistics.median(t))
def measure_mem(compressor, infilename, outfilename):
    massiffilename=tempfile.mktemp()
    run_compress(compressor[0:2] + ['valgrind', '-q', '--tool=massif', '--pages-as-heap=yes',  '--massif-out-file='+massiffilename]  + compressor[2:], infilename, outfilename)
    with open(massiffilename) as f:
        maxmem=0
        for line in f.readlines():
            match = re.match('^mem_heap_B=([0-9]+)', line)
            if match:
                maxmem = max(maxmem,int(match.group(1)))
    os.unlink(massiffilename)
    return(maxmem)


sourcefilehash=hashlib.sha256(open(sourcefilename, 'rb').read()).hexdigest()
sourcefilesize=os.path.getsize(sourcefilename)


maxnicknamelength = len(max(compressor_pairs,key=lambda p: len(p[0]))[0] )+2

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
        for (nickname,compressor,decompressor) in compressor_pairs:
            # nickname
            print(("%"+ str(maxnicknamelength) +"s |") % nickname, end='',flush=True)

            # compress time
            try:
                comp_time=measure_time(compressor,sourcefilename, outfilename)
                print_column(timesize(comp_time))
            except FileNotFoundError as e:
                print_column("(ERR)", sep=">")
                print(" " + e.strerror)
                continue

            # compress memory
            if mem_available:
                comp_mem=measure_mem(compressor, sourcefilename, outfilename)
                print_column(memsize(comp_mem))
            else:
                print_column("(N/A)")

            # compress rate
            outputsize=os.path.getsize(outfilename)
            print_column(100*float(outputsize) / float(sourcefilesize), format="%10.4f%%")

            # decompress time
            dec_time=measure_time(decompressor, outfilename, decompressedfilename)
            print_column(timesize(dec_time))

            # decompress memory
            if mem_available:
                dec_mem=measure_mem(decompressor,  outfilename, decompressedfilename)
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
