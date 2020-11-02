#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# fzf_cd.py - Change Directory in Total Commander with fzf
#
# Created by skywind on 2020/10/31
# Last Modified: 2020/10/31 22:15:03
#
#======================================================================
from __future__ import print_function, unicode_literals
import sys
import time
import array
import struct
import os
import ctypes



#----------------------------------------------------------------------
# 2/3 compatible
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
    unicode = str
    xrange = range
    long = int


#----------------------------------------------------------------------
# Win32API
#----------------------------------------------------------------------
class Win32API (object):

    def __init__ (self):
        import ctypes
        self.kernel32 = ctypes.windll.LoadLibrary('kernel32.dll')
        self.user32 = ctypes.windll.LoadLibrary('user32.dll')
        self._query_interface()
        self._guess_encoding()
        self._setup_struct()

    def _query_interface (self):
        import ctypes
        import ctypes.wintypes
        self.ctypes = ctypes
        self.wintypes = ctypes.wintypes
        wintypes = ctypes.wintypes
        HWND, LONG, BOOL = wintypes.HWND, wintypes.LONG, wintypes.BOOL
        UINT, DWORD, c_int = wintypes.UINT, wintypes.DWORD, ctypes.c_int
        WPARAM, LPARAM = wintypes.WPARAM, wintypes.LPARAM
        self.WNDENUMPROC = ctypes.WINFUNCTYPE(
                wintypes.BOOL,
                wintypes.HWND,    # _In_ hWnd
                wintypes.LPARAM,) # _In_ lParam
        self.user32.EnumThreadWindows.argtypes = (
                wintypes.DWORD,
                self.WNDENUMPROC,
                wintypes.LPARAM)
        self.user32.EnumThreadWindows.restype = wintypes.BOOL
        self.user32.GetParent.argtypes = (wintypes.HWND,)
        self.user32.GetParent.restype = wintypes.HWND
        self.kernel32.GetConsoleWindow.argtypes = []
        self.kernel32.GetConsoleWindow.restype = wintypes.HWND
        self.user32.GetWindowLongA.argtypes = (HWND, ctypes.c_int)
        self.user32.GetWindowLongA.restype = LONG
        self.user32.SetWindowLongA.argtypes = (HWND, ctypes.c_int, LONG)
        self.user32.SetWindowLongA.restype = LONG
        self.kernel32.GetCurrentThreadId.argtypes = []
        self.kernel32.GetCurrentThreadId.restype = wintypes.DWORD
        self.user32.SendMessageA.argtypes = (HWND, UINT, WPARAM, LPARAM)
        self.user32.SendMessageA.restype = wintypes.LONG
        self.user32.SendMessageW.argtypes = (HWND, UINT, WPARAM, LPARAM)
        self.user32.SendMessageW.restype = wintypes.LONG
        self.user32.PostMessageA.argtypes = (HWND, UINT, WPARAM, LPARAM)
        self.user32.PostMessageA.restype = wintypes.LONG
        self.user32.PostMessageW.argtypes = (HWND, UINT, WPARAM, LPARAM)
        self.user32.PostMessageW.restype = wintypes.LONG
        self.user32.FindWindowA.argtypes = (ctypes.c_char_p, ctypes.c_char_p)
        self.user32.FindWindowA.restype = HWND
        self.user32.FindWindowW.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p)
        self.user32.FindWindowW.restype = HWND
        args = (HWND, HWND, c_int, c_int, c_int, c_int, UINT)
        self.user32.SetWindowPos.argtypes = args
        self.user32.SetWindowPos.restype = LONG
        args = (HWND, wintypes.COLORREF, wintypes.BYTE, DWORD)
        self.user32.SetLayeredWindowAttributes.argtypes = args
        self.user32.SetLayeredWindowAttributes.restype = BOOL
        self.user32.GetAsyncKeyState.argtypes = (c_int,)
        self.user32.GetAsyncKeyState.restype = wintypes.SHORT
        self.user32.GetActiveWindow.argtypes = []
        self.user32.GetActiveWindow.restype = HWND
        args = [ ctypes.c_char_p, ctypes.c_char_p, DWORD ]
        self.kernel32.GetShortPathNameA.argtypes = args
        self.kernel32.GetShortPathNameA.restype = DWORD
        self.kernel32.GetLongPathNameA.argtypes = args
        self.kernel32.GetLongPathNameA.restype = DWORD
        args = [ ctypes.c_wchar_p, ctypes.c_wchar_p, DWORD ]
        self.kernel32.GetShortPathNameW.argtypes = args
        self.kernel32.GetShortPathNameW.restype = DWORD
        self.kernel32.GetLongPathNameW.argtypes = args
        self.kernel32.GetLongPathNameW.restype = DWORD

    def _guess_encoding (self):
        guess = []
        try:
            import locale
            guess.append(locale.getpreferredencoding())
        except:
            pass
        guess.append(sys.getdefaultencoding)
        for fp in (sys.stdout, sys.stdin):
            if fp and hasattr(fp, 'encoding'):
                if fp.encoding:
                    guess.append(fp.encoding)
        guess.append('utf-8')
        self.encoding = guess[0]
        return self.encoding

    def _setup_struct (self):
        import ctypes
        self.cbuffer = ctypes.create_string_buffer(8192)
        self.wbuffer = ctypes.create_unicode_buffer(8192)
        return 0

    def EnumThreadWindows (self, id, proc, lparam):
        return self.user32.EnumThreadWindows(id, proc, lparam)

    def GetWindowLong (self, hwnd, index):
        return self.user32.GetWindowLongA(hwnd, index)

    def SetWindowLong (self, hwnd, index, value):
        return self.user32.SetWindowLongA(hwnd, index, value)

    def GetCurrentThreadId (self):
        return self.kernel32.GetCurrentThreadId()

    def GetConsoleWindow (self):
        return self.kernel32.GetConsoleWindow()

    def GetParent (self, hwnd):
        return self.user32.GetParent(hwnd)

    def ConvertToParam (self, data):
        if data is None:
            return 0
        if isinstance(data, int):
            return data
        if isinstance(data, array.array):
            address, size = data.buffer_info()
            return address
        if isinstance(data, ctypes.Array):
            return ctypes.addressof(data)
        return data

    def SendMessageA (self, hwnd, msg, wparam, lparam):
        wparam = self.ConvertToParam(wparam)
        lparam = self.ConvertToParam(lparam)
        return self.user32.SendMessageA(hwnd, msg, wparam, lparam)

    def SendMessageW (self, hwnd, msg, wparam, lparam):
        wparam = self.ConvertToParam(wparam)
        lparam = self.ConvertToParam(lparam)
        return self.user32.SendMessageW(hwnd, msg, wparam, lparam)

    def PostMessageA (self, hwnd, msg, wparam, lparam):
        wparam = self.ConvertToParam(wparam)
        lparam = self.ConvertToParam(lparam)
        return self.user32.PostMessageA(hwnd, msg, wparam, lparam)

    def PostMessageW (self, hwnd, msg, wparam, lparam):
        wparam = self.ConvertToParam(wparam)
        lparam = self.ConvertToParam(lparam)
        return self.user32.PostMessageW(hwnd, msg, wparam, lparam)

    def SetWindowPos (self, hwnd, after, x, y, cx, cy, flags):
        return self.user32.SetWindowPos(hwnd, after, x, y, cx, cy, flags)

    def SetLayeredWindowAttributes (self, hwnd, cc, alpha, flag):
        return self.user32.SetLayeredWindowAttributes(hwnd, cc, alpha, flag)

    def GetAsyncKeyState (self, keycode):
        if isinstance(keycode, str):
            keycode = keycode and ord(keycode[0]) or 0
        return self.user32.GetAsyncKeyState(keycode)

    def GetActiveWindow (self):
        return self.user32.GetActiveWindow()

    def ConvertToWide (self, text):
        if text is None:
            return None
        if isinstance(text, bytes):
            for enc in (self.encoding, 'utf-8', 'gbk'):
                try:
                    p = text.decode(enc)
                    text = p
                    break
                except:
                    pass
        if isinstance(text, bytes):
            text = text.decode('utf-8', 'ignore')
        return text

    def ConvertToAnsi (self, text):
        if text is None:
            return None
        if isinstance(text, str):
            for enc in (self.encoding, 'utf-8', 'gbk'):
                try:
                    p = text.encode(enc)
                    text = p
                    break
                except:
                    pass
        if isinstance(text, str):
            text = text.encode('utf-8', 'ignore')
        return text

    def GetShortPathNameA (self, path):
        path = self.ConvertToAnsi(path)
        hr = self.kernel32.GetShortPathNameA(path, self.cbuffer, 4097)
        if hr <= 0:
            return None
        value = bytes(self.cbuffer[:hr])
        x = self.ConvertToWide(value)
        return x

    def GetShortPathNameW (self, path):
        path = self.ConvertToWide(path)
        hr = self.kernel32.GetShortPathNameW(path, self.wbuffer, 4097)
        if hr <= 0:
            return None
        value = str(self.wbuffer[:hr])
        return value

    def GetLongPathNameA (self, path):
        path = self.ConvertToAnsi(path)
        hr = self.kernel32.GetLongPathNameA(path, self.cbuffer, 4097)
        if hr <= 0:
            return None
        value = bytes(self.cbuffer[:hr])
        return self.ConvertToWide(value)

    def GetLongPathNameW (self, path):
        path = self.ConvertToWide(path)
        hr = self.kernel32.GetLongPathNameW(path, self.wbuffer, 4097)
        if hr <= 0:
            return None
        value = str(self.wbuffer[:hr])
        return value

    def FindWindowA (self, ClassName, WindowName):
        ClassName = self.ConvertToAnsi(ClassName)
        WindowName = self.ConvertToAnsi(WindowName)
        return self.user32.FindWindowA(ClassName, WindowName)

    def FindWindowW (self, ClassName, WindowName):
        ClassName = self.ConvertToWide(ClassName)
        WindowName = self.ConvertToWide(WindowName)
        return self.user32.FindWindowW(ClassName, WindowName)

    def CopyData (self, hwnd, msg, payload, source = None):
        payload = self.ConvertToAnsi(payload)
        data_size = 0
        data_address = 0
        data_ptr = None
        if payload:
            data_ptr = array.array('B', payload)
            data_address, data_size = data_ptr.buffer_info()
        copy_struct = struct.pack('PLP', msg, data_size, data_address)
        p1 = array.array('B', copy_struct)
        return self.SendMessageA(hwnd, 74, source, p1)

    def GetRightPathCase (self, path):
        path = self.GetShortPathNameW(path)
        path = self.GetLongPathNameW(path)
        if len(path) > 2:
            if path[1] == ':' and path[0].isalpha():
                path = path[0].upper() + path[1:]
        return path


