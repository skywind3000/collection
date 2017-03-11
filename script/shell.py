#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# shell.py - shell toolbox
#
# NOTE:
# for more information, please see the readme file.
#
#======================================================================
import sys, os, time
import socket

UNIX = sys.platform[:3] != 'win' and 1 or 0

if sys.version_info[0] >= 3:
	xrange = range


#----------------------------------------------------------------------
# call program and returns output (combination of stdout and stderr)
#----------------------------------------------------------------------
def execute(args, shell = False, capture = False):
	import sys, os
	parameters = []
	cmd = None
	if not isinstance(args, list):
		import shlex
		cmd = args
		if sys.platform[:3] == 'win':
			ucs = False
			if sys.version_info[0] < 3:
				if not isinstance(cmd, str):
					cmd = cmd.encode('utf-8')
					ucs = True
			args = shlex.split(cmd.replace('\\', '\x00'))
			args = [ n.replace('\x00', '\\') for n in args ]
			if ucs:
				args = [ n.decode('utf-8') for n in args ]
		else:
			args = shlex.split(cmd)
	for n in args:
		if sys.platform[:3] != 'win':
			replace = { ' ':'\\ ', '\\':'\\\\', '\"':'\\\"', '\t':'\\t', \
				'\n':'\\n', '\r':'\\r' }
			text = ''.join([ replace.get(ch, ch) for ch in n ])
			parameters.append(text)
		else:
			if (' ' in n) or ('\t' in n) or ('"' in n): 
				parameters.append('"%s"'%(n.replace('"', ' ')))
			else:
				parameters.append(n)
	if cmd is None:
		cmd = ' '.join(parameters)
	if sys.platform[:3] == 'win' and len(cmd) > 255:
		shell = False
	if shell and (not capture):
		os.system(cmd)
		return ''
	elif (not shell) and (not capture):
		import subprocess
		if 'call' in subprocess.__dict__:
			subprocess.call(args)
			return ''
	import subprocess
	if 'Popen' in subprocess.__dict__:
		p = subprocess.Popen(args, shell = shell,
				stdin = subprocess.PIPE, stdout = subprocess.PIPE, 
				stderr = subprocess.STDOUT)
		stdin, stdouterr = (p.stdin, p.stdout)
	else:
		p = None
		stdin, stdouterr = os.popen4(cmd)
	text = stdouterr.read()
	stdin.close()
	stdouterr.close()
	if p: p.wait()
	if not capture:
		sys.stdout.write(text)
		sys.stdout.flush()
		return ''
	return text


#----------------------------------------------------------------------
# call subprocess and returns retcode, stdout, stderr
#----------------------------------------------------------------------
def call(args, input_data = None, combine = False):
	import sys, os
	parameters = []
	for n in args:
		if sys.platform[:3] != 'win':
			replace = { ' ':'\\ ', '\\':'\\\\', '\"':'\\\"', '\t':'\\t', \
				'\n':'\\n', '\r':'\\r' }
			text = ''.join([ replace.get(ch, ch) for ch in n ])
			parameters.append(text)
		else:
			if (' ' in n) or ('\t' in n) or ('"' in n): 
				parameters.append('"%s"'%(n.replace('"', ' ')))
			else:
				parameters.append(n)
	cmd = ' '.join(parameters)
	import subprocess
	if 'Popen' in subprocess.__dict__:
		p = subprocess.Popen(args, shell = False,
			stdin = subprocess.PIPE, stdout = subprocess.PIPE,
			stderr = combine and subprocess.STDOUT or subprocess.PIPE)
		stdin, stdout, stderr = p.stdin, p.stdout, p.stderr
		if combine: stderr = None
	else:
		p = None
		if combine is False:
			stdin, stdout, stderr = os.popen3(cmd)
		else:
			stdin, stdout = os.popen4(cmd)
			stderr = None
	if input_data is not None:
		if not isinstance(input_data, bytes):
			input_data = input_data.encode(sys.stdin.encoding, 'ignore')
		stdin.write(input_data)
		stdin.flush()
	exeout = stdout.read()
	if stderr: exeerr = stderr.read()
	else: exeerr = None
	stdin.close()
	stdout.close()
	if stderr: stderr.close()
	retcode = None
	if p:
		retcode = p.wait()
	return retcode, exeout, exeerr


