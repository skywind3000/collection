#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# asyncredis.py - async redis client
#
# NOTE:
# visit http://redis.io/topics/protocol for more information
#
#======================================================================
import sys
import time
import socket
import errno
import random

#----------------------------------------------------------------------
# GLOBAL
#----------------------------------------------------------------------
READ_CHAR	= 0		# 读取一个字符
READ_LINE	= 1		# 读取一行数据（以\n结束的数据块）
READ_BLOCK	= 2		# 读取一个给定长度的数据块

NET_STATE_CLOSED = 0			# state: init value
NET_STATE_CONNECTING = 1		# state: connecting
NET_STATE_ESTAB = 2		# state: connected


#----------------------------------------------------------------------
# Redis Error
#----------------------------------------------------------------------
class RedisError (ValueError):
	def __init__ (self, *args, **argv):
		super(RedisError, self).__init__(*args, **argv)


#----------------------------------------------------------------------
# async reader
#----------------------------------------------------------------------
class reader (object):

	def __init__ (self):
		self._buffer = ''
		self._input = []
		self._ready = []
		self._mode = READ_CHAR
		self._need = 0
		self._size = 0
		self._position = 0

	# 输入数据
	def feed (self, text):
		if len(text) == 0:
			return 0
		self._input.append(text)
		self._size += len(text)
		return len(text)
	
	# 根据当前的模式，尝试读出一个完整的数据块，数据不够返回 None
	def read (self):
		if self._buffer == '' and len(self._input) == 0:
			return None
		if self._mode == READ_CHAR:
			if self._buffer:
				ch = self._buffer[:1]
				self._buffer = self._buffer[1:]
				self._size -= 1
				return ch
			if self._input:
				ch = self._input[0][:1]
				self._input[0] = self._input[0][1:]
				if not self._input[0]:
					self._input = self._input[1:]
				self._size -= 1
				return ch
			return None
		elif self._mode == READ_LINE:
			if self._buffer:
				if self._buffer[-1] == '\n':
					text = self._buffer
					self._buffer = ''
					return text
			while self._input:
				text = self._input[0]
				pos = text.find('\n')
				if pos >= 0:
					self._buffer += text[:pos + 1]
					text = text[pos + 1:]
					if not text:
						self._input = self._input[1:]
					else:
						self._input[0] = text
					self._size -= pos + 1
					packet = self._buffer
					self._buffer = ''
					return packet
				self._buffer += text
				self._input = self._input[1:]
				self._size -= len(text)
			return None
		elif self._mode == READ_BLOCK:
			need = self._need
			if len(self._buffer) >= need:
				text = self._buffer[:need]
				self._buffer = self._buffer[need:]
				return text
			while self._input:
				newneed = need - len(self._buffer)
				text = self._input[0]
				if len(text) >= newneed:
					self._buffer += text[:newneed]
					text = text[newneed:]
					self._size -= newneed
					if not text:
						self._input = self._input[1:]
					else:
						self._input[0] = text
					packet = self._buffer
					self._buffer = ''
					return packet
				self._buffer += text
				self._input = self._input[1:]
				self._size -= len(text)
			return None
		return None

	# 设定下一次读取的模式，READ_BLOCK 类型需要给定长度
	def set (self, mode, size = -1):
		if mode == READ_CHAR:
			self._mode = READ_CHAR
		elif mode == READ_LINE:
			self._mode = READ_LINE
		elif mode == READ_BLOCK:
			self._mode = READ_BLOCK
			if size < 1: 
				raise ValueError('size must greater than zero')
			self._need = size
		else:
			raise ValueError('unknow mode')
		return 0