#----------------------------------------------------------------------
# Configure
#----------------------------------------------------------------------
class Configure (object):

    def __init__ (self):
        self.dirname = os.path.dirname(os.path.abspath(__file__))
        self.cmdhome = None
        self._cache = {}
        self._guess_encoding()
        self._load_config()
        self.cmdhome = self._search_home()
        self.cmdconf = self._search_conf()
        self.origin = {}
        self.origin['path'] = os.environ.get('COMMANDER_PATH', '')
        self.origin['ini'] = os.environ.get('COMMANDER_INI', '')
        if self.cmdhome:
            os.environ['COMMANDER_PATH'] = self.cmdhome
        if self.cmdconf:
            os.environ['COMMANDER_INI'] = self.cmdconf
        self.ghisler = self._setup_dir()
        self.database = os.path.join(self.ghisler, 'fzfmru.txt')

    def _guess_encoding (self):
        guess = []
        try:
            import locale
            guess.append(locale.getpreferredencoding())
        except:
            pass
        guess.append(sys.getdefaultencoding)
        for fp in (sys.stdout, sys.stdin):
            if fp and hasattr(fp, 'encoding'):
                if fp.encoding:
                    guess.append(fp.encoding)
        guess.append('utf-8')
        self.encoding = guess[0]
        return self.encoding

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

    # save content
    def save_file_content (self, filename, content, mode = 'w'):
        try:
            fp = open(filename, mode)
            fp.write(content)
            fp.close()
        except:
            return False
        return True

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

    # save file text
    def save_file_text (self, filename, content, encoding = None):
        import codecs
        if encoding is None:
            encoding = 'utf-8'
        if (not isinstance(content, unicode)) and isinstance(content, bytes):
            return self.save_file_content(filename, content)
        with codecs.open(filename, 'w', 
                encoding = encoding, 
                errors = 'ignore') as fp:
            fp.write(content)
        return True

    # load ini without ConfigParser
    def load_ini (self, filename, encoding = None):
        text = self.load_file_text(filename, encoding)
        config = {}
        sect = 'default'
        if text is None:
            return None
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

    def read_ini (self, name, encoding = None):
        obj = self.load_ini(name, encoding)
        if not obj:
            obj = {}
        else:
            newobj = {}
            for sect in obj:
                section = {}
                for k, v in obj[sect].items():
                    section[k.lower()] = v
                newobj[sect.lower()] = section
            obj = newobj
        return obj

    # get ini file
    def read_config (self, ininame, encoding):
        ininame = os.path.abspath(ininame)
        ininame = os.path.normcase(ininame)
        if ininame in self._cache:
            return self._cache[ininame]
        if not os.path.exists(ininame):
            return None
        obj = self.read_ini(ininame, encoding)
        self._cache[ininame] = obj
        return obj

    def reset (self):
        self._cache = {}
        return True

    def _load_config (self):
        name = os.path.abspath(__file__)
        main = os.path.splitext(name)[0] + '.ini'
        obj = self.read_ini(main)
        if 'default' not in obj:
            obj['default'] = {}
        self.config = obj
        return obj

    def option (self, section, key, default = None):
        section = section.lower()
        if section not in self.config:
            return default
        sect = self.config[section]
        key = key.lower()
        if key not in sect:
            return default
        return sect[key]

    def tmpname (self, filename, fill = 5):
        import time, os, random
        while 1:
            name = '.' + str(int(time.time() * 1000000))
            for i in range(fill):
                k = random.randint(0, 51)
                name += (k < 26) and chr(ord('A') + k) or chr(ord('a') + k - 26)
            test = filename + name + str(os.getpid())
            if not os.path.exists(test):
                return test
        return None

    def save_atomic (self, filename, content):
        if isinstance(content, list):
            content = '\n'.join(content)
        temp = self.tmpname(filename)
        self.save_file_text(temp, content, 'utf-8')
        return self.replace_file(temp, filename)

    # find root
    def find_root (self, path, markers = None, fallback = False):
        if markers is None:
            markers = ('.git', '.svn', '.hg', '.project', '.root')
        if path is None:
            path = os.getcwd()
        path = os.path.abspath(path)
        base = path
        while True:
            parent = os.path.normpath(os.path.join(base, '..'))
            for marker in markers:
                test = os.path.join(base, marker)
                if os.path.exists(test):
                    return base
            if os.path.normcase(parent) == os.path.normcase(base):
                break
            base = parent
        if fallback:
            return path
        return None

    def _check_home (self, home):
        if not home:
            return False
        if not os.path.exists(home):
            return False
        if os.path.exists(os.path.join(home, 'totalcmd.exe')):
            return True
        if os.path.exists(os.path.join(home, 'totalcmd64.exe')):
            return True
        return False

    def _search_home (self):
        path = self.option('default', 'commander_path')
        if path:
            if self._check_home(path) and 0:
                return path
        if 'COMMANDER_PATH' in os.environ:
            path = os.environ['COMMANDER_PATH']
            if self._check_home(path):
                return path
        test = self.dirname
        while 1:
            if self._check_home(test):
                return test
            next = os.path.abspath(os.path.join(test, '..'))
            if os.path.normcase(next) == os.path.normcase(test):
                break
            test = next
        return None

    def _search_conf (self):
        if not self.cmdhome:
            return None
        path = self.option('default', 'commander_ini')
        if path:
            if os.path.exists(path):
                return os.path.abspath(path)
        if 'COMMANDER_INI' in os.environ:
            path = os.environ['COMMANDER_INI']
            if os.path.exists(path):
                return path
        path = os.path.join(self.cmdhome, 'wincmd.ini')
        if os.path.exists(path):
            config = self.read_ini(path, self.encoding)
            section = config.get('configuration', {})
            value = section.get('useiniinprogramdir', None)
            if value and isinstance(value, str):
                if value in ('t', 'true', 'y', 'yes', '1'):
                    return path
                if value.isdigit():
                    try:
                        value = int(value, 0)
                        if value != 0:
                            return path
                    except:
                        pass
        path = os.path.expandvars('%AppData%\Ghisler\wincmd.ini')
        if os.path.exists(path):
            return path
        path = os.path.expandvars('%WinDir%\wincmd.ini')
        if os.path.exists(path):
            return path
        return None

    def _load_section (self, name, section):
        config = self.read_config(name, self.encoding)
        if config is None:
            return None
        obj = config.get(section, None)
        if obj is None:
            return None
        if 'redirectsection' in obj:
            redirect = obj.get('redirectsection', None)
            if redirect:
                path = os.path.expandvars(redirect)
                if not os.path.exists(path):
                    return None
                x = self._load_section(path, section)
                return x
        return obj

    def load_history (self):
        if not self.cmdconf:
            return None
        config = self.read_config(self.cmdconf, self.encoding)
        if not config:
            return None
        if 'lefthistory' not in config:
            if 'righthistory' not in config:
                return None
        history = [None, None]
        fetch = [[], []]
        fetch[0] = self._load_section(self.cmdconf, 'lefthistory')
        fetch[1] = self._load_section(self.cmdconf, 'righthistory')
        for i in range(2):
            obj = fetch[i]
            items = []
            if obj is not None:
                for ii in range(len(obj)):
                    key = str(ii)
                    if key in obj:
                        value = obj[key].strip()
                        path, _, _ = value.partition('#')
                        path = path.strip()
                        if path:
                            items.append(path)
            history[i] = items
        return history

    def _setup_dir (self):
        path = os.environ.get('USERPROFILE', '')
        if 'AppData' in os.environ:
            path = os.path.expandvars('%AppData%\Ghisler')
        else:
            if path:
                path = os.path.join(path, 'AppData/Roaming/Ghisler')
            else:
                return None
        if not os.path.exists(path):
            os.makedirs(path)
        return path

    def mru_load (self, dbname):
        content = self.load_file_text(dbname)
        if content is None:
            return []
        lines = []
        for line in content.split('\n'):
            line = line.strip('\r\n\t ')
            if line:
                lines.append(line)
        return lines

    def mru_save (self, dbname, lines):
        content = '\n'.join(lines)
        self.save_atomic(dbname, content)
        return 0


