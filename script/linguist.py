#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# linguist.py - 
#
# Created by skywind on 2017/03/22
# Last change: 2017/03/22 13:44:42
#
#======================================================================
import sys, os, time

# https://www.nodebox.net/code/index.php/Linguistics

#----------------------------------------------------------------------
# python 2/3 compatible
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
	long = int
	xrange = range
	unicode = str


#----------------------------------------------------------------------
# 词形变换
#----------------------------------------------------------------------
class WordHelper (object):

	# 初始化
	def __init__ (self):
		self.__lemmatizer = None

	# 取得 WordNet 的词定义
	def definition (self, word, txt = False):
		from nltk.corpus import wordnet as wn
		syns = wn.synsets(word)
		output = []
		for syn in syns:
			name = syn.name()
			part = name.split('.')
			mode = part[1]
			output.append((mode, syn.definition()))
		if txt:
			output = '\n'.join([ (m + ' ' + n) for m, n in output ])
		return output

	# 取得动词的：-ing, -ed, -en, -s
	# NodeBox 的 Linguistics 软件包 11487 个动词只能处理 6722 个 
	def verb_tenses (self, word):
		word = word.lower()
		if ' ' in word:
			return None
		import en
		if not en.is_verb(word):
			return None
		tenses = {}
		try:
			tenses['i'] = en.verb.present_participle(word)
			tenses['p'] = en.verb.past(word)
			tenses['d'] = en.verb.past_participle(word)
			tenses['3'] = en.verb.present(word, person = 3, negate = False)
		except:
			return None
		return tenses

	# 取得所有动词
	def all_verbs (self):
		import en
		words = []
		for n in en.wordnet.all_verbs():
			words.append(n.form)
		return words

	# 取得所有副词
	def all_adverbs (self):
		import en
		words = []
		for n in en.wordnet.all_adverbs():
			words.append(n.form)
		return words

	# 取得所有形容词
	def all_adjectives (self):
		import en
		words = []
		for n in en.wordnet.all_adjectives():
			words.append(n.form)
		return words

	# 取得所有名词
	def all_adjectives (self):
		import en
		words = []
		for n in en.wordnet.all_nouns():
			words.append(n.form)
		return words

	# 返回原始单词
	def lemmatize (self, word, pos = 'n'):
		word = word.lower()
		if self.__lemmatizer is None:
			from nltk.stem.wordnet import WordNetLemmatizer
			self.__lemmatizer = WordNetLemmatizer()
		return self.__lemmatizer.lemmatize(word, pos)


#----------------------------------------------------------------------
# global
#----------------------------------------------------------------------
tools = WordHelper()


#----------------------------------------------------------------------
# testing
#----------------------------------------------------------------------
if __name__ == '__main__':
	def test1():
		for word in ['was', 'gave', 'be', 'bound']:
			print('%s -> %s'%(word, tools.lemmatize(word, 'v')))
		return 0
	test1()