#----------------------------------------------------------------------
# redirect process output to reader(what, text)
#----------------------------------------------------------------------
def redirect(args, reader, combine = True):
	import subprocess
	parameters = []
	for n in args:
		if sys.platform[:3] != 'win':
			replace = { ' ':'\\ ', '\\':'\\\\', '\"':'\\\"', '\t':'\\t', \
				'\n':'\\n', '\r':'\\r' }
			text = ''.join([ replace.get(ch, ch) for ch in n ])
			parameters.append(text)
		else:
			if (' ' in n) or ('\t' in n) or ('"' in n): 
				parameters.append('"%s"'%(n.replace('"', ' ')))
			else:
				parameters.append(n)
	cmd = ' '.join(parameters)
	if 'Popen' in subprocess.__dict__:
		p = subprocess.Popen(args, shell = False,
			stdin = subprocess.PIPE, stdout = subprocess.PIPE,
			stderr = combine and subprocess.STDOUT or subprocess.PIPE)
		stdin, stdout, stderr = p.stdin, p.stdout, p.stderr
		if combine: stderr = None
	else:
		p = None
		if combine is False:
			stdin, stdout, stderr = os.popen3(cmd)
		else:
			stdin, stdout = os.popen4(cmd)
			stderr = None
	stdin.close()
	while 1:
		text = stdout.readline()
		if text == b'' or text == '':
			break
		reader('stdout', text)
	while stderr != None:
		text = stderr.readline()
		if text == b'' or text == '':
			break
		reader('stderr', text)
	stdout.close()
	if stderr: stderr.close()
	retcode = None
	if p:
		retcode = p.wait()
	return retcode


#----------------------------------------------------------------------
# get short path in windows
#----------------------------------------------------------------------
def pathshort(path):
	if path is None:
		return None
	path = os.path.abspath(path)
	if sys.platform[:3] != 'win':
		return path
	kernel32 = None
	textdata = None
	GetShortPathName = None
	try:
		import ctypes
		kernel32 = ctypes.windll.LoadLibrary("kernel32.dll")
		textdata = ctypes.create_string_buffer(b'\000' * 1034)
		GetShortPathName = kernel32.GetShortPathNameA
		args = [ ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int ]
		GetShortPathName.argtypes = args
		GetShortPathName.restype = ctypes.c_uint32
	except: 
		pass
	if not GetShortPathName:
		return path
	if not isinstance(path, bytes):
		path = path.encode(sys.stdout.encoding, 'ignore')
	retval = GetShortPathName(path, textdata, 1034)
	shortpath = textdata.value
	if retval <= 0:
		return ''
	if isinstance(path, bytes):
		shortpath = shortpath.decode(sys.stdout.encoding, 'ignore')
	return shortpath


#----------------------------------------------------------------------
# mkdir -p
#----------------------------------------------------------------------
def mkdir(path):
	import sys, os
	unix = sys.platform[:3] != 'win' and True or False
	path = os.path.abspath(path)
	if os.path.exists(path):
		return False
	name = ''
	part = os.path.abspath(path).replace('\\', '/').split('/')
	if unix:
		name = '/'
	if (not unix) and (path[1:2] == ':'):
		part[0] += '/'
	for n in part:
		name = os.path.abspath(os.path.join(name, n))
		if not os.path.exists(name):
			os.mkdir(name)
	return True


#----------------------------------------------------------------------
# rm -rf
#----------------------------------------------------------------------
def rmtree(path, ignore_error = False, onerror = None):
	import shutil
	shutil.rmtree(path, ignore_error, onerror)


