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
from __future__ import unicode_literals, print_function
import sys
import time
import os
import codecs
import json
import random


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
    hiragana, katakana, romaji = item[:3]
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
        self.config['t'] = []
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
            json.dump(self.config, fp, indent = 4)
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

    # echo text
    def echo (self, color, text):
        self.console(color)
        sys.stdout.write(text)
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
# kquiz
#----------------------------------------------------------------------
class kquiz (object):

    def __init__ (self):
        self.config = configure()
        self.tokens = [] 

    def disorder (self, array):
        array = [ n for n in array ]
        output = []
        while array:
            size = len(array)
            pos = random.randint(0, size - 1)
            output.append(array[pos])
            array[pos] = array[size - 1]
            array.pop()
        return output

    def select (self, source):
        tokens = []
        if source not in ('hiragana', 'katakana', 'all'):
            source = 'hiragana'
        if source in ('hiragana', 'all'):
            for item in KANAS:
                tokens.append(item[0])
        if source in ('katakana', 'all'):
            for item in KANAS:
                tokens.append(item[1])
        return tokens

    def trinity (self, source):
        tokens = self.select(source)
        tokens = self.disorder(tokens)
        target = [ n for n in tokens ]
        trinity = []
        if not tokens:
            return []
        size = len(tokens)
        for i in range((size + 2) // 3):
            k1 = tokens[random.randint(0, size - 1)]
            k2 = tokens[random.randint(0, size - 1)]
            k3 = tokens[random.randint(0, size - 1)]
            if target:
                k1 = target.pop()
            if target:
                k2 = target.pop()
            if target:
                k3 = target.pop()
            trinity.append(k1 + k2 + k3)
        return trinity

    def echo (self, color, text):
        return self.config.echo(color, text)

    def single_quiz (self, word, heading = ''):
        romans = ''.join([ ROMAJI[c] for c in word ])
        self.config.console(-1)
        self.echo(7, '[')
        self.echo(14, word)
        self.echo(7, ']')
        if heading:
            self.echo(8, ' ' + heading)
        self.echo(-1, '\n')
        answer = None
        ts = time.time()
        while 1:
            self.echo(7, '? ')
            if sys.version_info[0] < 3:
                answer = raw_input()
            else:
                answer = input()
            answer = answer.strip()
            if answer:
                break
        ts = time.time() - ts
        if answer == romans:
            self.echo(2, 'correct')
            self.echo(8, ' (time %.2f)\n'%ts)
            hr = ts
        else:
            self.echo(1, 'wrong')
            self.echo(8, ' (%s->%s)\n'%(answer, romans))
            hr = None
        self.echo(-1, '\n')
        return hr



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
    def test2():
        quiz = kquiz()
        token = quiz.trinity('all')
        import pprint
        pprint.pprint(token)
        print(len(token), len(KANAS))
        return 0
    def test3():
        quiz = kquiz()
        print(quiz.single_quiz('かか', '(1/100)'))
    test1()



