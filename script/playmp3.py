#! /usr/bin/env python2
# -*- coding: utf-8 -*-
#======================================================================
#
# playsnd.py - play sound with ctypes + mci
#
# Created by skywind on 2013/12/01
# Last change: 2014/01/26 23:40:20
#
#======================================================================
import sys
import time
import os
import ctypes



#----------------------------------------------------------------------
# WinMM - Windows player
#----------------------------------------------------------------------
class WinMM (object):

	def __init__ (self):
		import ctypes.wintypes
		if sys.platform[:3] != 'win':
			raise SystemError('Windows is required')
		self.__winmm = ctypes.windll.LoadLibrary('winmm.dll')
		self.__mciSendStringW = self.__winmm.mciSendStringW
		self.__mciGetErrorStringW = self.__winmm.mciGetErrorStringW
		wintypes = ctypes.wintypes
		LPCWSTR, HANDLE = wintypes.LPCWSTR, wintypes.HANDLE
		args = [LPCWSTR, ctypes.c_char_p, wintypes.UINT, HANDLE]
		self.__mciSendStringW.argtypes = args
		self.__mciSendStringW.restype = wintypes.DWORD
		args = [wintypes.DWORD, ctypes.c_void_p, wintypes.UINT]
		self.__mciGetErrorStringW.argtypes = args
		self.__mciGetErrorStringW.restype = wintypes.BOOL
		self.__buffer = ctypes.create_string_buffer('?' * 4098)
		self.__alias_index = 0
	
	def mciSendString (self, command, encoding = 'utf-8'):
		if isinstance(command, str):
			command = command.decode(encoding)
		hr = self.__mciSendStringW(command, self.__buffer, 2048, 0)
		if hr != 0:
			return long(hr)
		buffer = self.__buffer
		size = 0
		for i in xrange(2048):
			if buffer[i * 2:i * 2 + 2] == '\x00\x00':
				size = i;
				break
		data = buffer[:size * 2].decode('utf-16')
		return data

	def mciGetErrorString (self, error):
		buffer = self.__buffer
		hr = self.__mciGetErrorStringW(error, buffer, 2048)
		if hr == 0:
			return None
		size = 0
		for i in xrange(2048):
			if buffer[i * 2:i * 2 + 2] == '\x00\x00':
				size = i;
				break
		data = buffer[:size * 2].decode('utf-16')
		return data

	def open (self, filename, media_type = ''):
		if not os.path.exists(filename):
			return None
		filename = os.path.abspath(filename)
		name = 'media:%d'%self.__alias_index
		self.__alias_index += 1
		cmd = u'open "%s" alias %s'%(filename, name)
		if media_type:
			cmd = u'open "%s" type %s alias %s'%(filename, media_type, name)
		hr = self.mciSendString(cmd)
		if isinstance(hr, unicode) or isinstance(hr, str):
			return name
		return None

	def close (self, name):
		hr = self.mciSendString(u'close %s'%name)
		if isinstance(hr, unicode) or isinstance(hr, str):
			return True
		return False

	def __get_status (self, name, what):
		hr = self.mciSendString(u'status %s %s'%(name, what))
		if isinstance(hr, unicode) or isinstance(hr, str):
			return hr
		return None

	def __get_status_int (self, name, what):
		hr = self.__get_status(name, what)
		if hr is None:
			return -1
		hr = long(hr)
		return (hr > 0x7fffffff) and hr or int(hr)

	def __mci_no_return (self, cmd):
		hr = self.mciSendString(cmd)
		if isinstance(hr, unicode) or isinstance(hr, str):
			return True
		return False

	def get_length (self, name):
		return self.__get_status_int(name, 'length')

	def get_position (self, name):
		return self.__get_status_int(name, 'position')

	def get_mode (self, name):
		hr = self.__get_status(name, 'mode')
		return hr

	def play (self, name, start = 0, end = -1, wait = False, repeat = False):
		if wait:
			repeat = False
		if start < 0:
			start = 0
		cmd = u'play %s from %d'%(name, start)
		if end >= 0:
			cmd += u' to %d'%end
		if wait:
			cmd += u' wait'
		if repeat:
			cmd += u' repeat'
		return self.__mci_no_return(cmd)

	def stop (self, name):
		return self.__mci_no_return(u'stop %s'%name)

	def seek (self, name, position):
		if isinstance(position, str) or isinstance(position, unicode):
			if position == u'end':
				position = 'end'
			else:
				position = '0'
		elif position < 0:
			position = 'end'
		else:
			position = str(position)
		return self.__mci_no_return(u'seek %s to %s'%name)

	def pause (self, name):
		return self.__mci_no_return(u'pause %s'%name)

	def resume (self, name):
		return self.__mci_no_return(u'resume %s'%name)

	def get_volume (self, name):
		return self.__get_status_int(name, 'volume')

	def set_volume (self, name, volume):
		return self.__mci_no_return(u'setaudio %s volume to %s'%(name, volume))

	def is_playing (self, name):
		mode = self.get_mode(name)
		if mode is None:
			return False
		if mode != 'playing':
			return False
		return True



#----------------------------------------------------------------------
# main entry
#----------------------------------------------------------------------
def main (args = None):
	if args is None:
		args = sys.argv
	args = [n for n in args]
	if len(args) < 2:
		print 'usage: playmp3.py [mp3]'
		return 0
	mp3 = args[1]
	if not os.path.exists(mp3):
		print 'not find: %s'%mp3
		return 1
	def ms2time(ms):
		if ms <= 0: return '00:00:000'
		time_sec, ms = ms / 1000, ms % 1000
		time_min, time_sec = time_sec / 60, time_sec % 60
		time_hor, time_min = time_min / 60, time_min % 60
		if time_hor == 0: return '%02d:%02d:%03d'%(time_min, time_sec, ms)
		return '%02d:%02d:%02d:%03d'%(time_hor, time_min, time_sec, ms)
	winmm = WinMM()
	name = winmm.open(mp3)
	if name is None:
		print 'can not play: %s'%mp3
		return 2
	import ctypes.wintypes
	user32 = ctypes.windll.user32
	user32.GetAsyncKeyState.restype = ctypes.wintypes.WORD
	user32.GetAsyncKeyState.argtypes = [ ctypes.c_char ]
	size = winmm.get_length(name)
	print 'Playing "%s", press \'q\' to exit ....'%mp3
	winmm.play(name, repeat = True)
	while 1:
		if user32.GetAsyncKeyState('Q'): break
		time.sleep(0.1)
		pos = winmm.get_position(name)
		sys.stdout.write('[%s / %s]\r'%(ms2time(pos), ms2time(size)))
		sys.stdout.flush()
	print ''
	print 'stopped'
	winmm.close(name)
	return 0
	

#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':
	def test1():
		winmm = WinMM()
		name = winmm.open('sample.mp3')
		print name
		print winmm.play(name)
		print winmm.get_length(name)
		print winmm.get_volume(name)
		print winmm.set_volume(name, 1000)
		print winmm.mciGetErrorString(1)
		raw_input()
		print 'is_playing', winmm.is_playing(name)
		print 'position:', winmm.get_position(name)
		print 'mode:', winmm.get_mode(name)
		print winmm.stop(name)
		print 'mode:', winmm.get_mode(name)
		return 0
	def test2():
		main([__file__, 'sample.mp3'])
		return 0

	#test2()
	main()


