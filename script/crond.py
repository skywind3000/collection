#! /usr/bin/env python2
# -*- coding: utf-8 -*-
#======================================================================
# 
# crontab.py - yet another crond implementation in python
#
# Maybe some time, you want a independent crond scheduler
# when you don't want to disturb system builtin crontab (eg, some nas
# /embed linux won't let you do it), or maybe, you just want a crontab 
# on Windows.
#
#======================================================================
import sys, time, os

#----------------------------------------------------------------------
# crontab
#----------------------------------------------------------------------
class crontab (object):

	def __init__ (self):
		self.daynames = {}
		self.monnames = {}
		DAYNAMES = ('sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat')
		MONNAMES = ('jan', 'feb', 'mar', 'apr', 'may', 'jun')
		MONNAMES = MONNAMES + ('jul', 'aug', 'sep', 'oct', 'nov', 'dec')
		for x in xrange(7):
			self.daynames[DAYNAMES[x]] = x
		for x in xrange(12):
			self.monnames[MONNAMES[x]] = x + 1
		self.timestamp = 0
		self.lastmin = -1
	
	# check_atom('0-10/2', (0, 59), 4) -> True
	# check_atom('0-10/2', (0, 59), 5) -> False
	def check_atom (self, text, minmax, value):
		if value < minmax[0] or value > minmax[1]:
			return None
		if minmax[0] > minmax[1]:
			return None
		text = text.strip('\r\n\t ')
		if text == '*':
			return True
		if text.isdigit():
			try: x = int(text)
			except: return None
			if x < minmax[0] or x > minmax[1]:
				return None
			if value == x:
				return True
			return False
		increase = 1
		if '/' in text:
			part = text.split('/')
			if len(part) != 2:
				return None
			try: increase = int(part[1])
			except: return None
			if increase < 1:
				return None
			text = part[0]
		if text == '*':
			x, y = minmax
		elif text.isdigit():
			try: x = int(text)
			except: return None
			if x < minmax[0] or x > minmax[1]:
				return None
			y = minmax[1]
		else:
			part = text.split('-')
			if len(part) != 2:
				return None
			try:
				x = int(part[0])
				y = int(part[1])
			except:
				return None
		if x < minmax[0] or x > minmax[1]:
			return None
		if y < minmax[0] or y > minmax[1]:
			return None
		if x <= y:
			if value >= x and value <= y:
				if increase == 1 or (value - x) % increase == 0:
					return True
			return False
		else:
			if value <= y:
				if increase == 1 or (value - minmax[0]) % increase == 0:
					return True
			elif value >= x:
				if increase == 1 or (value - x) % increase == 0:
					return True
			return False
		return None

	# 解析带有月份/星期缩写，逗号的项目
	def check_token (self, text, minmax, value):
		for text in text.strip('\r\n\t ').split(','):
			text = text.lower()
			for x, y in self.daynames.iteritems():
				text = text.replace(x, str(y))
			for x, y in self.monnames.iteritems():
				text = text.replace(x, str(y))
			hr = self.check_atom(text, minmax, value)
			if hr == None:
				return None
			if hr:
				return True
		return False
	
	# 切割 crontab配置
	def split (self, text):
		text = text.strip('\r\n\t ')
		need = text[:1] == '@' and 1 or 5
		data, mode, line = [], 1, ''
		for i in xrange(len(text)):
			ch = text[i]
			if mode == 1:
				if ch.isspace():
					data.append(line)
					line = ''
					mode = 0
				else:
					line += ch
			else:
				if not ch.isspace():
					if len(data) == need:
						data.append(text[i:])
						line = ''
						break
					line = ch
					mode = 1
		if line:
			data.append(line)
		if len(data) < need:
			return None
		if len(data) == need:
			data.append('')
		return tuple(data[:need + 1])
	
	# 传入如(2013, 10, 21, 16, 35)的时间，检查 crontab是否该运行
	def check (self, text, datetuple, runtimes = 0):
		data = self.split(text)
		if not data:
			return None
		if len(data) == 2:
			entry = data[0].lower()
			if entry == '@reboot':
				return (runtimes == 0) and True or False
			if entry == '@shutdown':
				return (runtimes < 0) and True or False
			if entry in ('@yearly', '@annually'):
				data = self.split('0 0 1 1 * ' + data[1])
			elif entry == '@monthly':
				data = self.split('0 0 1 * * ' + data[1])
			elif entry == '@weekly':
				data = self.split('0 0 * * 0 ' + data[1])
			elif entry == '@daily':
				data = self.split('0 0 * * * ' + data[1])
			elif entry == '@midnight':
				data = self.split('0 0 * * * ' + data[1])
			elif entry == '@hourly':
				data = self.split('0 * * * * ' + data[1])
			else:
				return None
			if data == None:
				return None
		if len(data) != 6 or len(datetuple) != 5:
			return None
		year, month, day, hour, mins = datetuple
		if type(month) == type(''):
			month = month.lower()
			for x, y in self.monnames.iteritems():
				month = month.replace(x, str(y))
			try:
				month = int(month)
			except:
				return None
		import datetime
		try:
			x = datetime.datetime(year, month, day).strftime("%w")
			weekday = int(x)
		except:
			return None
		hr = self.check_token(data[0], (0, 59), mins)
		if not hr: return hr
		hr = self.check_token(data[1], (0, 23), hour)
		if not hr: return hr
		hr = self.check_token(data[2], (0, 31), day)
		if not hr: return hr
		hr = self.check_token(data[3], (1, 12), month)
		if not hr: return hr
		hr = self.check_token(data[4], (0, 6), weekday)
		if not hr: return hr
		return True
	
	# 调用 crontab程序
	def call (self, command):
		import subprocess
		return subprocess.Popen(command, shell = True)

	# 读取crontab文本，如果错误则返回行号，否则返回任务列表
	def read (self, content, times = 0):
		schedule = []
		if content[:3] == '\xef\xbb\xbf':	# remove BOM+
			content = content[3:]
		ln = 0
		for line in content.split('\n'):
			line = line.strip('\r\n\t ')
			ln += 1
			if line[:1] in ('#', ';', ''): 
				continue
			hr = self.split(line)
			if hr == None:
				return ln
			obj = {}
			obj['cron'] = line
			obj['command'] = hr[-1]
			obj['runtimes'] = times
			obj['lineno'] = ln
			schedule.append(obj)
		return schedule
	
	# 单位时间触发，返回成功调度的任务
	def interval (self, schedule, timestamp, workdir = '.', env = {}):
		runlist = []
		if not schedule:
			return []
		savedir = os.getcwd()
		if timestamp - self.timestamp >= 300:
			self.timestamp = long(timestamp)
			self.timestamp = self.timestamp - (self.timestamp % 10)
		while timestamp >= self.timestamp:
			now = time.localtime(self.timestamp)
			if now.tm_min != self.lastmin:
				datetuple = now[:5]
				for obj in schedule:
					if self.check(obj['cron'], datetuple, 1):
						if workdir:
							os.chdir(workdir)
						command = obj['command']
						execute = command
						for k, v in env.iteritems():
							execute = execute.replace('$(%s)'%k, str(v))
						self.call(execute)
						if workdir:
							os.chdir(savedir)
						obj['runtimes'] += 1
						t = (obj['cron'], command, execute, obj['lineno'])
						runlist.append(t + (obj['runtimes'], ))
				self.lastmin = now.tm_min
			self.timestamp += 10
		return runlist

	# 开启关闭时调用，返回在关闭时调度的任务，开启mode=0，关闭 mode=1
	def event (self, schedule, mode, workdir = '.', env = {}, wait = False):
		runlist = []
		pids = []
		savedir = os.getcwd()
		for obj in schedule:
			if self.check(obj['cron'], (0, 0, 0, 0, 0), mode):
				if workdir:
					os.chdir(workdir)
				command = obj['command']
				execute = command
				for k, v in env.iteritems():
					execute = execute.replace('$(%s)'%k, str(v))
				pids.append(self.call(execute))
				if workdir:
					os.chdir(savedir)
				obj['runtimes'] += 1
				t = (obj['cron'], command, execute, obj['lineno'])
				runlist.append(t + (obj['runtimes'], ))
		if wait:
			for pid in pids:
				pid.wait()
		return runlist


