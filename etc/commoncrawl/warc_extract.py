#!/usr/bin/env python3

# Extracts all ASCII contents of a WET archive file

import sys
import mmap
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename", type=str)
parser.add_argument("--ascii", action="store_true")
parser.add_argument("--non_ascii", action="store_true")
parser.add_argument("--nulls", action="store_true")

parser.parse_args()
args = parser.parse_args()

filename = args.filename

def unit(n):
    return str.format("{} MiB", int(n/1024/1024))

with open(filename, 'rb') as f:
    m = mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ)

    header_offset = 0
    total_length = 0

    while header_offset < m.size():
        assert m.find(b"WARC/1.0\r\n", header_offset) == header_offset

        def field(name):
            pos = m.find(name, header_offset)
            pos = pos + len(name)
            end_pos = m.find(b"\r\n", pos)
            value = m[pos:end_pos]
            return (value, end_pos)

        (length, content_pos) = field(b"Content-Length: ")
        content_length = int(length)
        content_pos = content_pos + 4

        content = m[content_pos:content_pos + content_length]

        total_length += content_length

        #sys.stdout.buffer.write(b"START\r\n")
        #sys.stdout.buffer.write(content)
        #sys.stdout.buffer.write(b"END\r\n\r\n")

        (content_type, etc) = field(b"Content-Type: ")

        if content_type == b"application/warc-fields":
            pass
        elif content_type == b"text/plain":
            (uri, etc) = field(b"WARC-Target-URI: ")
            assert len(uri) > 0

            has_ascii = False
            has_non_ascii = False

            try:
                content.decode("ascii")
            except UnicodeDecodeError:
                has_non_ascii = True
            else:
                has_ascii = True

            has_null = (content.find(0) != -1)

            use = False

            if (args.ascii and has_ascii):
                use = True
            if (args.non_ascii and has_non_ascii):
                use = True

            if (not args.nulls and has_null):
                use = False

            if (use):
                sys.stdout.buffer.write(content)
        else:
            sys.exit(1)

        header_offset = content_pos
        header_offset += content_length
        header_offset += 4