#----------------------------------------------------------------------
# absolute path
#----------------------------------------------------------------------
def abspath(path, resolve = False):
	if path is None:
		return None
	if '~' in path:
		path = os.path.expanduser(path)
	path = os.path.abspath(path)
	if not UNIX:
		return path.lower().replace('\\', '/')
	if resolve:
		return os.path.abspath(os.path.realpath(path))
	return path


#----------------------------------------------------------------------
# find files
#----------------------------------------------------------------------
def find(path, extnames = None):
	result = []
	if extnames:
		if UNIX == 0:
			extnames = [ n.lower() for n in extnames ]
		extnames = tuple(extnames)
	for root, _, files in os.walk(path):
		for name in files:
			if extnames:
				ext = os.path.splitext(name)[-1]
				if UNIX == 0:
					ext = ext.lower()
				if not ext in extnames:
					continue
			result.append(os.path.abspath(os.path.join(root, name)))
	return result


#----------------------------------------------------------------------
# which
#----------------------------------------------------------------------
def which(name, prefix = None, postfix = None):
	if not prefix:
		prefix = []
	if not postfix:
		postfix = []
	PATH = os.environ.get('PATH', '').split(UNIX and ':' or ';')
	search = prefix + PATH + postfix
	for path in search:
		fullname = os.path.join(path, name)
		if os.path.exists(fullname):
			return fullname
	return None

#----------------------------------------------------------------------
# search exe
#----------------------------------------------------------------------
def search_exe(exename):
	path = which(exename)
	if path is None:
		return None
	return pathshort(path)


#----------------------------------------------------------------------
# file content load/save
#----------------------------------------------------------------------
def file_content_load(filename, mode = 'r'):
	try:
		fp = open(filename, mode)
		content = fp.read()
		fp.close()
	except:
		content = None
	return content

def file_content_save(filename, content, mode = 'w'):
	try:
		fp = open(filename, mode)
		fp.write(content)
		fp.close()
	except:
		return False
	return True

def load_config(path):
	import json
	try:
		text = file_content_load(path, 'rb')
		if text is None:
			return None
		if sys.version_info[0] < 3:
			return json.loads(text, encoding = "utf-8")
		else:
			text = text.decode('utf-8', 'ignore')
			return json.loads(text)
	except:
		return None
	return None

def save_config(path, obj):
	import json
	if sys.version_info[0] < 3:
		text = json.dumps(obj, indent = 4, encoding = "utf-8") + '\n'
	else:
		text = json.dumps(obj, indent = 4) + '\n'
		text = text.encode('utf-8', 'ignore')
	if not file_content_save(path, text, 'wb'):
		return False
	return True


#----------------------------------------------------------------------
# http_request
#----------------------------------------------------------------------
def http_request(url, timeout = 10, data = None, post = False):
	if sys.version_info[0] >= 3:
		import urllib
		import urllib.parse
		import urllib.request
		import urllib.error
		if data is not None:
			if isinstance(data, dict):
				data = urllib.parse.urlencode(data)
		if not post:
			if data is None:
				req = urllib.request.Request(url)
			else:
				mark = '?' in url and '&' or '?'
				req = urllib.request.Request(url + mark + data)
		else:
			data = data is not None and data or ''
			if not isinstance(data, bytes):
				data = data.encode('utf-8', 'ignore')
			req = urllib.request.Request(url, data)
		try:
			res = urllib.request.urlopen(req, timeout = timeout)
		except urllib.error.HTTPError as e:
			return e.code, str(e.message)
		except urllib.error.URLError as e:
			return -1, str(e)
		except socket.timeout:
			return -2, 'timeout'
		content = res.read()
	else:
		import urllib2
		import urllib
		if data is not None:
			if isinstance(data, dict):
				data = urllib.urlencode(data, encoding = 'utf-8')
		if not post:
			if data is None:
				req = urllib2.Request(url)
			else:
				mark = '?' in url and '&' or '?'
				req = urllib2.Request(url + mark + data)
		else:
			req = urllib2.Request(url, data is not None and data or '')
		try:
			res = urllib2.urlopen(req, timeout = timeout)
			content = res.read()
		except urllib2.HTTPError as e:
			return e.code, str(e.message)
		except urllib2.URLError as e:
			return -1, str(e)
		except socket.timeout:
			return -2, 'timeout'
	return 200, content


