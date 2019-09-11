#! /usr/bin/env python3
# -*- coding: utf-8 -*-
#======================================================================
#
# kanaquiz.py - 
#
# Created by skywind on 2019/09/11
# Last Modified: 2019/09/11 16:35:48
#
#======================================================================
import sys
import time
import os
import codecs
import json


#----------------------------------------------------------------------
# kana chart
#----------------------------------------------------------------------
KANAS = [
        ('あ', 'ア', 'a', 0),
        ('い', 'イ', 'i', 0),
        ('う', 'ウ', 'u', 0),
        ('え', 'エ', 'e', 0),
        ('お', 'オ', 'o', 0),

        ('か', 'カ', 'ka', 1),
        ('き', 'キ', 'ki', 1),
        ('く', 'ク', 'ku', 1),
        ('け', 'ケ', 'ke', 1),
        ('こ', 'コ', 'ko', 1),

        ('さ', 'サ', 'sa', 2),
        ('し', 'シ', 'shi', 2),
        ('す', 'ス', 'su', 2),
        ('せ', 'セ', 'se', 2),
        ('そ', 'ソ', 'so', 2),

        ('た', 'タ', 'ta', 3),
        ('ち', 'チ', 'chi', 3),
        ('つ', 'ツ', 'tsu', 3),
        ('て', 'テ', 'te', 3),
        ('と', 'ト', 'to', 3),

        ('な', 'ナ', 'na', 4),
        ('に', 'ニ', 'ni', 4),
        ('ぬ', 'ヌ', 'nu', 4),
        ('ね', 'ネ', 'ne', 4),
        ('の', 'ノ', 'no', 4),

        ('は', 'ハ', 'ha', 5),
        ('ひ', 'ヒ', 'hi', 5),
        ('ふ', 'フ', 'fu', 5),
        ('へ', 'ヘ', 'he', 5),
        ('ほ', 'ホ', 'ho', 5),

        ('ま', 'マ', 'ma', 6),
        ('み', 'ミ', 'mi', 6),
        ('む', 'ム', 'mu', 6),
        ('め', 'メ', 'me', 6),
        ('も', 'モ', 'mo', 6),

        ('や', 'ヤ', 'ya', 7),
        ('ゆ', 'ユ', 'yu', 7),
        ('よ', 'ヨ', 'yo', 7),

        ('ら', 'ラ', 'ra', 8),
        ('り', 'リ', 'ri', 8),
        ('る', 'ル', 'ru', 8),
        ('れ', 'レ', 're', 8),
        ('ろ', 'ロ', 'ro', 8),

        ('わ', 'ワ', 'wa', 9),
        ('を', 'ヲ', 'wo', 9),

        ('ん', 'ン', 'n', 10),
    ]


#----------------------------------------------------------------------
# hiragana/katakana -> romaji
#----------------------------------------------------------------------
ROMAJI = {}

for item in KANAS:
    hiragana = item[0]
    katakana = item[1]
    romaji = item[2]
    ROMAJI[hiragana] = romaji
    ROMAJI[katakana] = romaji


#----------------------------------------------------------------------
# config
#----------------------------------------------------------------------
class configure (object):

    def __init__ (self):
        self.dirhome = os.path.expanduser('~/.cache/kanaquiz')
        if not os.path.exists(self.dirhome):
            self.mkdir(self.dirhome)
        self.config = {}
        self.limit = 10
        self.cfgname = os.path.join(self.dirhome, 'quiz.tbl')
        self.load()

    def mkdir (self, path):
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

    def load (self):
        self.config = {}
        for item in KANAS:
            h, k = item[:2]
            self.config[h] = []
            self.config[k] = []
        self.config['h'] = []
        self.config['k'] = []
        self.config['a'] = []
        config = None
        try:
            with codecs.open(self.cfgname, 'r', encoding = 'utf-8') as fp:
                config = json.load(fp)
        except:
            pass
        if not config:
            return False
        if isinstance(config, dict):
            for k in config:
                v = config[k]
                if k in self.config:
                    self.config[k] = v
        return True

    def save (self):
        with codecs.open(self.cfgname, 'w', encoding = 'utf-8') as fp:
            json.dump(self.config, fp)
        return True

    # set terminal color
    def console (self, color):
        if sys.platform[:3] == 'win':
            import ctypes
            kernel32 = ctypes.windll.LoadLibrary('kernel32.dll')
            GetStdHandle = kernel32.GetStdHandle
            SetConsoleTextAttribute = kernel32.SetConsoleTextAttribute
            GetStdHandle.argtypes = [ ctypes.c_uint32 ]
            GetStdHandle.restype = ctypes.c_size_t
            SetConsoleTextAttribute.argtypes = [ ctypes.c_size_t, ctypes.c_uint16 ]
            SetConsoleTextAttribute.restype = ctypes.c_long
            handle = GetStdHandle(0xfffffff5)
            if color < 0: color = 7
            result = 0
            if (color & 1): result |= 4
            if (color & 2): result |= 2
            if (color & 4): result |= 1
            if (color & 8): result |= 8
            if (color & 16): result |= 64
            if (color & 32): result |= 32
            if (color & 64): result |= 16
            if (color & 128): result |= 128
            SetConsoleTextAttribute(handle, result)
        else:
            if color >= 0:
                foreground = color & 7
                background = (color >> 4) & 7
                bold = color & 8
                sys.stdout.write(" \033[%s3%d;4%dm"%(bold and "01;" or "", foreground, background))
                sys.stdout.flush()
            else:
                sys.stdout.write(" \033[0m")
                sys.stdout.flush()
        return 0

    # new record
    def update (self, kana, elapse):
        if kana not in self.config:
            return False
        if elapse is None:
            return False
        self.config[kana].append(elapse)
        if len(self.config[kana]) > self.limit:
            self.config[kana] = self.config[kana][-self.limit:]
        return True
    
    # average score
    def average (self, kana):
        records = self.config.get(kana)
        if not records:
            return None
        return float(sum(records)) / len(records)

    # best score
    def best (self, kana):
        records = self.config.get(kana)
        if not records:
            return None
        return min(records)



#----------------------------------------------------------------------
# entry
#----------------------------------------------------------------------
if __name__ == '__main__':
    def test1():
        cfg = configure()
        # cfg.config['a'] = [1,2,3]
        cfg.save()
        print(cfg.config)
        return 0
    test1()



