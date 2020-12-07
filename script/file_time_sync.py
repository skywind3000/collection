#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# file_time_sync.py - 
#
# Created by skywind on 2020/12/08
# Last Modified: 2020/12/08 01:08:36
#
#======================================================================
from __future__ import print_function, unicode_literals
import sys
import os


#----------------------------------------------------------------------
# configure
#----------------------------------------------------------------------
class Configure (object):

    def __init__ (self, ininame = None):
        self.dirname = os.path.dirname(os.path.abspath(__file__))
        if ininame is None:
            ininame = os.path.split(__file__)[-1]
            ininame = os.path.splitext(ininame)[0] + '.ini'
        self.ininame = os.path.abspath(os.path.join(self.dirname, ininame))
        # print(self.ininame)
        self.config = self.read_ini(self.ininame)
        self.tasks = {}
        self.history = os.path.join(self.dirname, 'history')
        self.__read_tasks()
        self.__find_history()

    def __read_tasks (self):
        self.tasks = {}
        inihome = os.path.dirname(self.ininame)
        for name in self.config:
            if name == 'default':
                continue
            items = self.config[name]
            task = []
            index = 1
            while True:
                key = 'file%d'%index
                if key not in items:
                    break
                filename = items[key].strip()
                if filename:
                    if not os.path.isabs(filename):
                        filename = os.path.join(inihome, filename)
                    filename = os.path.abspath(filename)
                    task.append(filename)
                index += 1
            if task:
                self.tasks[name] = task
        return True

    def __find_history (self):
        if 'default' not in self.config:
            return False
        items = self.config['default']
        if 'history' in items:
            self.history = items['history']
        return True

    def get_mtime (self, path):
        if not os.path.isfile:
            return None
        return os.path.getmtime(path)

    def need_update (self, task_name):
        if task not in self.tasks:
            return -1
        task = self.tasks[task_name]
        size = len(task)
        checks = []
        first_check = None
        for name in task:
            check = self.get_mtime()
            if not checks:
                first_check = check
            checks.append(check)
        equals = True
        for index, name in enumerate(task):
            if first_check != checks[index]:
                equals = False
                break
        if equals:
            return -1
        return -1

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


#----------------------------------------------------------------------
# 
#----------------------------------------------------------------------
if __name__ == '__main__':
    
    def test1():
        cfg = Configure()
        print(cfg.tasks)
        return 0

    test1()