#----------------------------------------------------------------------
# request with retry
#----------------------------------------------------------------------
def request_safe(url, timeout = 10, retry = 3, verbose = True, delay = 1):
	for i in xrange(retry):
		if verbose:
			print('%s: %s'%(i == 0 and 'request' or 'retry', url))
		time.sleep(delay)
		code, content = http_request(url, timeout)
		if code == 200:
			return content
	return None


#----------------------------------------------------------------------
# request json rpc
#----------------------------------------------------------------------
def json_rpc_post(url, message, timeout = 10):
	import json
	data = json.dumps(message)
	code, content = http_request(url, timeout, data, True)
	if code == 200:
		content = json.loads(content)
	return code, content


#----------------------------------------------------------------------
# timestamp
#----------------------------------------------------------------------
def timestamp(ts = None, onlyday = False):
	import time
	if not ts: ts = time.time()
	if onlyday:
		time.strftime('%Y%m%d', time.localtime(ts))
	return time.strftime('%Y%m%d%H%M%S', time.localtime(ts))


#----------------------------------------------------------------------
# timestamp
#----------------------------------------------------------------------
def readts(ts, onlyday = False):
	if onlyday: ts += '000000'
	try: return time.mktime(time.strptime(ts, '%Y%m%d%H%M%S'))
	except: pass
	return 0


#----------------------------------------------------------------------
# DNS Check
#----------------------------------------------------------------------
def checkip():
	url = 'http://ddns.oray.com/checkip'
	return http_request(url)


#----------------------------------------------------------------------
# DNS update
#----------------------------------------------------------------------
def ddns_oray_up(user, passwd, hostname, ip = None):
	user = user.replace('/', ' ').replace(':', ' ').replace('@', '@')
	passwd = passwd.replace('/', ' ').replace(':', ' ').replace('@', '@')
	url = 'http://%s:%s@ddns.oray.com/ph/update?hostname=%s'
	url = url%(user, passwd, hostname)
	if ip:
		url += '&myip=' + ip
	import urllib
	return urllib.urlopen(url).read()


#----------------------------------------------------------------------
# DNS update
#----------------------------------------------------------------------
def ddns_noip_up(user, passwd, hostname, ip = None):
	user = user.replace('/', ' ').replace(':', ' ').replace('@', '@')
	passwd = passwd.replace('/', ' ').replace(':', ' ').replace('@', '@')
	url = 'http://%s:%s@dynupdate.no-ip.com/nic/update?hostname=%s'
	url = url%(user, passwd, hostname)
	if ip:
		url += '&myip=' + ip
	import urllib
	return urllib.urlopen(url).read()


#----------------------------------------------------------------------
# html escape
#----------------------------------------------------------------------
def text2html(s):
	import cgi
	return cgi.escape(s, True).replace('\n', "<br />\n")


#----------------------------------------------------------------------
# simple html2text
#----------------------------------------------------------------------
def html2text (html):
	part = []
	pos = 0
	while 1:
		f1 = html.find('<', pos)
		if f1 < 0:
			part.append((0, html[pos:]))
			break
		f2 = html.find('>', f1)
		if f2 < 0:
			part.append((0, html[pos:]))
			break
		text = html[pos:f1]
		flag = html[f1:f2+1]
		pos = f2 + 1
		if text:
			part.append((0, text))
		if flag:
			part.append((1, flag))
	output = ''
	for mode, text in part:
		if mode == 0:
			text = text.lstrip()
			text = text.replace('&nbsp;', ' ').replace('&gt;', '>')
			text = text.replace('&lt;', '<').replace('&amp;', '&')
			output += text
		else:
			text = text.strip()
			tiny = text.replace(' ', '')
			if tiny in ('</p>', '<p/>', '<br>', '</br>', '<br/>'):
				output += '\n'
			elif tiny in ('</tr>', '<tr/>', '</h1>', '</h2>', '</h3>'):
				output += '\n'
			elif tiny in ('</td>', '<td/>'):
				output += ' '
	return output


