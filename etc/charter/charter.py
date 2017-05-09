#!/usr/bin/python3

# Python3 host for the charter core.
# This can be used to generate chart SVGs from the command-line.
# Requires the PyExecJS module, NodeJS is recommended.

import argparse
import json

import execjs

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
parser.add_argument('file', metavar='FILE', type=str,
                    help='the input file (json)')

parser.set_defaults(draw_groups=True)
parser.set_defaults(draw_legend=True)
parser.set_defaults(draw_offsets=True)

args = parser.parse_args()

# Create JavaScript runtime
js = execjs.get()

# Compile charter script
with open("charter.js", "r") as charter_js_file:
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
with open(args.file) as example_json:
    print(charter.call(
        "drawSVG",
        example_json.read(),
        json.dumps(options)))

