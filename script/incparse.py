#! /usr/bin/python
# -*- coding: utf-8 -*-
#======================================================================
#
# prehead.py - search header files related to the C/C++ source file
#
# history of this file:
# Sep.23 2005  skywind  implemented pm_source_filter
# Sep.24 2005  skywind  implemented pm_search_reference_header
# Sep.25 2005  skywind  implemented pm_parse_source
#
#======================================================================


import sys
import os

#----------------------------------------------------------------------
# pm_search_header_files
#----------------------------------------------------------------------
def pm_search_header_files(path):
	headers = []
	for root, dirs, files in os.walk(path):
		for name in files:
			if ((name[-2:] == '.h') or (name[-2:] == '.H')):
				headers.append(os.path.os.path.join(root, name))
	return headers

#----------------------------------------------------------------------
# pm_source_filter
#----------------------------------------------------------------------
def pm_source_filter(text):
	content = text
	spaces = [' ', '\n', '\t', '\r']
	srctext = ''
	memo = 0
	i = 0
	while i < len(content):
		if memo == 0:		# 正文中
			if content[i:i+2] == '/*':
				srctext = srctext + '``'
				i = i + 2
				memo = 1
				continue
			if content[i:i+2] == '//':
				srctext = srctext + '``'
				i = i + 2
				while (i < len(content)) and (content[i] != '\n'):
					if content[i] in spaces:
						srctext = srctext + content[i]
						i = i + 1
						continue						
					srctext = srctext + '`'
					i = i + 1
				continue
			if content[i] == '\"':
				srctext = srctext + '\"'
				i = i + 1
				memo = 2
				continue
			if content[i] == '\'':
				srctext = srctext + '\''
				i = i + 1
				memo = 3
				continue
			srctext = srctext + content[i]
		elif memo == 1:		# 注释中
			if content[i:i+2] == '*/':
				srctext = srctext + '``'
				i = i + 2
				memo = 0
				continue
			if content[i] in spaces:
				srctext = srctext + content[i]
				i = i + 1
				continue
			srctext = srctext + '`'
		elif memo == 2:		# 字符串中
			if content[i:i+2] == '\\\"':
				srctext = srctext + '$$'
				i = i + 2
				continue
			if content[i] == '\"':
				srctext = srctext + '\"'
				i = i + 1
				memo = 0
				continue
			if content[i] in spaces:
				srctext = srctext + content[i]
				i = i + 1
				continue
			srctext = srctext + '$'
		elif memo == 3:		# 字符中
			if content[i:i+2] == '\\\'':
				srctext = srctext + '$$'
				i = i + 2
				continue
			if content[i] == '\'':
				srctext = srctext + '\''
				i = i + 1
				memo = 0
				continue
			if content[i] in spaces:
				srctext = srctext + content[i]
				i = i + 1
				continue
			srctext = srctext + '$'
		i = i + 1
	return srctext

#----------------------------------------------------------------------
# pm_search_reference_header
#----------------------------------------------------------------------
def pm_search_reference_header(source, heads):
	content = ''
	del heads[:]
	try:
		fp = open(source, "r")
	except:
		return ''
	number = 1

	for line in fp:
		content = content + line
		number = number + 1
	fp.close()

	outtext = ''
	for i in xrange(0,len(content)):
		if content[i] != '\r':
			outtext = outtext + content[i]
	content = outtext

	srctext = pm_source_filter(content)
	blank = [ ' ', '\t', '\r', '\n', '/', '*', '$' ]
	space = [ ' ', '\t', '/', '*', '$' ]

	length = len(srctext)
	start = 0
	endup =-1
	number = 0

	while (start >= 0) and (start < length):
		start = endup + 1
		endup = srctext.find('\n', start)
		if (endup < 0):
			endup = length
		number = number + 1

		line = srctext[start:endup]
		offset1 = srctext.find('#', start, endup)
		offset2 = srctext.find('include', offset1, endup)
		offset3 = srctext.find('\"', offset2, endup)
		offset4 = srctext.find('\"', offset3 + 1, endup)

		if (offset1 < 0) or (offset2 < 0) or (offset3 < 0) or (offset4 < 0):
			continue
		check_range = [i for i in xrange(start, offset1)]
		check_range = check_range + [i for i in xrange(offset1 + 1, offset2)]
		check_range = check_range + [i for i in xrange(offset2 + 7, offset3)]
		check = 1

		for i in check_range:
			if not (srctext[i] in [' ', '`' ]):
				check = 0
		if check != 1:
			continue
		
		name = content[offset3+1:offset4]
		heads.append([name, offset1, offset4, number])

	return content


