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
import time
import os
import codecs
import shutil


#----------------------------------------------------------------------
# configure
#----------------------------------------------------------------------
class Configure (object):

    def __init__ (self, ininame = None):
        self.dirname = os.path.dirname(os.path.abspath(__file__))
        if ininame is None:
            ininame = os.path.split(__file__)[-1]
            ininame = os.path.splitext(ininame)[0] + '.ini'
        logname = os.path.splitext(os.path.split(__file__)[-1])[0] + '.log'
        self.ininame = os.path.abspath(os.path.join(self.dirname, ininame))
        self.logname = os.path.abspath(os.path.join(self.dirname, logname))
        # print(self.ininame)
        self.config = self.read_ini(self.ininame)
        self.tasks = {}
        self.history = os.path.join(self.dirname, 'history')
        self.__read_tasks()
        self.__read_default()

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

    def __read_default (self):
        if 'default' not in self.config:
            return False
        items = self.config['default']
        inihome = os.path.dirname(self.ininame)
        if 'history' in items:
            self.history = os.path.join(inihome, items['history'])
            self.history = os.path.abspath(self.history)
        if 'log' in items:
            log = items['log'].strip()
            if not log:
                self.logname = None
            else:
                self.logname = os.path.abspath(os.path.join(inihome, log))
        return True

    def get_mtime (self, path):
        if not os.path.isfile(path):
            return -1
        return int(os.path.getmtime(path) * 1000)

    def copy_file (self, src, dst):
        src = os.path.abspath(src)
        dst = os.path.abspath(dst)
        if src == dst:
            return False
        shutil.copyfile(src, dst)
        shutil.copystat(src, dst)
        return True

    def need_update (self, task_name):
        if task_name not in self.tasks:
            return -1
        task = self.tasks[task_name]
        size = len(task)
        if size <= 0:
            return -1
        checks = []
        first_check = None
        for name in task:
            check = self.get_mtime(name)
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
        max_pos = 0
        max_val = checks[0]
        for i in range(size):
            if checks[i] > max_val:
                max_pos = i
                max_val = checks[i]
        return max_pos

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

    def sp_decode (self, text):
        code = b''
        size = len(text)
        index = 0
        while index < size:
            ch = text[index]
            if index + 2 < size:
                nc = text[index + 1]
                if nc != '#':
                    code += bytes([ord(ch)])
                    index += 1
                else:
                    nc = text[index + 2]
                    code += bytes([int(ch + nc, 16)])
                    index += 3
            else:
                code += bytes([ord(ch)])
                index += 1
        return code.decode('utf-8', 'ignore')


#----------------------------------------------------------------------
# FileSync
#----------------------------------------------------------------------
class FileSync (object):

    def __init__ (self, ininame = None):
        self.config = Configure(ininame)
        self.__log_fp = None

    # write log
    def log (self, *args):
        text = ' '.join([ str(n) for n in args ])
        if not self.__log_fp:
            fp = codecs.open(self.config.logname, 'a', encoding = 'utf-8')
            self.__log_fp = fp
        now = time.strftime('%Y-%m-%d %H:%M:%S')
        line = '[%s] %s'%(now, text)
        self.__log_fp.write(line + '\n')
        self.__log_fp.flush()
        print(line)
        return 0

    def task_list (self):
        tasks = self.config.tasks
        for name in tasks:
            print('%s:'%name)
            task = tasks[name]
            check = self.config.need_update(name)
            for index, fn in enumerate(task):
                if index != check:
                    print('    ' + fn)
                else:
                    print('  * ' + fn)
            print('')
        return 0

    def task_update (self, name):
        task = self.config.tasks.get(name)
        if not task:
            print('error: file group %s does not exist !'%name)
            return False
        check = self.config.need_update(name)
        if check < 0:
            print('file group %s is clear.'%name)
            return False
        source = task[check]
        self.log('[update] file group dirty: %s'%(name))
        self.log('source: ' + source)
        for index, fn in enumerate(task):
            if index != check:
                self.config.copy_file(source, task[index])
                self.log('sync -> %s'%(task[index], ))
        if not os.path.exists(self.config.history):
            os.makedirs(self.config.history)
        mtime = self.config.get_mtime(source)
        mtext = time.strftime('%Y%m%d_%H%M%S', time.localtime(mtime * 0.001))
        hname = name + '.' + mtext
        history = os.path.join(self.config.history, hname)
        self.log('record: %s'%hname)
        self.config.copy_file(source, history)
        return True

    def task_sync (self):
        for name in self.config.tasks:
            if self.config.need_update(name) >= 0:
                self.task_update(name)
        return True


#----------------------------------------------------------------------
# testing suit
#----------------------------------------------------------------------
if __name__ == '__main__':
    
    def test1():
        cfg = Configure()
        print(cfg.tasks)
        print(cfg.get_mtime(__file__))
        print(cfg.logname)
        # solsync.py
        # flashsol.py
        return 0

    def test2():
        ts = FileSync()
        ts.task_list()

    def test3():
        ts = FileSync()
        ts.config.copy_file('d:/file1.txt', 'd:/file4.txt')
        print(ts.config.get_mtime('d:/file1.txt'))
        print(ts.config.get_mtime('d:/file4.txt'))

    def test4():
        ts = FileSync()
        ts.task_sync()
        return 0

    test4()