#----------------------------------------------------------------------
# redis protocol tokenizer
#----------------------------------------------------------------------
class tokenizer (object):

	def __init__ (self):
		self.reset()
	
	def reset (self):
		self._reader = reader()
		self._stack = []
		self._token = []
		self._state = 0
		self._need = 0
		self._data = ''
		self._error = ''

	# 收到数据输入分析器
	def feed (self, text):
		r = self._reader
		r.feed(text)
		while True:
			if self._state == 0:
				r.set(READ_LINE)
				p = r.read()
				if p == None:
					break
				c = p[:1]
				if c in ('+', '-'):
					self._stack.append((c, p[1:].rstrip('\r\n')))
				elif c in (':', '*'):
					x = p[1:].rstrip('\r\n')
					try: 
						x = int(x)
					except:
						self._error = 'error value %s'%repr(p)
					self._stack.append((c, x))
				elif c == '$':
					x = p[1:].rstrip('\r\n')
					try: 
						x = int(x)
					except:
						self._error = 'error value %s'%repr(p)
						break
					if x < 0:
						self._stack.append((c, None))
					elif x == 0:
						self._data = ''
						self._state = 2
					else:
						self._data = ''
						self._state = 1
						self._need = x
			elif self._state == 1:
				r.set(READ_BLOCK, self._need)
				p = r.read()
				if p == None:
					break
				self._data = p
				self._state = 2
			elif self._state == 2:
				r.set(READ_LINE)
				p = r.read()
				if p == None:
					break
				self._stack.append(('$', self._data))
				self._data = ''
				self._state = 0
			else:
				raise ValueError('unknow state')
		while self._stack:
			self.__position = 0
			hr = self.__parse_token()
			if hr == None:
				break
			self._token.append(hr)
			self._stack = self._stack[self.__position:]
		return 0
	
	def __next_token (self):
		if self.__position >= len(self._stack):
			return None
		token = self._stack[self.__position]
		self.__position += 1
		return token
	
	def __parse_token (self):
		token = self.__next_token()
		if token == None:
			return None
		cmd = token[0]
		if cmd in ('+', ':', '$', '-'):
			return (token[0], token[1])
		elif cmd == '*':
			num = token[1]
			if num < 0:
				return ('*', None)
			elif num == 0:
				return ('*', [])
			else:
				data = []
				for i in xrange(num):
					p = self.__parse_token()
					if p == None:
						return None
					data.append(p)
				return ('*', data)
		return None
	
	# 递归序列化
	def __serialize (self, fp, obj):
		t = type(obj)
		if t in (type(0), type(0L)):
			obj = str(obj)
			fp.write('$%d\r\n%s\r\n'%(len(obj), obj))
			return
		if t == type(''):
			fp.write('$%d\r\n%s\r\n'%(len(obj), obj))
			return
		if t == type(u''):
			obj = obj.encode('utf-8')
			fp.write('$%d\r\n%s\r\n'%(len(obj), obj))
			return
		if t == type([]):
			fp.write('*%d\r\n'%len(obj))
			for n in obj:
				self.__serialize(fp, n)
			return
		if obj == None:
			fp.write('$-1\r\n')
			return
		return

	# 序列化：将准备发到redis的东西序列化为字符串
	def serialize (self, obj):
		import cStringIO
		sio = cStringIO.StringIO()
		self.__serialize(sio, obj)
		hr = sio.getvalue()
		sio = None
		return hr
	
	# 读取一个结果
	def next (self):
		if not self._token:
			return None
		p = self._token[1]
		self._token = self._token[1:]
		return p
		
	# 翻译 redis返回值为 python数据类型
	def translate (self, token):
		if token == None:
			return None
		cmd = token[0]
		val = token[1]
		if cmd in ('+', '$', ':'):
			return val
		if cmd == '-':
			return RedisError(val)
		if cmd == '*':
			if val == None:
				return None
			return [ self.translate(n) for n in val ]
		return None
	
	# 放弃结果缓存
	def clear (self):
		self._token = []
	
	def __iter__ (self):
		return self._token.__iter__()

	def __len__ (self):
		return len(self._token)
	
	def __getitem__ (self, index):
		return self._token[index]