#----------------------------------------------------------------------
# pm_parse_source
#----------------------------------------------------------------------
def pm_parse_source(filename, history_headers):
	headers = []
	outtext = ''
	if not os.path.exists(filename):
		sys.stderr.write('can not open %s\n'%(filename))
		return outtext
	content = pm_search_reference_header(filename, headers)
	save_cwd = os.getcwd()
	file_cwd = os.path.dirname(filename)
	if file_cwd == '':
		file_cwd = '.'
	os.chdir(file_cwd)
	for head in headers:
		if not os.path.exists(head[0]):
			sys.stderr.write('%s:%d: can not open "%s"\n'%(filename, head[3], head[0]))
			os.chdir(save_cwd)
			return outtext
	offset = 0
	for head in headers:
		name = os.path.realpath(head[0])
		name = os.path.normcase(os.path.normpath(name))
		if not (name in history_headers):
			history_headers.append(name)
			position = len(history_headers) - 1
			text = pm_parse_source(name, history_headers)
			del history_headers[position]
			history_headers.append(name)
			outtext = outtext + content[offset:head[1]] + '\n'
			outtext = outtext + '/*:: <%s> ::*/\n'%(head[0])
			outtext = outtext + text + '\n/*:: </:%s> ::*/\n'%(head[0])
			offset = head[2] + 1
		else:
			outtext = outtext + content[offset:head[1]] + '\n'
			outtext = outtext + '/*:: skip including "%s" ::*/\n'%(head[0])
			offset = head[2] + 1
	outtext = outtext + content[offset:]
	os.chdir(save_cwd)
	return outtext


#----------------------------------------------------------------------
# clean memo
#----------------------------------------------------------------------
def pm_cleanup_memo(text):
	content = text
	outtext = ''
	srctext = pm_source_filter(content)
	space = [ ' ', '\t', '`' ]
	start = 0
	endup = -1
	sized = len(srctext)
	while (start >= 0) and (start < sized):
		start = endup + 1
		endup = srctext.find('\n', start)
		if endup < 0:
			endup = sized
		empty = 1
		memod = 0
		for i in xrange(start, endup):
			if not (srctext[i] in space):
				empty = 0
			if srctext[i] == '`':
				memod = 1
		if empty and memod:
			continue
		for i in xrange(start, endup):
			if srctext[i] != '`':
				outtext = outtext + content[i]
		outtext = outtext + '\n'
	return outtext

#----------------------------------------------------------------------
# main program
#----------------------------------------------------------------------
if __name__ == '__main__':
	history = []
	argc = len(sys.argv)
	if argc == 1:
		print 'parametes: source [output]'
	elif argc == 2:
		pm_parse_source(sys.argv[1], history)
		for name in history:
			print name
	elif sys.argv[2] == '-':
		print pm_parse_source(sys.argv[1], history)
	else:
		source = pm_parse_source(sys.argv[1], history)
		if argc > 3:
			source = pm_cleanup_memo(source)
		try:
			file = open(sys.argv[2], "w")
			file.write(source)
			file.close()
		except:
			sys.stderr.write('write %s error\n'%(sys.argv[2]))
		source = ''


