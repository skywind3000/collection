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
                    if '~' in filename:
                        filename = os.path.expanduser(filename)
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
            history = items['history']
            if '~' in history:
                history = os.path.expanduser(history)
            history = os.path.join(inihome, history)
            self.history = os.path.abspath(history)
        if 'log' in items:
            log = items['log'].strip()
            if not log:
                self.logname = None
            else:
                if '~' in log:
                    log = os.path.expanduser(log)
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
        path = os.path.dirname(dst)
        if not os.path.isdir(path):
            try:
                os.makedirs(path)
            except OSError:
                return False
        try:
            shutil.copyfile(src, dst)
            shutil.copystat(src, dst)
        except OSError as e:
            print(e)
            return False
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
        text = text.replace('##', '--')
        while index < size:
            ch = text[index]
            if index + 2 < size:
                nc = text[index + 1]
                if nc != '#':
                    code += bytes([ord(ch)])
                    index += 1
                else:
                    nc = text[index + 2]
                    if nc == '#':
                        code += bytes([ord(ch), ord('#'), ord('#')])
                        index += 3
                    else:
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

    def sol_dump (self):
        sols = []
        appdata = os.environ.get('APPDATA')
        if not appdata:
            return None
        prefix = 'Macromedia/Flash Player/#SharedObjects'
        solhome = os.path.abspath(os.path.join(appdata, prefix))
        size = len(solhome)
        for root, dirs, files in os.walk(solhome):
            for fn in files:
                if os.path.splitext(fn)[-1].lower() == '.sol':
                    path = os.path.abspath(os.path.join(root, fn))
                    short = path[size+1:]
                    sols.append((path, short))
        return sols

    def sol_local (self):
        sols = []
        for path, short in self.sol_dump():
            test = short.replace('\\', '/')
            p1 = test.find('/')
            if p1 < 0:
                continue
            p2 = test.find('/', p1 + 1)
            if p2 < 0:
                continue
            name = short[p1 + 1:p2]
            if name != 'localhost':
                continue
            mark = short[p2 + 1:]
            # print(mark)
            mark = self.config.sp_decode(mark)
            sols.append((path, mark))
        return sols

    def sol_list (self, limit = None):
        sols = self.sol_local()
        items = []
        for path, short in sols:
            # print(short)
            mtime = self.config.get_mtime(path)
            items.append((mtime, path, short))
        items.sort(reverse = True)
        if limit is not None:
            items = items[:limit]
        for mtime, path, short in items:
            ts = time.localtime(mtime * 0.001)
            ts = time.strftime('%Y-%m-%d %H:%M:%S', ts)
            try:
                print('%s'%(path,))
                print('(%s): %s'%(ts, short))
                print()
            except:
                pass
        return True


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
def main(args = None):
    args = args and args or sys.argv
    args = [ n for n in args ]
    options, args = getopt(args[1:])
    if not options:
        prog = os.path.split(__file__)[-1]
        print('usage: %s <operation> [ininame]'%prog)
        print('available operations:')
        print('    %s  {-l --list}  [ininame]'%prog)
        print('    %s  {-s --sync}  [ininame]'%prog)
        print()
        return 0
    if ('f' in options) or ('flash' in options):
        fs = FileSync()
        fs.sol_list()
        return 0
    ininame = None
    if len(args) > 0:
        ininame = args[0]
        if not os.path.exists(ininame):
            print('error: "%s" not exists'%ininame)
            return 1
        ininame = os.path.abspath(ininame)
    if not ininame:
        test = os.path.expanduser('~/.config/filesync2.ini')
        test = os.path.abspath(test)
        if os.path.exists(test):
            ininame = test
        else:
            print('error: default ini "%s" missing'%test)
            return 2
    if ('l' in options) or ('list' in options):
        fs = FileSync(ininame)
        fs.task_list()
        return 0
    elif ('s' in options) or ('sync' in options):
        fs = FileSync(ininame)
        fs.task_sync()
        return 0
    elif not options:
        print('error: empty operation')
        return 3
    else:
        print('error: unknow operation %s'%options)
        return 4
    return 0


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

    def test5():
        args = ['', '-l', 'filesync2.ini']
        args = ['', '-s', 'filesync2.ini']
        args = ['', '-f']
        main(args)
        return 0

    # test5()
    main()