#----------------------------------------------------------------------
# async socket
#----------------------------------------------------------------------
class asyncsock (object):

	def __init__ (self):
		self.sock = None		# socket object
		self.send_buf = []		# send buffer
		self.recv_buf = []		# recv buffer
		self.pend_buf = ''		# send pending
		self.state = NET_STATE_CLOSED
		self.errd = [ errno.EINPROGRESS, errno.EALREADY, errno.EWOULDBLOCK ]
		self.conn = ( errno.EISCONN, 10057, 10053 )
		self.errc = 0
		self.ipv6 = False
		self.eintr = ()
		if 'EINTR' in errno.__dict__:
			self.eintr = (errno.__dict__['EINTR'],)
		if 'WSAEWOULDBLOCK' in errno.__dict__:
			self.errd.append(errno.WSAEWOULDBLOCK)
		self.errd = tuple(self.errd)
		self.timeout = 0
		self.timecon = 0
	
	def __try_connect(self):
		if (self.state == NET_STATE_ESTAB):
			return 1
		if (self.state != NET_STATE_CONNECTING):
			return -1
		try:
			self.sock.recv(0)
		except socket.error, e:
			if e.errno in self.conn:
				return 0
			if e.errno in self.errd:
				self.state = NET_STATE_ESTAB
				self.recv_buf = ''
				return 1
			self.close()
			return -1
		self.state = NET_STATE_ESTAB
		return 1

	# try to receive all the data into recv_buf
	def __try_recv(self):
		size = 0
		while 1:
			text = ''
			try:
				text = self.sock.recv(4096)
				if not text:
					self.errc = 10000
					self.close()
					return -1
			except socket.error, e:
				if not e.errno in self.errd:
					#sys.stderr.write('[TRYRECV] '+strerror+'\n')
					self.errc = e.errno
					self.close()
					return -1
			if text == '':
				break
			self.recv_buf.append(text)
			size += len(text)
		return size

	# send data from send_buf until block (reached system buffer limit)
	def __try_send(self):
		if self.send_buf:
			self.pend_buf += ''.join(self.send_buf)
			self.send_buf = []
		wsize = 0
		if len(self.pend_buf) == 0:
			return 0
		try:
			wsize = self.sock.send(self.pend_buf)
		except socket.error, e:
			if not e.errno in self.errd:
				#sys.stderr.write('[TRYSEND] '+strerror+'\n')
				self.errc = e.errno
				self.close()
				return -1
		self.pend_buf = self.pend_buf[wsize:]
		return wsize

	# connect the remote server
	def connect(self, address, port, timeout = 0):
		self.close()
		af = socket.AF_INET
		if ':' in address:
			if not 'AF_INET6' in socket.__dict__:
				return -1
			if not socket.has_ipv6:
				return -2
			af = socket.AF_INET6
			self.ipv6 = True
		self.sock = socket.socket(af, socket.SOCK_STREAM)
		to = self.sock.gettimeout()
		self.sock.setblocking(0)
		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
		self.state = NET_STATE_CONNECTING
		try:
			hr = self.sock.connect_ex((address, port))
		except socket.error, e:
			pass
		self.send_buf = []
		self.recv_buf = []
		self.pend_buf = ''
		self.errc = 0
		self.timecon = time.time()
		self.timeout = timeout
		return 0

	# close connection
	def close(self):
		self.state = NET_STATE_CLOSED
		if not self.sock:
			return 0
		try:
			self.sock.close()
		except:
			pass
		self.sock = None
		self.ipv6 = False
		return 0

	# update 
	def update(self):
		if self.state == NET_STATE_CLOSED:
			return 0
		if self.state == NET_STATE_CONNECTING:
			self.__try_connect()
			if self.state == NET_STATE_CONNECTING and self.timeout > 0:
				if time.time() > self.timecon + self.timeout:
					self.close()
					return 0
		if self.state == NET_STATE_ESTAB:
			self.__try_recv()
		if self.state == NET_STATE_ESTAB:
			self.__try_send()
		return 0

	# return state
	def status(self):
		return self.state
	
	# get errno
	def error(self):
		return self.errc

	# recv
	def recv (self):
		if not self.recv_buf:
			return None
		r = self.recv_buf[0]
		self.recv_buf = self.recv_buf[1:]
		return r
	
	# send 
	def send (self, text, flush = False):
		self.send_buf.append(text)
		if flush:
			self.flush()
		return 0
	
	# flush
	def flush (self):
		if self.state == NET_STATE_ESTAB:
			self.__try_send()
		return 0

	# discard
	def clear (self):
		self.recv_buf = []
	
	def __iter__ (self):
		return self.recv_buf.__iter__()

	def __len__ (self):
		return len(self.recv_buf)
	
	def __getitem__ (self, index):
		return self.recv_buf[index]



