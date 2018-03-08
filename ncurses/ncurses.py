#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# ncurses.py - 
#
# Download curses pyd on windows from:
#     https://www.lfd.uci.edu/~gohlke/pythonlibs/#curses
#
# Created by skywind on 2018/02/02
# Last change: 2018/02/02 00:11:36
#
#======================================================================
from __future__ import print_function
import sys
import time
import os
import curses
import curses.panel
import curses.textpad


#----------------------------------------------------------------------
# basic curses
#----------------------------------------------------------------------
class ncurses (object):

	def __init__ (self):
		self.name = 'ncurses'
		self.screen = None
		self.guard = False

	def init (self):
		self.screen = curses.initscr()
		# curses.def_shell_mode()
		curses.noecho()
		curses.cbreak()
		self.screen.keypad(1)
		try:
			curses.start_color()
		except:
			pass
		return 0

	def quit (self):
		if self.screen:
			self.screen.keypad(0)
			self.screen = None
		curses.echo()
		curses.nocbreak()
		curses.endwin()
		# curses.reset_shell_mode()
		return 0

	def wrapper (self, main, *args, **argv):
		hr = 0
		if not self.guard:
			self.init()
			hr = main(self.screen, *args, **argv)
			self.quit()
		else:
			try:
				self.init()
				hr = main(self.screen, *args, **argv)
			finally:
				self.quit()
		return hr

#----------------------------------------------------------------------
# log out
#----------------------------------------------------------------------
def plog(*args):
	try:
		now = time.strftime('%Y-%m-%d %H:%M:%S')
		date = now.split(None, 1)[0].replace('-', '')
		logfile = sys.modules[__name__].__dict__.get('logfile', None)
		logtime = sys.modules[__name__].__dict__.get('logtime', '')
		loghome = sys.modules[__name__].__dict__.get('loghome', '')
		if not loghome:
			loghome = os.getcwd()
			sys.modules[__name__].__dict__['loghome'] = loghome
		if date != logtime:
			logtime = date
			if logfile: logfile.close()
			logfile = None
		if logfile == None:
			logname = '%s%s.log'%('c', date)
			logfile = open(os.path.join(loghome, logname), 'a')
		sys.modules[__name__].__dict__['logtime'] = logtime
		sys.modules[__name__].__dict__['logfile'] = logfile
		def strlize(x):
			if isinstance(x, unicode):
				return x.encode("utf-8")
			return str(x)
		str_args = map(strlize, args)
		text = " ".join(str_args)
		logfile.write('[%s] %s\n'%(now, text))
		logfile.flush()
		if 1 == 0:
			text = '[%s] %s\n'%(now, text)
			#STDOUT.write(text.decode('utf-8'))
			stdout.write(text)
	except Exception:
		pass
	return 0


#----------------------------------------------------------------------
# testing 
#----------------------------------------------------------------------
if __name__ == '__main__':

	nc = ncurses()
	os.environ['PDC_RESTORE_SCREEN'] = 'yes'

	def test1():
		def main(scr):
			scr.border()
			while True:
				print('fuck')
				ch = scr.getch()
				if ch == ord('q'):
					break
				if ch == curses.KEY_RESIZE:
					y, x = scr.getmaxyx()
					print('resize %dx%d'%(x, y))
			return 0
		print('hello')
		nc.wrapper(main)
		print('end')
		return 0

	def test2():
		def main(stdscr):
			begin_x = 20
			begin_y = 7
			height = 5
			width = 40
			win = curses.newwin(height, width, begin_y, begin_x)
			tb = curses.textpad.Textbox(win)
			text = tb.edit()
			curses.addstr(4,1,text.encode('utf_8'))
			time.sleep(10)
		nc.wrapper(main)

	test1()


