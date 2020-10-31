#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# tcmru.py - 
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

    def _guess_encoding (self):
        guess = []
        try:
            import locale
            guess.append(locale.getpreferredencoding())
        except:
            pass
        for fp in (sys.stdout, sys.stdin):
            if fp and hasattr(fp, 'encoding'):
                if fp.encoding:
                    guess.append(fp.encoding)
        guess.append('utf-8')
        self.encoding = guess[0]
        return self.encoding

    def _setup_struct (self):
        import ctypes
        self.buffer = ctypes.create_string_buffer(8192)
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


#----------------------------------------------------------------------
# Win32 Interface
#----------------------------------------------------------------------
win32 = Win32API()


#----------------------------------------------------------------------
# TotalCommander
#----------------------------------------------------------------------
class TotalCommander (object):

    def __init__ (self):
        self.win32 = Win32API()
        self.hwnd = self.FindTC()
        self.source = None
        self.MSG_EM = ord('E') + ord('M') * 256
        self.MSG_CD = ord('C') + ord('D') * 256
        # print(self.MSG_EM)

    def FindTC (self):
        return self.win32.FindWindowW('TTOTAL_CMD', None)

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
        return 0

    test3()