#----------------------------------------------------------------------
# async redis
#----------------------------------------------------------------------
class AsyncRedis (object):

	def __init__ (self):
		self.__tokenizer = tokenizer()
		self.__network = asyncsock()
		self.__state = NET_STATE_CLOSED
		self.__cb_estab = None
		self.__cb_close = None
		self.__cb_reply = None
		self.__cb_start = None
		self.__cb_reply = None
		self.__cb_estab = None
		self.__cb_close = None
		self.__cb_start = None
		self.__reconnect_time = 20
		self.__reconnect_ts = -1
		self.host = ''
		self.port = 0
	
	# 删除自己
	def __del__ (self):
		try:
			self.close()
		except:
			pass
		self.__cb_estab = None
		self.__cb_close = None
		self.__cb_reply = None
		self.__cb_start = None
		return 0
	
	# 设置四种回调: 收到数据、连接建立、连接断开
	# on_reply(self, obj) - 收到完整 redis返回时调用，参数转为 Python类型，错误为 RedisError
	# on_start(self)      - 每次调用完 connect以后发生（包括自动重连），用于初始化
	# on_estab(self)      - 连接建立时调用
	# on_close(self)      - 连接断开时调用
	def callback (self, on_reply, on_start = None, on_estab = None, on_close = None):
		self.__cb_reply = on_reply
		self.__cb_estab = on_estab
		self.__cb_close = on_close
		self.__cb_start = on_start
		return 0

	# 连接远端 redis
	def connect (self, host, port = 6379, timeout = 15, reconn = 10):
		if (not host) or (not port):
			return 1
		self.__tokenizer.reset()
		hr = self.__network.connect(host, port, timeout)
		self.__state = self.__network.state
		if self.__cb_start:
			self.__cb_start(self)
		self.__reconnect_time = reconn
		self.__reconnect_ts = -1
		self.__connect_host = host
		self.__connect_port = port
		self.__connect_timeout = timeout
		self.host = host
		self.port = port
		return hr

	# 处理网络事件并触发回调，放到 Timer里面执行：推荐200ms一次
	def update (self):
		# 网络收发
		self.__network.update()
		# 更新状态
		newstate = self.__network.state
		oldstate = self.__state
		if oldstate != NET_STATE_ESTAB and newstate == NET_STATE_ESTAB:
			self.__state = NET_STATE_ESTAB
			if self.__cb_estab:
				self.__cb_estab(self)
			self.__reconnect_ts = -1
		# 处理网络包
		packets = self.__network.recv_buf
		self.__network.clear()
		if self.__cb_reply == None:
			packets = None
		else:
			for packet in packets:
				self.__tokenizer.feed(packet)
			for t in self.__tokenizer:
				self.__cb_reply(self, self.__tokenizer.translate(t))
			self.__tokenizer.clear()
		# 更新状态
		oldstate = self.__state
		if oldstate != NET_STATE_CONNECTING and newstate == NET_STATE_CONNECTING:
			self.__state = NET_STATE_CONNECTING
		oldstate = self.__state
		if oldstate != NET_STATE_CLOSED and newstate == NET_STATE_CLOSED:
			self.__state = NET_STATE_CLOSED
			if self.__cb_close:
				self.__cb_close(self)
		# 计算重连
		if self.__state == NET_STATE_CLOSED:
			if self.__reconnect_ts < 0 and self.__reconnect_time > 0:
				self.__reconnect_ts = time.time() + self.__reconnect_time
			elif self.__reconnect_ts > 0:
				current = time.time()
				if current >= self.__reconnect_ts:
					self.connect(self.__connect_host, self.__connect_port, \
						self.__connect_timeout, self.__reconnect_time)
		return 0
	
	# 发送命令
	def send (self, obj, flush = False):
		text = self.__tokenizer.serialize(obj)
		self.__network.send(text, flush)
	
	# 断开连接
	def close (self):
		self.__network.update()
		self.__network.close()
		if self.__state != NET_STATE_CLOSED:
			self.__state = NET_STATE_CLOSED
			if self.__cb_close:
				self.__cb_close(self)
		self.__reconnect_time = -1
		return 0
	
	# 刷新缓存
	def flush (self):
		self.__network.flush()

	# 取得状态
	def state (self):
		return self.__state
	
	# 是否活动
	def alive (self):
		return (self.__state == NET_STATE_ESTAB)


