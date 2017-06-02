#!/usr/bin/python3

# Python3 host for the charter core.
# This can be used to generate chart SVGs from the command-line.
# Requires the PyExecJS module, NodeJS is recommended.

import argparse
import os
import json
import sys

import execjs

# Determine local directory
local_dir = os.path.dirname(os.path.realpath(__file__))

# Parse command line arguments
parser = argparse.ArgumentParser(description="""
    Generate a chart SVG based on a tudocomp JSON dataset.
""")

parser.add_argument('--width', type=int, default=900,
                    help='the SVG width in pixels')
parser.add_argument('--height', type=int, default=450,
                    help='the SVG width in pixels')
parser.add_argument('--no-groups', dest="draw_groups", action="store_false",
                    help='disable group bracket drawing')
parser.add_argument('--no-legend', dest="draw_legend", action="store_false",
                    help='disable legend drawing')
parser.add_argument('--no-offsets', dest="draw_offsets", action="store_false",
                    help='disable memory offset drawing')
parser.add_argument('file', nargs="?", type=str,
                    help='the input file (json)')

parser.set_defaults(draw_groups=True)
parser.set_defaults(draw_legend=True)
parser.set_defaults(draw_offsets=True)

args = parser.parse_args()

# Read input into stat_json (file or stdin)
if args.file:
    with open(args.file) as input_file:
        stat_json = input_file.read()
elif not sys.stdin.isatty():
    stat_json = sys.stdin.read()
else:
    print("no input given")
    quit()

# Create JavaScript runtime
js = execjs.get()

# Compile charter script
with open(local_dir + "/charter.js", "r") as charter_js_file:
    charter = execjs.compile(charter_js_file.read())

# TODO: test success

# Set options
options = {
    "svgWidth": args.width,
    "svgHeight": args.height,
    "drawGroups": args.draw_groups,
    "drawOffsets": args.draw_offsets,
    "drawLegend": args.draw_legend,
}

# Draw chart
print(charter.call(
    "drawSVG",
    stat_json,
    json.dumps(options)))

