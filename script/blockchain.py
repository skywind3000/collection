#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# blockchain.py - 
#
# Created by skywind on 2018/03/08
# Last Modified: 2018/03/08 18:08:29
#
#======================================================================
from __future__ import print_function
import sys
import time
import hashlib
import json


#----------------------------------------------------------------------
# Blockchain
#----------------------------------------------------------------------
class Blockchain(object):

	def __init__ (self):
		self.chain = []
		self.current_transactions = []
		self.new_block(previous_hash = 1, proof = 100)

	def new_block (self, proof, previous_hash = None):
		block = {
				'index': len(self.chain) + 1,
				'timestamp': time.time(),
				'transactions': self.current_transactions,
				'proof': 'proof',
				'previous_hash': previous_hash or self.hash(self.chain[-1]),
				}
		self.current_transactions = []
		self.chain.append(block)
		return block

	def new_transaction (self, sender, recipient, amount):
		self.current_transactions.append({
			'sender': sender,
			'recipient': recipient,
			'amount': amount,
			})
		if not self.chain:
			return 1
		return self.last_block['index'] + 1

	@staticmethod
	def hash (block):
		block_string = json.dumps(block, sort_keys = True).encode()
		return hashlib.sha256(block_string).hexdigest()

	@property
	def last_block (self):
		if not self.chain:
			return None
		return self.chain[-1]

