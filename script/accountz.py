#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# accounts.py - 
#
# Created by skywind on 2017/03/16
# Last change: 2017/03/16 11:58:41
#
#======================================================================
import sys
import time
import os
import sqlite3

try:
	import json
except:
	import simplejson as json

MySQLdb = None
pymongo = None


#----------------------------------------------------------------------
# python3 compatible
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
	unicode = str
	long = int



#----------------------------------------------------------------------
# AccountLocal
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
		    "name" VARCHAR(32),			
		    "pass" VARCHAR(64) DEFAULT(''),
			"mode" VARCHAR(16),
		    "gender" INTEGER DEFAULT (0),
			"credit" REAL DEFAULT (0),
		    "gold" REAL DEFAULT (0),
		    "level" INTEGER DEFAULT (0),
			"score" INTEGER DEFAULT (0),
			"birthday" DATE,
		    "icon" INTEGER DEFAULT (0),
		    "mail" VARCHAR(88),
		    "mobile" VARCHAR(32), 
			"sign" VARCHAR(32),			
		    "photo" VARCHAR(256),
			"intro" VARCHAR(256),
			"misc" TEXT,
			"ip" VARCHAR(70),
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



#----------------------------------------------------------------------
# initialize MySQLdb 
#----------------------------------------------------------------------
def mysql_init():
	global MySQLdb
	if MySQLdb is not None:
		return True
	try:
		import MySQLdb as _mysql
		MySQLdb = _mysql
	except ImportError:
		return False
	return True

def pymongo_init():
	global pymongo
	if pymongo is not None:
		return True
	try:
		import pymongo as _pymongo
		pymongo = _pymongo
	except ImportError:
		return False
	return True