#----------------------------------------------------------------------
# 双连接到 Redis一条用于控制，一条用于订阅
#----------------------------------------------------------------------
class RedisClient (object):

	def __init__ (self, heartbeat = 5):
		self.__redis1 = AsyncRedis()
		self.__redis2 = AsyncRedis()
		self.__redis1.callback(self.__normal_reply, self.__normal_start)
		self.__redis2.callback(self.__subscribe_reply, self.__subscribe_start)
		self.__on_reply = None
		self.__on_subscribe = None
		self.__on_timer = None
		self.__heartbeat = heartbeat
		self.__info = []
		self.__info_tuple = ()
		self.__version = ''
		self.__heartbeat_ts1 = time.time() + self.__heartbeat
		self.__heartbeat_ts2 = time.time() + self.__heartbeat
		self.__timer_ts = time.time() + random.randint(1000, 3000) * 0.001
		self.__cache = []
		self.__cache_limit = 1000
		self.__host = None
		self.__port = 6379
		self.__auth = None
		self.__subscribe = {}
		self.heartbeat = False
	
	def __del__ (self):
		try:
			self.close()
		except:
			pass
		self.__redis1 = None
		self.__redis2 = None
		self.__on_reply = None
		self.__on_subscribe = None
		self.__on_timer = None
		return 0
	
	def callback (self, reply, subscribe, timer):
		self.__on_reply = reply
		self.__on_subscribe = subscribe
		self.__on_timer = timer
		return 0

	def __normal_reply (self, obj, data):
		if self.__on_reply:
			self.__on_reply(self, data)
		return 0
	
	def __normal_start (self, obj):
		if self.__auth != None:
			self.__redis1.send(['auth', self.__auth])
		for n in self.__cache:
			self.__redis1.send(n)
		self.__cache = []
		return 0
	
	def __parse_info (self, text):
		data = {}
		part = []
		for n in text.split('\n'):
			n = n.strip('\r\n\t ')
			p = n.find(':')
			if p < 0: continue
			name = n[:p]
			value = n[p + 1:]
			data[name] = value
			part.append((name, value))
		return data, tuple(part)
	
	def __subscribe_reply (self, obj, data):
		if type(data) != list:
			return -1
		if not data:
			return -1
		cmd = data[0]
		if cmd == 'message' and len(data) >= 3:
			if self.__on_subscribe:
				self.__on_subscribe(self, data[1], data[2])
		return -1
	
	def __subscribe_start (self, obj):
		if self.__auth != None:
			self.__redis2.send(['auth', self.__auth])
		count = 0
		for n in self.__subscribe:
			self.__redis2.send(['subscribe', n])
			count += 1
			if count >= 100:
				self.__redis2.flush()
				count = 0
		return 0
	
	def connect (self, host, port = 6379, auth = None, timeout = 15):
		self.__redis1.connect(host, port, timeout, 10)
		self.__redis2.connect(host, port, timeout, 10)
		self.__cache = []
		self.__info = []
		self.__host = host
		self.__port = port
		self.__auth = auth
		self.__timeout = timeout
		self.__version = ''
		return 0
	
	def close (self):
		self.__redis1.close()
		self.__redis2.close()
		return 0
	
	def update (self, current = None):
		if current == None:
			current = time.time()
		if self.__heartbeat > 0:
			r2 = random.randint(0, 20)
			if current >= self.__heartbeat_ts1:
				rnd = random.randint(0, 5)
				self.__heartbeat_ts1 = current + self.__heartbeat + rnd
				#if self.__redis1.alive():
				#	self.__redis1.send(['ping'])	
				self.heartbeat = True
			if current >= self.__heartbeat_ts2:
				rnd = random.randint(0, 5)
				self.__heartbeat_ts2 = current + self.__heartbeat + rnd
				if self.__redis2.alive():
					self.__redis2.send(['ping'])
		if current >= self.__timer_ts:
			self.__timer_ts = current + 2 + random.randint(0, 1000) * 0.001
			if self.__on_timer and self.__redis1.alive():
				self.__on_timer(self)
		self.__redis1.update()
		self.__redis2.update()
		return 0

	def send (self, data):
		self.__redis1.send(data)
		return 0
	
	def info (self):
		result = {}
		for n in self.__info:
			result[n] = self.__info[n]
		return result
	
	def version (self):
		return self.__version
	
	def alive (self):
		return self.__redis1.alive()

	def sub_add (self, key):
		key = str(key)
		self.__redis2.send(['subscribe', key])
		self.__subscribe[key] = 1
	
	def sub_del (self, key):
		key = str(key)
		self.__redis2.send(['unsubscribe', key])
		if key in self.__subscribe:
			del self.__subscribe[key]
		return 0
	
	def sub_chk (self, key):
		key = str(key)
		return self.__subscribe.__contains__ (key)
	
	def sub_clear (self):
		count = 0
		for n in self.__subscribe:
			self.__redis2.send(['unsubscribe', n])
			count += 1
			if count >= 100:
				count = 0
				self.__redis2.flush()
		self.__subscribe.clear()
		return 0
	
	def __contains__ (self, key):
		key = str(key)
		return self.__subscribe.__contains__ (key)
	
	def __iter__ (self):
		return self.__subscribe.__iter__ ()



