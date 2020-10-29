#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# tcjump.py - 
#
# Created by skywind on 2020/10/29
# Last Modified: 2020/10/29 17:33:02
#
#======================================================================
from __future__ import unicode_literals, print_function
import sys
import time
import os


#----------------------------------------------------------------------
# configure
#----------------------------------------------------------------------
class configure (object):

    def __init__ (self):
        self.dirname = os.path.dirname(os.path.abspath(__file__))

    def replace_file (self, srcname, dstname):
        import sys, os
        if sys.platform[:3] != 'win':
            try:
                os.rename(srcname, dstname)
            except OSError:
                return False
        else:
            import ctypes.wintypes
            kernel32 = ctypes.windll.kernel32
            wp, vp, cp = ctypes.c_wchar_p, ctypes.c_void_p, ctypes.c_char_p
            DWORD, BOOL = ctypes.wintypes.DWORD, ctypes.wintypes.BOOL
            kernel32.ReplaceFileA.argtypes = [ cp, cp, cp, DWORD, vp, vp ]
            kernel32.ReplaceFileW.argtypes = [ wp, wp, wp, DWORD, vp, vp ]
            kernel32.ReplaceFileA.restype = BOOL
            kernel32.ReplaceFileW.restype = BOOL
            kernel32.GetLastError.argtypes = []
            kernel32.GetLastError.restype = DWORD
            success = False
            try:
                os.rename(srcname, dstname)
                success = True
            except OSError:
                pass
            if success:
                return True
            if sys.version_info[0] < 3 and isinstance(srcname, str):
                hr = kernel32.ReplaceFileA(dstname, srcname, None, 2, None, None)
            else:
                hr = kernel32.ReplaceFileW(dstname, srcname, None, 2, None, None)
            if not hr:
                return False
        return True

    # load content
    def load_file_content (self, filename, mode = 'r'):
        if hasattr(filename, 'read'):
            try: content = filename.read()
            except: pass
            return content
        try:
            fp = open(filename, mode)
            content = fp.read()
            fp.close()
        except:
            content = None
        return content

    # load file and guess encoding
    def load_file_text (self, filename, encoding = None):
        content = self.load_file_content(filename, 'rb')
        if content is None:
            return None
        if content[:3] == b'\xef\xbb\xbf':
            text = content[3:].decode('utf-8')
        elif encoding is not None:
            text = content.decode(encoding, 'ignore')
        else:
            text = None
            guess = [sys.getdefaultencoding(), 'utf-8']
            if sys.stdout and sys.stdout.encoding:
                guess.append(sys.stdout.encoding)
            try:
                import locale
                guess.append(locale.getpreferredencoding())
            except:
                pass
            visit = {}
            for name in guess + ['gbk', 'ascii', 'latin1']:
                if name in visit:
                    continue
                visit[name] = 1
                try:
                    text = content.decode(name)
                    break
                except:
                    pass
            if text is None:
                text = content.decode('utf-8', 'ignore')
        return text

    # load ini without ConfigParser
    def load_ini (self, filename, encoding = None):
        text = self.load_file_text(filename, encoding)
        config = {}
        sect = 'default'
        for line in text.split('\n'):
            line = line.strip('\r\n\t ')
            if not line:
                continue
            elif line[:1] in ('#', ';'):
                continue
            elif line.startswith('['):
                if line.endswith(']'):
                    sect = line[1:-1].strip('\r\n\t ')
                    if sect not in config:
                        config[sect] = {}
            else:
                pos = line.find('=')
                if pos >= 0:
                    key = line[:pos].rstrip('\r\n\t ')
                    val = line[pos + 1:].lstrip('\r\n\t ')
                    if sect not in config:
                        config[sect] = {}
                    config[sect][key] = val
        return config


#----------------------------------------------------------------------
# testing suit
#----------------------------------------------------------------------
if __name__ == '__main__':
    def test1():
        return 0
    test1()


