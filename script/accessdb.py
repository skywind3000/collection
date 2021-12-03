#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# accessdb.py - ��Ϸ�ı������ݿ⣬�ļ����ݿ⣬��¼������
#
# NOTE:
# ע�⣬���ļ��Ĳ�����������·������
#
#======================================================================
from __future__ import print_function
import sys, os
import time
import sqlite3

try:
    import json
except:
    import simplejson as json

#----------------------------------------------------------------------
# compatible
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
    xrange = range
    long = int


#----------------------------------------------------------------------
# export
#----------------------------------------------------------------------
__all__ = ["AccountLocal", "GameDBLocal" ]


#----------------------------------------------------------------------
# ����С���ʻ����ݿ�
#----------------------------------------------------------------------
class AccountLocal (object):

    def __init__ (self, filename):
        self.__dbname = os.path.abspath(filename)
        self.__conn = None
        self.__open()
        self.mode = 0
    
    def __open (self):
        sql = '''
        CREATE TABLE IF NOT EXISTS "account" (
            "uid" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,
            "urs" VARCHAR(88) NOT NULL UNIQUE,
            "cid" INTEGER DEFAULT (0),
            "name" VARCHAR(88),         
            "pass" VARCHAR(32) DEFAULT(''),
            "gender" INTEGER DEFAULT (0),
            "credit" REAL DEFAULT (0),
            "gold" REAL DEFAULT (0),
            "level" INTEGER DEFAULT (0),
            "score" INTEGER DEFAULT (0),
            "birthday" DATE,
            "icon" INTEGER DEFAULT (0),
            "mail" VARCHAR(88),
            "mobile" VARCHAR(64), 
            "sign" VARCHAR(64),         
            "photo" VARCHAR(256),
            "intro" VARCHAR(256),
            "misc" VARCHAR(256),
            "RegDate" DATETIME,
            "LastLoginDate" DATETIME,
            "LoginTimes" INTEGER DEFAULT (0),
            "CreditConsumed" REAL DEFAULT (0),
            "GoldConsumed" REAL DEFAULT (0)
        );
        CREATE UNIQUE INDEX IF NOT EXISTS "account_index1" ON account (uid);
        CREATE UNIQUE INDEX IF NOT EXISTS "account_index2" ON account (urs, pass);
        CREATE INDEX IF NOT EXISTS "account_index3" ON account (cid);
        '''

        self.__conn = sqlite3.connect(self.__dbname, isolation_level = 'IMMEDIATE')
        self.__conn.isolation_level = 'IMMEDIATE'

        sql = '\n'.join([ n.strip('\t') for n in sql.split('\n') ])
        sql = sql.strip('\n')

        self.__conn.executescript(sql)
        self.__conn.commit()
    
        self.__names = {
            'uid':0, 'urs':1, 'cid':2, 'name':3, 'pass':4, 'gender':5, 
            'credit':6, 'gold':7, 'level':8, 'score':9, 'birthday':10,
            'icon':11, 'mail':12, 'mobile':13, 'sign':14, 'photo':15, 
            'intro':16, 'misc':17, 'RegDate':18, 'LastLoginDate':19, 
            'LogTimes':20, 'CreditConsumed':21, 'GoldConsumed':22, 
        }
        self.__items = self.__names.items()
        x = ('cid', 'name', 'pass', 'gender', 'icon', 'mail', 'mobile', 
            'photo', 'misc', 'level', 'score', 'birthday', 'sign', 
            'intro')
        self.__enable = {}
        for n in x:
            self.__enable[n] = self.__names[n]

        return True

    
    # �����ݿ��¼ת��Ϊ�ֵ�
    def __record2obj (self, record):
        if record is None:
            return None
        user = {}
        for k, v in self.__items:
            if k != 'pass':
                user[k] = record[v]
        return user
    
    # �ر����ݿ�����
    def close (self):
        if self.__conn:
            self.__conn.close()
        self.__conn = None

    # �ر����ݿ�����
    def __del__ (self):
        self.close()
    

    # ��¼�������û��������룬�����û����ݣ�����Ϊ None�Ļ�ǿ�Ƶ�¼
    def login (self, urs, passwd):
        sql1 = 'select * from account where urs = ? and pass = ?;'
        sql2 = 'select * from account where urs = ?;'
        record = None
        c = self.__conn.cursor()
        try:
            if passwd is not None:
                c.execute(sql1, (urs, passwd))
            else:
                c.execute(sql2, (urs,))
            record = c.fetchone()
            if record is None:
                c.close()
                return None
            x = "update account set "
            x += "LastLoginDate = datetime('now', 'localtime'),"
            x += "LoginTimes = LoginTimes + 1"
        except sqlite3.IntegrityError:
            c.close()
            return None
        c.close()
        if self.mode == 0:
            try:
                self.__conn.execute(x + ' where urs = ?;', (urs,))
                self.__conn.commit()
            except sqlite3.IntegrityError:
                self.__conn.rollback()
                return None
        return self.__record2obj(record)

    
    # ��ѯ�û���Ϣ��
    #   �� urs��ȡ��Ϣ��urs != None, uid == None
    #   �� uid��ȡ��Ϣ��urs == None, uid != None
    #   ��֤ urs/uidƥ�䣺urs != None, uid != None
    # �ɹ������û���¼��ʧ�ܷ��� None
    def query (self, urs = None, uid = None):
        if urs is None and uid is None:
            return None
        c = self.__conn.cursor()
        record = None
        try:
            if urs is not None and uid is None:
                c.execute('select * from account where urs = ?;', (urs,))
            elif urs is None and uid is not None:
                c.execute('select * from account where uid = ?;', (uid,))
            else:
                x = 'select * from account where urs = ? and uid = ?;'
                c.execute(x, (urs, uid))
            record = c.fetchone()
        except:
            c.close()
            return None
        return self.__record2obj(record)
    

    # �û�ע�ᣬ���ؼ�¼
    def register (self, urs, passwd, name, gender = 0):
        try:
            sql = 'INSERT INTO account(urs, pass, name, gender, RegDate) '
            sql += "VALUES(?, ?, ?, ?, datetime('now', 'localtime'));"
            self.__conn.execute(sql, (urs, passwd, name, gender))
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
            return None
        return self.query(urs)
    

    # �û���������, changes��һ���ֵ��ʽ�� query������ͬ�����������ֶ��У�
    # cid, name, pass, gender, icon, mail, mobile, photo, misc, level, 
    # score, intro, sign, birthday
    def update (self, uid, changes):
        names, values = [], []
        for k, v in changes.items():
            if k not in self.__enable:
                return False
            names.append(k)
            values.append(v)
        if not values:
            return False
        sql = 'UPDATE account SET ' + ', '.join(['%s=?'%n for n in names])
        sql += ' WHERE uid=%d;'%uid
        try:
            self.__conn.execute(sql, tuple(values))
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
            return False
        return True
    
    # ֧��Ǯ��kindΪ 'credit'�� 'gold'��money����Ҫ֧����Ǯ��
    # ���� (���, ���ж���Ǯ, ����ԭ��) 
    # ���=0֧���ɹ������=1�û������ڣ����=2Ǯ���������=3δ֪����
    def payment (self, uid, kind, money):
        kind = kind.lower()
        if kind not in ('credit', 'gold'):
            return (-1, 0, 'money kind error %s'%kind)
        if kind == 'credit':
            x1, x2 = ('credit', 'CreditConsumed')
        else:
            x1, x2 = ('gold', 'GoldConsumed')
        sql = 'UPDATE account SET %s=%s-?, %s=%s+? WHERE uid=? and %s>=?;'
        sql = sql%(x1, x1, x2, x2, x1)
        changes = self.__conn.total_changes
        try:
            self.__conn.execute(sql, (money, money, uid, money))
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
        changed = self.__conn.total_changes - changes
        data = self.query(None, uid)
        if data is None:
            return (1, 0, 'bad uid %d'%uid)
        if changed == 0:
            if data[x1] < money:
                return (2, data[x1], 'not enough %s'%x1)
            return (3, data[x1], 'unknow payment error')
        return (0, data[x1], 'ok')
    
    # ��Ǯ��kindΪ 'credit'�� 'gold'��money����Ҫ���ӵ�Ǯ��
    def deposit (self, uid, kind, money):
        kind = kind.lower()
        if kind not in ('credit', 'gold'):
            return (-1, 0, 'money kind error %s'%kind)
        sql = 'UPDATE account SET %s=%s+? WHERE uid=?'%(kind, kind)
        changes = self.__conn.total_changes
        try:
            self.__conn.execute(sql, (money, uid))
            self.__conn.commit()
        except:
            self.__conn.rollback()
        changed = self.__conn.total_changes - changes
        data = self.query(None, uid)
        if data is None:
            return (1, 0, 'bad uid %d'%uid)
        if changed == 0:
            return (2, data[kind], 'unknow deposit error')
        return (0, data[kind], 'ok')

    # �����ݿ���������¼�����ڲ���
    def population (self, count = 100):
        x = 'INSERT INTO account(urs, name, pass, gender, RegDate) '
        y = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
        succeed = 0
        for i in xrange(count):
            z = "VALUES('10%d@qq.com', 'name%d', '****', '%d', '%s');"%( 
                i + 1, i + 1, i % 3, y)
            sql = x + z
            try:
                self.__conn.execute(sql)
                succeed += 1
            except:
                pass
        self.__conn.commit()
        return succeed


    # ���ԣ�
    def test (self):
        c = self.__conn.execute('select uid, name from account;')
        print(dir(c))
        for c, desc in enumerate(c.description):
            print(c, desc[0])
        return 0


