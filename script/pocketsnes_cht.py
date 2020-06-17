#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# pocketsnes_cht.py - 
#
# Created by skywind on 2020/06/17
# Last Modified: 2020/06/17 20:26:39
#
#======================================================================
from __future__ import print_function, unicode_literals
import sys
import os
import time
import struct
import codecs


#----------------------------------------------------------------------
# CheatItem
#----------------------------------------------------------------------
class CheatItem (object):

    def __init__ (self, name = '', address = 0, byte = 0, enable = False):
        self.name = name
        self.address = address
        self.byte = byte
        self.enable = enable
        self.saved = False
        self.saved_byte = 0

    def encode (self, pocketsnes = True):
        namesize = pocketsnes and 48 or 20
        head = [0] * 8
        if self.enable:
            head[0] |= 4
        if self.saved:
            head[0] |= 8
        head[1] = self.byte
        head[2] = (self.address >> 0) & 0xff
        head[3] = (self.address >> 8) & 0xff
        head[4] = (self.address >> 16) & 0xff
        head[5] = self.saved_byte
        head[6] = 254
        head[7] = 252
        name = self.name
        if not isinstance(name, bytes):
            name = name.encode('utf-8', 'ignore')
        name = name[:namesize]
        if len(name) < namesize:
            name += b'\x00' * (namesize - len(name))
        data = b''
        for n in head:
            data += struct.pack('<B', n)
        return data + name

    def decode (self, data, pocketsnes = True):
        if not isinstance(data, bytes):
            data = data.encode('ascii', 'ignore')
        namesize = pocketsnes and 48 or 20
        if len(data) != namesize + 8:
            raise ValueError('invalid binary cheat size')
        head = [ int(n) for n in data[:8] ]
        if head[6] != 254 or head[7] != 252:
            raise ValueError('invalid binary cheat data')
        self.address = head[2] + (head[3] << 8) + (head[4] << 16)
        self.byte = head[1]
        self.saved_byte = head[5]
        self.saved = (head[0] & 8) and True or False
        self.enable = (head[0] & 4) and True or False
        name = data[8:]
        p1 = name.find(b'\x00')
        if p1 > 0:
            name = name[:p1]
        self.name = name.decode('utf-8', 'ignore')
        return 0

    def code (self):
        return '%06x=%02x'%(self.address, self.byte)

    def __repr__ (self):
        return 'CheatItem(%s, %d, %d, %s)'%(repr(self.name), self.address,
                self.byte, repr(self.enable))

    def __str__ (self):
        return '%s:%s'%(self.name, self.code())


#----------------------------------------------------------------------
# CheatFile
#----------------------------------------------------------------------
class CheatFile (object):

    def __init__ (self):
        self.cheats = []

    def __iter__ (self):
        return self.cheats.__iter__()

    def __getitem__ (self, key):
        return self.cheats[key]

    def __len__ (self):
        return len(self.cheats)

    def snes9x_load (self, filename):
        self.cheats = []
        cheats = {}
        index = -1
        avail = False
        with codecs.open(filename, 'r', encoding = 'utf-8') as fp:
            for line in fp:
                line = line.rstrip('\r\n\t ')
                if not line:
                    continue
                space = 0
                while space < len(line):
                    if line[space].isspace():
                        space += 1
                    else:
                        break
                if space == 0:
                    if line == 'cheat':
                        index += 1
                        avail = True 
                    else:
                        avail = False
                elif avail:
                    text = line.strip('\r\n\t ')
                    if not text:
                        continue
                    if index not in cheats:
                        cheats[index] = {}
                    if text == 'enable':
                        cheats[index]['enable'] = True
                    elif ':' in text:
                        key, _, val = text.partition(':')
                        key = key.strip()
                        val = val.strip()
                        cheats[index][key] = val
        size = index
        for i in range(size + 1):
            if i not in cheats:
                continue
            ni = cheats[i]
            if 'name' not in ni:
                continue
            if 'code' not in ni:
                continue
            code = ni['code']
            if '+' in code:
                continue
            if '=' not in code:
                continue
            if '?' in code:
                code = code[:code.find('?')]
            if len(code) != 9:
                continue
            if code[6] != '=':
                continue
            address = int(code[:6], 16)
            byte = int(code[7:9], 16)
            enable = ni.get('enable', False) and True or False
            cc = CheatItem(ni['name'].strip(), address, byte, enable)
            self.cheats.append(cc)
        return 0

    def snes9x_save (self, filename):
        with codecs.open(filename, 'w', encoding = 'utf-8') as fp:
            for cheat in self.cheats:
                fp.write('cheat\n')
                fp.write('  name: %s'%cheat.name)
                fp.write('  code: %s\n'%(cheat.code(),))
                if cheat.enable:
                    fp.write('  enable\n')
                fp.write('\n')
        return 0

    def pocketsnes_load (self, filename, legacy = False):
        self.cheats = []
        newfmt = (not legacy) and True or False
        namesize = newfmt and 48 or 20
        cheatsize = namesize + 8
        with open(filename, 'rb') as fp:
            data = fp.read(cheatsize)
            if len(data) == cheatsize:
                cc = CheatItem()
                cc.decode(data, newfmt)
                self.cheats.append(cc)
        return 0

    def pocketsnes_save (self, filename, legacy = False):
        with open(filename, 'wb') as fp:
            for cheat in self.cheats:
                data = cheat.encode((not legacy) and True or False)
                fp.write(data)
        return 0



#----------------------------------------------------------------------
# testing suit
#----------------------------------------------------------------------
if __name__ == '__main__':
    def test1():
        c1 = CheatItem('hello', 12345, 99)
        data = c1.encode()
        print(c1)
        c2 = CheatItem('life', 11, 2)
        c2.decode(data)
        print(c2)
        print(repr(c2))
        import ascmini
        ascmini.utils.print_binary(data)
        return 0
    def test2():
        cf = CheatFile()
        cf.snes9x_load('d:/games/emulator/snes/cheats/sunset.cht')
        for cc in cf:
            print(cc)
        cf.pocketsnes_save('sunset2.cht')
        return 0
    test2()