#----------------------------------------------------------------------
# 输出二进制
#----------------------------------------------------------------------
def print_binary(data, char = False):
	content = ''
	charset = ''
	lines = []
	for i in xrange(len(data)):
		ascii = ord(data[i])
		if i % 16 == 0: content += '%04X  '%i
		content += '%02X'%ascii
		content += ((i & 15) == 7) and '-' or ' '
		if (ascii >= 0x20) and (ascii < 0x7f): charset += data[i]
		else: charset += '.'
		if i % 16 == 15: 
			lines.append(content + ' ' + charset)
			content, charset = '', ''
	if len(content) < 56: content += ' ' * (54 - len(content))
	lines.append(content + ' ' + charset)
	limit = char and 100 or 54
	for n in lines:
		print(n[:limit])
	return 0



#----------------------------------------------------------------------
# 输出调用栈
#----------------------------------------------------------------------
def print_traceback():
	import StringIO, traceback
	sio = StringIO.StringIO()
	traceback.print_exc(file = sio)
	for line in sio.getvalue().split('\n'):
		print(line)
	return 0


#----------------------------------------------------------------------
# returns last n lines of file
#----------------------------------------------------------------------
def tail(filename, need = 10):
	if isinstance(filename, str) or isinstance(filename, unicode):
		try:
			fp = open(filename, 'rb+')
		except:
			return None
	else:
		fp = filename
	lines = []
	fp.seek(0, os.SEEK_END)
	length = fp.tell()
	block = 1024
	position = length
	while 1:
		newpos = position - block
		if newpos < 0: newpos = 0
		canread = position - newpos
		if canread <= 0: 
			break
		fp.seek(newpos, os.SEEK_SET)
		text = fp.read(canread)
		#print 'read', repr(text)
		if len(text) == 0 or text is None:
			break
		newlines = text.split('\n')
		if len(lines) > 0:
			newlines[-1] = newlines[-1] + lines[0]
			lines = lines[1:]
		lines = newlines + lines
		if len(lines) > need + 1:
			break
		position = newpos
	if fp != filename:
		try:
			fp.close()
			fp = None
		except:
			pass
	return '\n'.join(lines[-need:])


#----------------------------------------------------------------------
# returns first n lines of file
#----------------------------------------------------------------------
def head(filename, need = 10):
	if isinstance(filename, str) or isinstance(filename, unicode):
		try:
			fp = open(filename, 'rb+')
		except:
			return None
	else:
		fp = filename
	lines = []
	block = 1024
	while 1:
		canread = block
		if canread <= 0: 
			break
		text = fp.read(canread)
		#print 'read', repr(text)
		if len(text) == 0 or text is None:
			break
		newlines = text.split('\n')
		if len(lines) > 0:
			lines[-1] = lines[-1] + newlines[0]
			newlines = newlines[1:]
		lines = lines + newlines
		if len(lines) > need + 1:
			break
	if fp != filename:
		try:
			fp.close()
			fp = None
		except:
			pass
	return '\n'.join(lines[:need])


#----------------------------------------------------------------------
# returns lines (characters end up with '\n'), returns next position
#----------------------------------------------------------------------
def between(filename, start = 0, stop = -1):
	if isinstance(filename, str) or isinstance(filename, unicode):
		try:
			fp = open(filename, 'rb+')
		except:
			return -1, None
	else:
		fp = filename
	if stop < 0:
		fp.seek(0, os.SEEK_END)
		stop = fp.tell()
	fp.seek(start, os.SEEK_SET)
	need = stop - start
	text = fp.read(need)
	if fp != filename:
		try:
			fp.close()
			fp = None
		except:
			pass
	if len(text) == 0:
		return start, None
	pos = text.rfind('\n')
	if pos < 0:
		return start, None
	text = text[:pos + 1]
	return start + len(text), text


