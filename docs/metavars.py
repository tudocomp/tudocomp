#!/usr/bin/env python

"""
Pandoc filter to allow interpolation of metadata fields
into a document.  %{fields} will be replaced by the field's
value, assuming it is of the type MetaInlines or MetaString.

This filter has been modified for use in tudocomp.
Meta variables in link URLs and texts will be replaced.
MetaInlines are no longer considered.
"""

from pandocfilters import toJSONFilter, attributes, Span, Str, Link
import re
import sys

pattern = re.compile('%\{(.+?)\}$')
patternUrl = re.compile('%%7B(.+?)%7D$')

def metavars(key, value, format, meta):
    def resolveMetaString(field):
        result = meta.get(field, {})
        if 'MetaString' in result['t']:
            return result['c']
        else:
            return field

    if key == 'Str':
        m = pattern.match(value)
        if m: return Str(resolveMetaString(m.group(1)))
    elif key == 'Link':
        # replace in URL
        m = patternUrl.match(value[2][0])
        if m: value[2][0] = resolveMetaString(m.group(1))
        # don't return - that will make the filter continue with sub elements
        

if __name__ == "__main__":
    toJSONFilter(metavars)