#----------------------------------------------------------------------
# TotalCommander
#----------------------------------------------------------------------
class TotalCommander (object):

    def __init__ (self):
        self.config = Configure()
        self.win32 = Win32API()
        self.hwnd = self.FindTC()
        self.source = None
        self.MSG_EM = ord('E') + ord('M') * 256
        self.MSG_CD = ord('C') + ord('D') * 256

    def FindTC (self):
        return self.win32.FindWindowW('TTOTAL_CMD', None)

    def CheckTC (self):
        if self.config.cmdhome is None:
            print('can not locate tc home, please set %COMMANDER_PATH%')
            return -1
        if self.config.cmdconf is None:
            print('can not locate tc ini, please set %COMMANDER_INI%')
            return -2
        if not self.FindTC():
            print('TC is not running')
            return -3
        return 0

    def SendMessage (self, msg, text):
        text = self.win32.ConvertToAnsi(text)
        code = 0
        if isinstance(msg, str):
            for i, ch in enumerate(msg):
                code += ord(ch) * (i << 8)
        elif isinstance(msg, int):
            code = msg
        return self.win32.CopyData(self.hwnd, code, text, self.source)

    def SendUserCommand (self, command):
        return self.SendMessage(self.MSG_EM, command)

    def SendChangeDirectory (self, first, second, flag):
        params = []
        if (not first) and (not second):
            return -1
        for param in (first, second, flag):
            params.append(self.win32.ConvertToAnsi(param))
        output = b''
        first, second, flag = params
        if first:
            output = first
        output += b'\r'
        if second:
            output += second
        output += b'\x00'
        if flag:
            output += flag
        return self.SendMessage(self.MSG_CD, output)

    def LoadHistory (self):
        ini = self.config.load_history()
        mru = self.config.mru_load(self.config.database)
        history = []
        for i in range(max(len(ini[0]), len(ini[1]))):
            if i < len(ini[0]):
                history.append(ini[0][i])
            if i < len(ini[1]):
                history.append(ini[1][i])
        skips = {}
        for n in mru:
            history.append(n)
            skips[os.path.normcase(n)] = 1
        unique = []
        exists = {}
        for path in history:
            path = os.path.abspath(path)
            key = os.path.normcase(path)
            if key not in exists:
                if path.startswith('\\\\'):
                    continue
                if len(path) < 2:
                    continue
                elif not path[0].isalpha():
                    continue
                elif path[1] != ':':
                    continue
                if path[0].islower():
                    path = path[0].upper() + path[1:]
                if len(path) == 3 and path.endswith('\\'):
                    continue
                if not os.path.isdir(path):
                    continue
                if os.path.exists(path):
                    if key not in skips:
                        path = self.win32.GetRightPathCase(path)
                        if not path:
                            continue
                    unique.append(path)
                    exists[key] = 1
        return unique

    def SaveHistory (self, history):
        return self.config.mru_save(self.config.database, history)

    def StartFZF (self, input, args = None, fzf = None):
        import tempfile
        code = 0
        output = None
        args = args is not None and args or ''
        fzf = fzf is not None and fzf or 'fzf'
        with tempfile.TemporaryDirectory(prefix = 'fzf.') as dirname:
            outname = os.path.join(dirname, 'output.txt')
            if isinstance(input, list):
                inname = os.path.join(dirname, 'input.txt')
                with open(inname, 'wb') as fp:
                    content = '\n'.join([ str(n) for n in input ])
                    fp.write(content.encode('utf-8'))
                cmd = '%s %s < "%s" > "%s"'%(fzf, args, inname, outname)
            elif isinstance(input, str):
                cmd = '%s | %s %s > "%s"'%(input, fzf, args, outname)
            code = os.system(cmd)
            if os.path.exists(outname):
                with open(outname, 'rb') as fp:
                    output = fp.read()
        if output is not None:
            output = output.decode('utf-8')
        if code != 0:
            return None
        output = output.strip('\r\n')
        return output

    def CheckExe (self, exename):
        cmd = 'where %s > nul 2> nul'%exename
        code = os.system(cmd)
        if code == 0:
            return True
        return False