#----------------------------------------------------------------------
# ����С����Ϸ���ݿ⣺
#----------------------------------------------------------------------
class GameDBLocal (object):

    def __init__ (self, filename):
        self.__dbname = os.path.abspath(filename)
        self.__conn = None
        self.__open()
    
    def __open (self):
        self.__conn = sqlite3.connect(self.__dbname, isolation_level = 'IMMEDIATE')
        self.__conn.isolation_level = 'IMMEDIATE'
        sql = '''
        CREATE TABLE IF NOT EXISTS "data" (
          uid           integer PRIMARY KEY NOT NULL UNIQUE,
          nick          varchar(64) DEFAULT NULL,
          title         varchar(64) DEFAULT NULL,
          privilege     integer DEFAULT 0,
          score         integer DEFAULT 0,
          coin1         integer DEFAULT 0,
          coin2         integer DEFAULT 0,
          hp            integer DEFAULT 0,
          mp            integer DEFAULT 0,
          maxhp         integer DEFAULT 0,
          maxmp         integer DEFAULT 0,
          exp           integer DEFAULT 0,
          level         integer DEFAULT 0,
          achieve       integer DEFAULT 0,
          point         integer DEFAULT 0,
          skill         text DEFAULT NULL,
          equip         text DEFAULT NULL,
          item          text DEFAULT NULL,
          effect        text DEFAULT NULL,
          sect1         text DEFAULT NULL,
          sect2         text DEFAULT NULL,
          sect3         text DEFAULT NULL,
          sect4         text DEFAULT NULL,
          nwin          integer DEFAULT 0,
          ndraw         integer DEFAULT 0,
          nlose         integer DEFAULT 0,
          nbreak        integer DEFAULT 0,
          attrib        text DEFAULT NULL,
          buddy         text DEFAULT NULL,
          msgbox        text DEFAULT NULL,
          consumed1     bigint DEFAULT 0,
          consumed2     bigint DEFAULT 0,
          bantime       DATETIME DEFAULT NULL,
          playtimes     integer DEFAULT 0,
          lastlogin     DATETIME DEFAULT NULL,
          regdate       DATETIME DEFAULT NULL
        );
        CREATE TABLE IF NOT EXISTS "game" (
          key   varchar(64) PRIMARY KEY NOT NULL UNIQUE,
          data  text,
          version   integer DEFAULT 0,
          recent    datetime
        );
        CREATE UNIQUE INDEX IF NOT EXISTS "data_index1" ON data (uid);
        CREATE INDEX IF NOT EXISTS "data_index2" ON data (nick);
        CREATE UNIQUE INDEX IF NOT EXISTS "game_index" ON game (key);
        '''

        sql = '\n'.join([ n.strip('\t') for n in sql.split('\n') ])
        sql = sql.strip('\n')

        self.__conn.executescript(sql)
        self.__conn.commit()

        self.namevec = ( 'uid', 'nick', 'title', 'privilege', 'score', 
            'coin1', 'coin2', 'hp', 'mp', 'maxhp', 'maxmp', 'exp', 'level',
            'achieve', 'point', 'skill', 'equip', 'item', 'effect', 'sect1',
            'sect2', 'sect3', 'sect4', 'nwin', 'ndraw', 'nlose', 'nbreak', 
            'attrib', 'buddy', 'msgbox', 'consumed1', 'consumed2', 'bantime', 
            'playtimes', 'lastlogin', 'regdate' )

        self.namecol = {}
        
        for i in xrange(len(self.namevec)):
            self.namecol[self.namevec[i]] = i

        a, b = self.namecol['sect1'], self.namecol['sect2']
        c, d = self.namecol['sect3'], self.namecol['sect4']

        self.sects = { 'sect1':a, 'sect2':b, 'sect3':c, 'sect4':d }
        self.jsons = { 'skill':1, 'equip':1, 'item':1, 'effect':1,
            'attrib':1, 'buddy':1, 'msgbox':1 }

        self.integer = { 'privilege':1, 'score':1, 'coin1':1, 'coin2':1,
            'hp':1, 'mp':1, 'maxhp':1, 'maxmp':1, 'exp':1, 'level':1,
            'achieve':1, 'point':1, 'nwin':1, 'ndraw':1, 'nlose':1, 
            'nbreak':1 }

        self.readonly = {'uid':1, 'playtimes':1, 'lastlogin':1, 'regdate':1 }

        return 0
    
    # �ر����ݿ�
    def close (self):
        if self.__conn:
            self.__conn.close()
        self.__conn = None
        return 0

    # �ر����ݿ�����
    def __del__ (self):
        self.close()

    # ������Ϸ���ݣ�תjson��
    def game_data_set (self, key, data):
        sql1 = 'INSERT OR IGNORE INTO game(key) VALUES(?);'
        sql2 = 'UPDATE game SET version = version + 1, data = ?, '
        sql2 += "recent = datetime('now', 'localtime') where key = ?;"
        text = json.dumps(data)
        try:
            self.__conn.execute(sql1, (key,))
            self.__conn.execute(sql2, (text, key))
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
            return False
        return True
    
    # ȡ����Ϸ��������
    def game_data_dump (self):
        rows = {}
        c = self.__conn.cursor()
        c.execute('select * from game;')
        for row in c:
            try:
                d = json.loads(row[1])
            except:
                d = {}
            rows[row[0]] = d
        c.close()
        return rows
    
    # ��Ϸ���ݸ���
    def game_data_update (self, data):
        for key, value in data.iteritems():
            sql1 = 'INSERT OR IGNORE INTO game(key) VALUES(?);'
            sql2 = 'UPDATE game SET version = version + 1, data = ?, '
            sql2 += "recent = datetime('now', 'localtime') where key = ?;"
            text = json.dumps(value)
            try:
                self.__conn.execute(sql1, (key,))
                self.__conn.execute(sql2, (text, key))
            except sqlite3.IntegrityError:
                self.__conn.rollback()
                return False
        try:
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
            return False
        return True

    # �û����ݶ�ȡ
    def user_data_touch (self, uid):
        try:
            c = self.__conn.execute('SELECT * FROM data WHERE uid=?', (uid,))
            if c.fetchone() is None:
                sql = 'INSERT OR IGNORE INTO data(uid, regdate) '
                sql += "VALUES(?, datetime('now', 'localtime'));"
                self.__conn.execute(sql, (uid,))
            else:
                sql = "UPDATE data SET lastlogin=datetime('now', 'localtime'), "
                sql += 'playtimes = playtimes + 1 WHERE uid=?;'
                self.__conn.execute(sql, (uid,))
            c.close()
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
            return False
        return True
    
    # �û����ݶ�ȡ
    def user_data_get (self, uid):
        c = self.__conn.cursor()
        try:
            c.execute('select * from data where uid=?', (uid,))
        except sqlite3.IntegrityError:
            return None

        row = c.fetchone()
        if row is None:
            return None

        data = [ {}, {}, {}, {}, {} ]

        for i in xrange(0, len(self.namevec)):
            name = self.namevec[i]
            ncol = self.namecol[name]
            if name not in self.sects:
                if name in self.jsons:
                    value = None
                    try:
                        value = json.loads(row[ncol])
                    except:
                        pass
                else:
                    value = row[ncol]
                data[0][name] = value
        
        for i in xrange(1, 5):
            name = 'sect%d'%i
            ncol = self.namecol[name]
            try:
                x = json.loads(row[ncol])
                if isinstance(x, type({})):
                    data[i] = x
            except:
                pass

        return data
    
    # �û�����д��
    def user_data_set (self, uid, data):
        if not isinstance(data, type([])):
            return False
        if len(data) == 0:
            return False
        if not isinstance(data[0], type({})):
            return False

        ps, vs = [], []
        namevec = self.namevec

        for i in xrange(1, len(namevec)):
            name = namevec[i]
            if name in self.namecol and name in data[0]:
                if (name in self.sects) or (name in self.readonly):
                    continue
                if name in ('nick', 'title'):
                    if data[0][name] is not None:
                        ps.append('%s=?'%name)
                        vs.append(data[0][name])
                    else:
                        ps.append('%s=NULL'%name)
                elif name in self.jsons:
                    if data[0][name] is not None:
                        ps.append('%s=?'%name)
                        vs.append(json.dumps(data[0][name]))
                    else:
                        ps.append('%s=NULL'%name)
                elif name in self.integer:
                    value = 0
                    try:
                        value = int(data[0][name])
                    except:
                        continue
                    ps.append('%s=?'%name)
                    vs.append(value)
                elif name in ('consumed1', 'consumed2'):
                    value = 0
                    try:
                        value = long(data[0][name])
                    except:
                        continue
                    ps.append('%s=?'%name)
                    vs.append(value)
        
        for i in xrange(1, 5):
            if len(data) <= i:
                continue
            if data[i] is None:
                continue
            if not isinstance(data[i], type({})):
                continue
            ps.append('sect%d=?'%i)
            vs.append(json.dumps(data[i]))
        
        if len(ps) == 0:
            return True
        
        vs.append(uid)
            
        sql = 'UPDATE data SET ' + ', '.join(ps) + ' WHERE uid=?;'
        #print sql

        try:
            self.__conn.execute(sql, tuple(vs))
            self.__conn.commit()
        except sqlite3.IntegrityError:
            self.__conn.rollback()
            return False
    
        return True

    # �û����ݴ�ӡ
    def user_data_to_string (self, data):
        text = []
        for i in xrange(5):
            text.append('sect%d:'%i)
            sect = data[i]
            if i == 0:
                for n in self.namevec:
                    if n not in sect:
                        continue
                    text.append('    %s=%s'%(n, sect[n]))
            else:
                for n in sect:
                    text.append('    %s=%s'%(n, sect[n]))
        return '\n'.join(text)


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':
    def test1():
        db1 = AccountLocal('../../account.db')
        db2 = GameDBLocal('../../game1.db')
        # db1.population(200)
        t1 = time.time()
        print(db1.login('skywind3000@163.com', None))
        # print(db1.login('skywind3000@163.com', '1234'))
        print('-----')
        t1 = time.time() - t1
        #print(db1.query('10171@qq.com')))
        #print(db1.register('skywind3000@163.com', '000000', 'skywind')))
        #print(db1.update(171, { 'nick':'jack', 'level':15, 'gender':3 })))
        #print(db2.userdat_set(102, {3:4, 5:6})))
        t2 = time.time()
        print(db2.game_data_set('map1', {1:2, 3:4}))
        t2 = time.time() - t2
        t3 = time.time()
        #print(db2.userdat_get(102))
        #print(db2.gamedat_get('map1'))
        print(db2.game_data_dump())
        t3 = time.time() - t3
        print(t1, t2, t3)
        print('-' * 72)
        print(db2.game_data_dump())
        db1.test()
    def test2():
        db2 = GameDBLocal('../../game/game1.db')
        #for i in xrange(100):
        #   t = time.time()
        #   db2.user_data_touch(i)
        #   print('time', time.time() - t)
        x = db2.user_data_get(1)
        print(x)
        x[0]['nick'] = 'zhj'
        x[0]['title'] = None
        x[0]['item'] = [1,2,3,4,5]
        x[1]['case'] = [10,20]
        db2.user_data_set(2, x)
        print(db2.user_data_get(2))
        print(db2.user_data_to_string(x))
    def test3():
        db = AccountLocal('account.db')
        uid = 1
        print(db.update(uid, {'credit':1000}))
        print(db.payment(uid, 'credit', 100))
        print(db.deposit(uid, 'credit', 200))
        db.test()
        #db.close()
    def test4():
        return 0
    test4()