#----------------------------------------------------------------------
# daemon
#----------------------------------------------------------------------
def daemon():
	if sys.platform[:3] == 'win':
		return -1
	try:
		if os.fork() > 0: os._exit(0)
	except OSError, error:
		os._exit(1)
	os.setsid()
	os.umask(0)
	try:
		if os.fork() > 0: os._exit(0)
	except OSError, error:
		os._exit(1)
	return 0


#----------------------------------------------------------------------
# signals
#----------------------------------------------------------------------
closing = False

def sig_exit (signum, frame):
	global closing
	closing = True

def sig_chld (signum, frame):
	while 1:
		try:
			pid, status = os.waitpid(-1, os.WNOHANG)
		except:
			pid = -1
		if pid < 0: break
	return 0

def signal_initialize():
	import signal
	signal.signal(signal.SIGTERM, sig_exit)
	signal.signal(signal.SIGINT, sig_exit)
	signal.signal(signal.SIGABRT, sig_exit)
	if 'SIGQUIT' in signal.__dict__:
		signal.signal(signal.SIGQUIT, sig_exit)
	if 'SIGCHLD' in signal.__dict__:
		signal.signal(signal.SIGCHLD, sig_chld)
	if 'SIGPIPE' in signal.__dict__:
		signal.signal(signal.SIGPIPE, signal.SIG_IGN)
	return 0