#----------------------------------------------------------------------
# getopt: returns (options, args)
#----------------------------------------------------------------------
def getopt (argv):
    args = []
    options = {}
    if argv is None:
        argv = sys.argv[1:]
    index = 0
    count = len(argv)
    while index < count:
        arg = argv[index]
        if arg != '':
            head = arg[:1]
            if head != '-':
                break
            if arg == '-':
                break
            name = arg.lstrip('-')
            key, _, val = name.partition('=')
            options[key.strip()] = val.strip()
        index += 1
    while index < count:
        args.append(argv[index])
        index += 1
    return options, args


#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------
def main (argv = None):
    argv = argv and argv or sys.argv
    argv = [n for n in argv]
    opts, args = getopt(argv)
    mode = ''
    if 'm' in opts:
        mode = 'history'
    elif 'f' in opts:
        mode = 'forward'
    elif 'b' in opts:
        mode = 'backward'
    elif 'p' in opts:
        mode = 'project'
    if not mode:
        prog = os.path.split(__file__)[-1]
        print('usage: python %s <operation>'%prog)
        print('available operations:')
        print('    -m    cd from mru history')
        print('    -f    cd forward')
        print('    -b    cd backward')
        print('    -p    cd in project')
        print()
        return 0
    tc = TotalCommander()
    hr = tc.CheckTC()
    if hr != 0:
        return 1
    args = ''
    fzf = 'fzf'
    fzf = 'peco'
    if fzf == 'fzf':
        args = '--reverse'
        t = os.environ.get('FZF_CD_ARGS')
        if t:
            args = t
    else:
        pass
    if mode == 'history':
        print('waiting ...')
        tc.SendUserCommand('cm_ConfigSaveDirHistory')
        time.sleep(0.1)
        if not tc.CheckExe('fzf'):
            print('can not find fzf executable')
            return 2
        tc.config.reset()
        mru = tc.LoadHistory()
        tc.SaveHistory(mru)
        path = tc.StartFZF(mru, args, fzf)
        print('change to', path)
        # time.sleep(10)
        if path:
            tc.SendChangeDirectory(path, None, 'S')
        # input()
        return 0
    return 0