#----------------------------------------------------------------------
# SVN
#----------------------------------------------------------------------
_EXE_SVN = search_exe('svn' + (UNIX == 0 and '.exe' or ''))
_EXE_SVNLOOK = search_exe('svnlook' + (UNIX == 0 and '.exe' or ''))

def svn(args, shell = False, capture = False):
	if not _EXE_SVN:
		sys.stderr.write('not find svn in environ PATH')
		return None
	return execute([_EXE_SVN] + args, shell, capture)

def svnlook(args, shell = False, capture = False):
	if not _EXE_SVNLOOK:
		sys.stderr.write('not find svnlook in environ PATH')
		return None
	return execute([_EXE_SVNLOOK] + args, shell, capture)

def svn_cat(url, username = None, password = None, revision = None):
	if not _EXE_SVN:
		sys.stderr.write('not find svn in environ PATH')
		return None
	args = [_EXE_SVN, 'cat']
	if username != None:
		args += ['--username', username]
	if password != None:
		args += ['--password', password]
	if revision != None:
		args += ['--revision', revision]
	args += ['--non-interactive', '--no-auth-cache']
	args += [url]
	code, out, err = call(args)
	if code != 0:
		sys.stderr.write('svn cat error:\n%s\n'%err)
		sys.stderr.flush()
		return None
	return out

def svnlook_youngest(repos):
	result = svnlook(['youngest', repos], capture = True)
	if result is None:
		return None
	revision = -1
	try:
		revision = int(result)
	except:
		revision = None
	return revision

def svnlook_cat(repos, filepath, revision = None):
	if not _EXE_SVNLOOK:
		sys.stderr.write('not find svnlook in environ PATH')
		return None
	args = [ _EXE_SVNLOOK, 'cat' ]
	if revision != None:
		args += [ '-r', str(revision) ]
	code, out, err = call(args + [repos, filepath])
	if code != 0:
		sys.stderr.write('svnlook error:\n%s\n'%err)
		sys.stderr.flush()
		return None
	return out


#----------------------------------------------------------------------
# expand tab
#----------------------------------------------------------------------
def expandtab (tabsize, text):
	output = []
	for line in text.split('\r\n'):
		line = line.rstrip()
		text = ''
		pos = 0
		size = abs(tabsize)
		if tabsize > 0:
			for ch in line:
				if ch != '\t':
					text += ch
					pos += 1
				else:
					inc = size - (pos % size)
					pos += inc
					text += ' ' * inc
		elif tabsize < 0:
			offset = 0
			for ch in line:
				if ch == ' ':
					pos += 1
				elif ch == '\t':
					inc = size - (pos % size)
					pos += inc
				else:
					break
				offset += 1
			count = pos
			head = '\t' * (count / size) + (' ' * (count % size))
			text = head + line[offset:]
		else:
			text = line
		output.append(text)
	return '\n'.join(output)


#----------------------------------------------------------------------
# plutil
#----------------------------------------------------------------------
def plutil(*args):
	args = [ n for n in args ]
	locate = None
	if sys.platform[:3] == 'win':
		place = []
		p1 = os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')
		p2 = os.environ.get('ProgramFiles', 'C:/Program Files')
		p1 = os.path.join(p1, 'Common Files')
		p2 = os.path.join(p2, 'Common Files')
		if os.path.exists(p1) and (not p1 in place):
			place.append(p1)
		if os.path.exists(p2) and (not p2 in place):
			place.append(p2)
		p1 = os.environ.get('CommonProgramFiles(x86)', '')
		p2 = os.environ.get('CommonProgramFiles', '')
		p3 = os.environ.get('CommonProgramW6432', '')
		if p1 == '':
			p1 = 'C:/Program Files (x86)/Common Files'
		if p2 == '':
			p2 = 'C:/Program Files/Common Files'
		if not p1 in place:
			place.append(p1)
		if not p2 in place:
			place.append(p2)
		if p3 and (not p3 in place):
			place.append(p3)
		name = 'Apple/Apple Application Support/plutil.exe'
		for home in place:
			home = os.path.join(home, name)
			if os.path.exists(home):
				locate = home
				break
		if locate is None:
			locate = which('plutil.exe')
		if locate is None:
			raise IOError('can not find plutil.exe, please install iTunes')
			return -1
		locate = pathshort(locate)
	else:
		locate = which('plutil')
		if locate is None:
			raise IOError('can not find plutil')
	code = os.spawnv(os.P_WAIT, locate, ['plutil'] + args)
	return code