#----------------------------------------------------------------------
# logs
#----------------------------------------------------------------------
LOGFILE = None
LOGSTDOUT = True

def mlog(text):
	global LOGFILE, LOGSTDOUT
	now = time.strftime('%Y-%m-%d %H:%M:%S')
	txt = '[%s] %s'%(now, text)
	if LOGFILE:
		LOGFILE.write(txt + '\n')
		LOGFILE.flush()
	if LOGSTDOUT:
		sys.stdout.write(txt + '\n')
		sys.stdout.flush()
	return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
def main(args = None):
	if args == None:
		args = [ n for n in sys.argv ]
	import optparse
	p = optparse.OptionParser('usage: %prog [options] to start cron')
	p.add_option('-f', '--filename', dest = 'filename', metavar='FILE', help = 'config file name')
	p.add_option('-i', '--pid', dest = 'pid', help = 'pid file path')
	p.add_option('-l', '--log', dest = 'log', metavar='LOG', help = 'log file')
	p.add_option('-c', '--cwd', dest = 'dir', help = 'working dir')
	p.add_option('-d', '--daemon', action = 'store_true', dest = 'daemon', help = 'run as daemon')
	options, args = p.parse_args(args) 
	if not options.filename:
		print >>sys.stderr, 'No config file name. Try --help for more information.'
		return 2
	filename = options.filename
	if filename:
		if not os.path.exists(filename):
			filename = None
	if not filename:
		print >>sys.stderr, 'invalid file name'
		return 4
	filetime = 0
	try:
		filetime = os.stat(filename).st_mtime
		text = open(filename, 'rb').read()
	except:
		print >>sys.stderr, 'cannot read %s'%filename
		return 5
	
	# crontab initialize
	cron = crontab()

	# read content
	task = cron.read(text)
	if type(task) != type([]):
		print >>sys.stderr, '%s:%d: error: syntax error'%(filename, task)
		return 1

	global LOGSTDOUT, LOGFILE
	if options.log:
		try:
			LOGFILE = open(options.log, 'a')
		except:
			print >>sys.stderr, 'can not open: ' + options.log
			return 6

	if options.daemon:
		if sys.platform[:3] == 'win':
			print >>sys.stderr, 'daemon mode does support in windows'
		elif not 'fork' in os.__dict__:
			print >>sys.stderr, 'can not fork myself'
		else:
			daemon()
			LOGSTDOUT = False

	if options.pid:
		try:
			fp = open(options.pid, 'w')
			fp.write('%d'%os.getpid())
			fp.close()
		except:
			pass

	signal_initialize()

	environ = {}
	for n in os.environ:
		environ[n] = os.environ[n]
	
	mlog('crontab start with %d task(s)'%len(task))
	
	if options.dir:
		if os.path.exists(options.dir):
			try:
				os.chdir(options.dir)
			except:
				mlog('can not chdir to %s'%options.dir)
		else:
			mlog('dir does not exist: %s'%options.dir)

	for node in cron.event(task, 0, env = environ):
		mlog('init: ' + node[1])

	loopcount = 0

	# main loop
	while not closing:
		ts = long(time.time())
		now = time.localtime(ts)[:5]
		run = cron.interval(task, ts, env = environ)
		if run:
			for node in run:
				mlog('schedule: ' + node[1])
		if loopcount % 10 == 0:
			newts = -1
			try:
				newts = os.stat(filename).st_mtime
			except:
				pass
			if newts > 0 and newts > filetime:
				content = None
				try:
					content = open(filename).read()
				except:
					mlog('error open: ' + filename)
					content =None
				if content != None:
					newtask = cron.read(content)
					if type(newtask) != type([]):
						mlog('%s:%d: syntax error'%(filename, newtask))
					else:
						task = newtask
						filetime = ts
						mlog('refresh config with %d task(s)'%len(task))
		loopcount += 1
		time.sleep(1)

	for node in cron.event(task, -1, env = environ):
		mlog('quit: ' + node[1])

	mlog('terminate')

	return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':
	def test1():
		print os.stat('crontab.cfg').st_mtime
		return 0
	def test2():
		cron = crontab()
		task = cron.read(open('crontab.cfg').read())
		ts = time.time()
		for n in cron.event(task, ev = 0):
			print 'reboot', n[0]
		for i in xrange(3600):
			now = time.localtime(ts)[:5]
			res = cron.interval(task, ts)
			for n in res:
				print now, n[0]
			ts += 1
		for n in cron.event(task, ev = -1):
			print 'shutdown', n[0]
		return 0
	def test3():
		args = [ 'crontab', '--filename=crontab.cfg', '--pid=crontab.pid' ]
		#args = ['crontab', '--help']
		main(args)
		return 0
	#test3()
	sys.exit(main())