#----------------------------------------------------------------------
# testing suit
#----------------------------------------------------------------------
if __name__ == '__main__':

    def test1():
        print(win32.GetActiveWindow())
        print(win32.FindWindowW('TTOTAL_CMD', None))
        return 0

    def test2():
        tc = TotalCommander()
        hr = tc.SendUserCommand('em_calc')
        hr = tc.SendUserCommand('cm_ConfigSaveDirHistory')
        print(hr)

    def test3():
        tc = TotalCommander()
        # hr = tc.SendChangeDirectory('d:/temp', None, None)
        hr = tc.SendChangeDirectory('d:/acm', 'e:/Lab', 'S')
        print(hr)
        print(tc.config.cmdhome)
        print(tc.config.cmdconf)
        # print(tc.config.load_history())
        # import pprint
        # pprint.pprint(tc.LoadHistory())
        # print(tc.win32.GetRightPathCase('d:\\program files'))
        return 0

    def test4():
        os.chdir(os.path.expandvars('%USERPROFILE%'))
        tc = TotalCommander()
        # hr = tc.StartFZF(['1234', '5678'], '--reverse')
        hr = tc.StartFZF('dir /A:d /b /s', '--reverse')
        print(hr)
        return 0

    def test5():
        args = ['', '']
        args = ['', '-m']
        main(args)
        return 0

    test5()



