#!/usr/bin/env python3

import sys
import re
import glob


subst={
	'0gamma'              : '\\iFix{\ensuremath{\gamma}}',
	'1gamma'              : '\\iFix{3\ensuremath{\gamma}}',
	'0'              : '\\iFix{}',
	'1'              : '\\iFix{3}',
	'2'              : '\\iMulti{}',
	'3'              : '\\iMulti{3}',
	'4'              : '\\iGrow{}',
	'5'              : '\\iGrow{3}',
	'6'              : '\\iClassic{}',
	'7'              : '\\iBrute{}',
	'8'              : '\\iGrow{3s}',
	'9'              : '\\iGrow{s}',
	'10'             : '\\iFix{+}',
	'11'             : '\\iFix{3+}',
	'chain0P'        : '\\JumpPointer{\\iGrp{0}}',
	'compact_kvP'    : '\\JumpPointer{\\iCompact{kv}}',
	'binarykP'       : '\\JumpPointer{\\iBinary{k}}',
	'plainchain'     : '\\iGrp{}',
	'compress'     : '\\texttt{compress}',
	'splitbinary'  : '\\texttt{split}',
	'chainbeta'         : '\\iGrp{\\beta}',
	'chain0'         : '\\iGrp{0}',
	'chain10'          : '\\iGrp{10}',
	'chain20'        : '\\iGrp{20}',
	'chain30'       : '\\iGrp{30}',
	'compact'        : '\\iCompact{}',
	'compact_k'      : '\\iCompact{k}',
	'compact_kv'     : '\\iCompact{kv}',
	'ternary'        : '\\iTernary{}',
	'binaryMTF'         : '\\iBinary{mtf}',
	'binary'         : '\\iBinary{}',
	'binaryk'        : '\\iBinary{k}',
	'binarysorted'   : '\\iBinary{s}',
	'judy'           : '\\iJudy{}',
	'hash'           : '\\iHash{}',
	'hashplus'       : '\\iHash{+}',
	'exthash'        : '\\iUnordered{}',
	'rolling'        : '\\iRolling{}',
	'rollingplus'    : '\\iRolling{+}',
	'rolling128'     : '\\iRolling{128}',
	'rolling128plus' : '\\iRolling{128+}',
	'chtD'           : '\\iBucket{}',
	'clearyP'        : '\\iCleary{P}',
	'clearyS'        : '\\iCleary{S}',
	'eliasP'         : '\\iElias{P}',
	'eliasS'         : '\\iElias{S}',
	'grp'            : '\\iGrp{}',
	'layeredP'       : '\\iLayered{P}',
	'layeredS'       : '\\iLayered{S}',
	'78'       : '\\LZEight{}',
	'w'       : '\\LZW{}',
	}

import tempfile


for texfilename in glob.glob('*.tex'):
	print('Processing file ' + texfilename)
	with tempfile.TemporaryFile(mode='w+') as tmpfile:
		with open(texfilename,'r') as texfile:
			for line in texfile.readlines():
				match = re.search('\\\\addlegendentry{([^\\\\}]+)};', line)
				# if not match: 
				# 	tmpfile.write(line)
				# 	continue
				if match:
					keyword = match.group(1)
					assert keyword in subst, 'not in dictionary: ' + keyword
					line = line.replace('\\addlegendentry{%s}' % keyword, '\\addlegendentry{%s}' % subst[keyword])
				match = re.search('JUSTIFY\(([^\\\\}]+)\)', line)
				if match:
					keyword = match.group(1)
					assert keyword in subst, 'not in dictionary: ' + keyword
					line = line.replace('JUSTIFY(%s)' % keyword, '%s' % subst[keyword])
				tmpfile.write(line)
		tmpfile.seek(0)
		with open(texfilename,'w') as texfile:
			texfile.write(tmpfile.read())