#----------------------------------------------------------------------
# plist operations
#----------------------------------------------------------------------
def plist_load(filename):
	import plistlib
	fp = open(filename, 'rb')
	content = fp.read(8)
	fp.close()
	if content == 'bplist00':
		import warnings
		warnings.filterwarnings("ignore")
		tmpname = os.tempnam(None, 'plist.')
		plutil('-convert', 'xml1', '-o', tmpname, filename)
		data = plistlib.readPlist(tmpname)
		os.remove(tmpname)
		return data
	data = plistlib.readPlist(filename)
	return data

def plist_save(filename, data, binary = False):
	import plistlib
	if not binary:
		plistlib.writePlist(data, filename)
		return 0
	import warnings
	warnings.filterwarnings("ignore")
	tmpname = os.tempnam(None, 'plist.')
	plistlib.writePlist(data, tmpname)
	plutil('-convert', 'binary1', '-o', filename, tmpname)
	os.remove(tmpname)
	return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':
	def test1():
		args = ['test 2/child.exe', 'hello world ! "no i"', 'asdf\\ asdf']
		n = execute(args, False, True)
		print('output:')
		print(n.decode(sys.stdout.encoding))
		print('-' * 20)
		import subprocess
		subprocess.call(args)
	def test2():
		cmd = '"test 2/child.exe" "Hello World ! "no"" "asdf\\ asdf" -I"d:\program files\python25"'
		print(execute(cmd, False, False))
	def test3():
		print(pathshort('c:\\program files'))
	def test4():
		obj = { 'x':1, u'y\u6797':2, 'array':[1,2,3,5], 'dict':{'a':3, 'b':4}}
		import json
		text = json.dumps(obj, indent = 4)
		print(type(text))
		n = json.loads(text, encoding = 'utf-8')
		print(n)
		save_config('e:/abc.json', obj)
		n = load_config('e:/abc.json')
		print(n)
		return 0
	def test5():
		for n in find('e:/lab/casuald/src', ['.cpp']):
			print(n)
	def test6():
		redirect(['d:/dev/python27/python', 'e:/lab/timer.py'], lambda n, x: sys.stdout.write(x.decode('gbk')))
	def test7():
		data = plist_load('e:/com.googlecode.iterm2.plist')
		#print data
		for bm in data['New Bookmarks']:
			print(bm['Name'])
		keymap = data['New Bookmarks'][0]['Keyboard Map']
		for key in keymap:
			print(key, keymap[key])
		plist_save('e:/com.xml.plist', data)
		plist_save('e:/com.binary.plist', data, True)
		return 0
	def test8():
		uri = 'http://192.168.1.3/web/test.php'
		values = {'data':u'\u6797\u6798', 'suck':9999}
		info = {}
		req = http_request(uri, post = True, data = values)
		print(req[0])
		print(req[1].decode('utf-8'))
		return 0
	def test9():
		req = json_rpc_post('https://api.shanbay.com/oauth2/token/', {'id':1000})
		print(req)
		return 0
	# shcmd.py
	# shlib.py
	# shlib.py
	# coresh.py system.py
	# shkit.py shset.py osset.py kitos.py 
	# oskit.py shellkit.py shellos shells.py
	test8()