#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':

	def test1():
		t = tokenizer()
		x = '*3\r\n$3\r\nSET\r\n$1\r\nX\r\n$10\r\n012\x004\r\n789\r\n+PING\r\n'
		y = '*4\r\n$3\r\nHAH\r\n*2\r\n$4\r\nFUCK\r\n$4\r\nSUCK\r\n$5\r\nHELLO\r\n*0\r\n'
		t.feed(x)
		t.feed(y[:42])
		t.feed(y[42:])
		x = t.serialize(['get', 'x', 10, None])
		t.feed(x)
		for n in t._token:
			print t.translate(n)
		print type(t)
		e = RedisError('fuck')
		print repr(e)

	def test2():
		def on_start(r):
			print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'on_start'
			r.send(['set', 'x', 1024])
			r.send(['get', 'x'])
			r.send(['get', 'y'])
			r.send(['fuck', 'x'])
			r.send(['hset', 'uid:10', 'eid1', 1])
			r.send(['hset', 'uid:10', 'eid2', 2])
			r.send(['hset', 'uid:10', 'eid3', 3])
			r.send(['hgetall', 'uid:10'])
			r.send(['hgetall', 'uid:12'])
			r.send(['multi'])
			r.send(['echo', 'transaction.ident'])
			r.send(['echo', 'context:12345'])
			r.send(['hget', 'uid:10', 'eid1'])
			r.send(['exec'])
		def on_estab(r):
			print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'on_estab'
		def on_close(r):
			print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'on_close'
		def on_reply(r, p):
			print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'on_reply(%s)'%repr(p)
		c = AsyncRedis()
		c.callback(on_reply, on_start, on_estab, on_close)
		#c.connect('192.168.1.3')
		c.connect('xnode2.ddns.net')
		#c.connect('192.168.0.21')
		#c.close()
		#c.send(['eval', "return {1,2,{3,4},5,{6,7},'haha'}", 0])
		c.send(['get', 'x'])
		while 1:
			time.sleep(0.1)
			c.update()
		return 0

	def test3():
		def on_reply (r, p):
			print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'on_reply(%s)'%repr(p)
		def on_subscribe (r, k, p):
			print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'on_subscribe(%s,%s)'%(repr(k), repr(p))
		def on_timer (r):
			r.send(['publish', '1', 'fuck:%d'%r.count])
			r.send(['publish', '2', 'fuck2'])
			r.send(['publish', '3', 'fuck3'])
			r.count += 1
		rc = RedisClient()
		rc.callback(on_reply, on_subscribe, on_timer)
		rc.connect('192.168.1.3', 6379)
		rc.sub_add(1)
		rc.sub_add(2)
		rc.sub_add(3)
		rc.count = 0
		print time.strftime('[%Y-%m-%d %H:%M:%S]'), 'start'
		while 1:
			time.sleep(0.1)
			rc.update()
		return 0

	test2()

