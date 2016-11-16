#! /usr/bin/python

import sys, time, random, math
import cavemake
import sys, random

from cavemake import CFLOOR, CWALL, CPERMWALL


#----------------------------------------------------------------------
# math
#----------------------------------------------------------------------
def create_matrix(w = 8, h = 8, c = 0):
	m = [ [ c for j in xrange(w) ] for i in xrange(h) ]
	return m

def copy_matrix(m):
	return [ [ n for n in line ] for line in m ]

def matrix_size(m):
	if len(m) == 0: return 0, 0
	return len(m[0]), len(m)

def print_matrix(m, is_all_char = False):
	mlen = 0
	for line in m:
		for n in line:
			s = len(str(n))
			mlen = max(mlen, s)
	for line in m:
		text = ''
		for n in line:
			result = str(n).rjust(mlen) + ' '
			if is_all_char: result = str(n)[:1]
			text += result
		print text
	return 0

def drawchar(screen, x, y, c):
	w, h = matrix_size(screen)
	if (x >= 0) and (y >= 0) and (x < w) and (y < h):
		screen[y][x] = c

def drawtext(screen, x, y, text):
	for n in text:
		drawchar(screen, x, y, n)
		x += 1

def displayscr(screen):
	for line in screen:
		row = ''
		for c in line: row += c
		print row

def drawline(screen, x1, y1, x2, y2, c = '*'):
	if y1 == y2:
		if x1 > x2: x1, x2 = x2, x1
		while x1 <= x2:
			drawchar(screen, x1, y1, c)
			x1 += 1
		return
	if x1 == x2:
		if y1 > y2: y1, y2 = y2, y1
		while y1 <= y2:
			drawchar(screen, x1, y1, c)
			y1 += 1
		return
	if abs(x1 - x2) >= abs(y1 - y2):
		step = abs(x2 - x1) + 1
		incx = 1
		if x1 > x2: incx = -1
		incy = float(y2 - y1) / step
		x = float(x1)
		y = float(y1)
		for i in xrange(step):
			drawchar(screen, int(x), int(y), c)
			x += incx
			y += incy
	else:
		step = abs(y2 - y1) + 1
		incy = 1
		if y1 > y2: incy = -1
		incx = float(x2 - x1) / step
		x = float(x1)
		y = float(y1)
		for i in xrange(step):
			drawchar(screen, int(x), int(y), c)
			x += incx
			y += incy


#----------------------------------------------------------------------
# disjointset
#----------------------------------------------------------------------
class disjointset:
	def __init__(self):
		self.__father = {}
		self.__weight = {}
		self.__size = 0
	def __len__(self):
		return self.__size
	def find(self, x):
		if x == None:
			return None
		father = self.__father
		if x in father:
			root = x
			path = []
			while father[root] != None:
				path.append(root)
				root = father[root]
			for n in path:
				self.__father[n] = root
			return root
		self.__father[x] = None
		self.__weight[x] = 1
		self.__size += 1
		return x
	def __getitem__(self, key):
		return self.find(key)
	def weight(self, x):
		return self.__weight[self.find(x)]
	def clear(self):
		self.__father = {}
		self.__weight = {}
		self.__size = 0
	def union(self, x, y):
		root1 = self.find(x)
		root2 = self.find(y)
		if root1 != root2:
			if self.__weight[root1] < self.__weight[root2]:
				self.__weight[root2] += self.__weight[root1]
				del self.__weight[root1]
				self.__father[root1] = root2
			else:
				self.__weight[root1] += self.__weight[root2]
				del self.__weight[root2]
				self.__father[root2] = root1
	def split(self):
		roots = {}
		for n in self.__father:
			f = self.find(n)
			if f in roots: 
				roots[f].append(n)
			else:
				roots[f] = [n]
		return roots


class simplebunch:
	def __init__ (self, **kwds): self.__dict__ = kwds


class vector2d:
	def __init__ (self, x = 0, y = 0):
		self.x = x
		self.y = y
	def __add__ (self, p):
		if type(p) == type((0, 0)):
			return vector2d(self.x + p[0], self.y + p[1])
		return vector2d(self.x + p.x, self.y + p.y)
	def __sub__ (self, p):
		if type(p) == type((0, 0)):
			return vector2d(self.x - p[0], self.y - p[1])
		return vector2d(self.x - p.x, self.y - p.y)
	def __radd__ (self, p):
		return self.__add__(p)
	def __rsub__ (self, p):
		if type(p) == type((0, 0)):
			return vector2d(p[0] - self.x, p[1] - self.y)
		return vector2d(p.x - self.x, p.y - self.y)
	def __neg__ (self):
		return vector2d(-self.x, -self.y)
	def __repr__ (self):
		return 'vector2d(%s,%s)'%(self.x, self.y)
	def __getitem__ (self, key):
		if key == 0: return self.x
		if key == 1: return self.y
		raise KeyError('unknow key(%s) for vector2d'%key)
	def __setitem__ (self, key, val):
		if key == 0: self.x = val
		elif key == 1: self.y = val
		else: raise KeyError('unknow key(%s) for vector2d'%key)
	def __eq__ (self, p):
		if (self.x == p.x) and (self.y == p.y): 
			return True
		return False
	def __ne__ (self, p):
		return not self.__eq__(p)
	def length (self):
		import math
		return math.sqrt(self.x * self.x + self.y * self.y)
	def dot (self, p):
		if type(p) == type((0, 0)):
			return self.x * p[0] + self.y * p[1]
		return self.x * p.x + self.y * p.y
	def __mul__ (self, p):
		if type(p) == type((0, 0)):
			return self.x * p[1] - self.y * p[0]
		elif type(p) in (int, long, float):
			return vector2d(self.x * p, self.y * p)
		return self.x * p.y - self.y * p.x
	def __rmul__ (self, p):
		if type(p) == type((0, 0)):
			return p[0] * self.y - p[1] * self.x
		elif type(p) in (int, long, float):
			return vector2d(self.x * p, self.y * p)
		return p.x * self.y - p.y * self.x
	def __copy__ (self):
		return vector2d(self.x, self.y)
	def __deepcopy__ (self):
		return self.__copy__()


class line2d:
	def __init__ (self, p1 = (0, 0), p2 = (0, 0)):
		self.p1 = vector2d()
		self.p2 = vector2d()
		if type(p1) == type((0, 0)):
			self.p1.x, self.p1.y = p1[0], p1[1]
		else:
			self.p1.x, self.p1.y = p1.x, p1.y
		if type(p2) == type((0, 0)):
			self.p2.x, self.p2.y = p2[0], p2[1]
		else:
			self.p2.x, self.p2.y = p2.x, p2.y
	def length (self):
		p = self.p2 - self.p1
		return p.length()
	def __repr__ (self):
		p1, p2 = self.p1, self.p2
		return 'line(%s,%s,%s,%s)'%(p1.x, p1.y, p2.x, p2.y)
	def intersect (self, l):
		v1 = (self.p2.x - self.p1.x) * (l.p2.y - self.p1.y)
		v2 = (self.p2.x - self.p1.x) * (l.p1.y - self.p1.y)
		v1-= (self.p2.y - self.p1.y) * (l.p2.x - self.p1.x)
		v2-= (self.p2.y - self.p1.y) * (l.p1.x - self.p1.x)
		if (v1 * v2) >= 0:
			return False
		v3 = (l.p2.x - l.p1.x) * (self.p2.y - l.p1.y)
		v4 = (l.p2.x - l.p1.x) * (self.p1.y - l.p1.y)
		v3-= (l.p2.y - l.p1.y) * (self.p2.x - l.p1.x)
		v4-= (l.p2.y - l.p1.y) * (self.p1.x - l.p1.x)
		if (v3 * v4) >= 0:
			return False
		return True
	def online (self, p):
		if type(p) == type((0, 0)):
			p = vector2d(p[0], p[1])
		if (p - self.p1) * (self.p2 - self.p1) == 0:
			return True
		return False
	def onsegment (self, p):
		if type(p) == type((0, 0)):
			p = vector2d(p[0], p[1])
		if not self.online(p):
			return False
		if p.x < min(self.p1.x, self.p2.x):
			return False
		if p.y < min(self.p1.y, self.p2.y):
			return False
		if p.x > max(self.p1.x, self.p2.x):
			return False
		if p.y > max(self.p1.y, self.p2.y):
			return False
		return True


def savebmp(fname, screen):
	from struct import pack, unpack
	w, h = len(screen[0]), len(screen)
	depth = 24
	filler = 3 - ((w * h * int(depth / 8) - 1) & 3)
	imgsize = (w * 3 + filler) * h
	filesize = 54 + imgsize
	fp = file(fname, 'wb')
	fp.write(pack('<HLHHL', 0x4d42, filesize, 0, 0, 54))
	fp.write(pack('<LLLHHLLLL', 40, w, h, 1, 24, 0, imgsize, 0xb12, 0xb12))
	fp.write(pack('<LL', 0, 0))
	for y in xrange(h):
		line = screen[h - 1 - y]
		for c in line:
			r, g, b = (c & 0xff), ((c >> 8) & 0xff), ((c >> 16) & 0xff)
			fp.write(pack('<BBB', r, g, b))
		for i in xrange(filler):
			fp.write('\x00')
	fp.close()

def showbmp(fname):
	import os
	cmd = 'start %s'%fname
	os.system(cmd)

RGB = lambda r, g, b: ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)
mid = lambda a, b, c: max(min(b, c), a)


def speedup():
	try:
		import psyco
		psyco.bind(disjointset)
		psyco.bind(vector2d)
		psyco.bind(line2d)
		psyco.bind(savebmp)
	except ImportError:
		pass


def test_disjoint():
	ds = disjointset()
	for i in xrange(20):
		ds.find(i)
	ds.union(0, 1)
	ds.union(1, 3)
	ds.union(3, 10)
	ds.union(2, 8)
	ds.union(8, 9)
	ds.union(11, 2)
	ds.union(7, 6)
	ds.union(5, 7)
	ds.union(13, 14)
	ds.union(13, 15)
	ds.union(13, 18)
	ds.union(12, 19)
	ds.union(4, 17)
	ds.union(17, 16)
	ds.union(6, 16)
	rs = ds.split()
	for n in rs:
		print n, ':', rs[n], ds.weight(n)


if __name__ == '__main__':
	p1 = vector2d(1, 2)
	p2 = vector2d(13, 8)
	p1 = p2 + (0, 0)
	p2.x = 99
	print p1, p2
	p1 = p2
	p2.x = 33
	print p1, p2
	p2 += (1, 2)
	p1 = (1000, 1000) + p2
	print p1
	print p1.length()
	print (p1 * 0.1).length()
	l1 = line2d((0, 0), (640, 480))
	l2 = line2d((1, 0), (640, 0))
	print l1.intersect(l2)
	l1 = line2d((25, 15), (34, 12))
	l2 = line2d((47, 3), (49, 11))
	print l1.intersect(l2)
	l1 = line2d((0, 0), (10, 0))
	l2 = line2d((2, -1), (5, 1))
	print 'onsegment', l1.onsegment((11, 0))
	print 'intersect', l1.intersect(l2)


class population:
	def __init__ (self, width = 32, height = 32, seed = None):
		self.__cm = cavemake.cavemake(width, height, 0.24)
		self.__width = width
		self.__height = height
		self.__seed = seed
		self.__map = create_matrix(width, height)
		self.__entrances = []
		self.__items = []
		self.__roots = []
		self.__display = []
		self.__rootmap = {}
		self.roomset = {}
		self.seed(seed)
	
	def seed (self, seed = None):
		if seed == None:
			seed = int(time.time() * 1000)
		self.__seed = seed
		random.seed(seed)
	
	def entrance (self):
		if len(self.__entrances) == 0:
			self.__entrances.append((self.__height - 2, self.__width * 3 / 4))
		elif len(self.__entrances) == 1:
			self.__entrances.append((self.__width / 3, 1))
		elif len(self.__entrances) == 2:
			self.__entrances.append((self.__width - 2, self.__height * 2 / 3))
		elif len(self.__entrances) == 3:
			self.__entrances.append((1, self.__height / 3))

	def cavegen (self, ratio = 0.28):
		self.__cm = cavemake.cavemake(self.__width, self.__height, ratio)
		self.__ratio = ratio

		self.entrance()
		self.__cm.generate(False, self.__entrances)

		for entrance in self.__entrances:
			self.__cm.open_area(entrance)

		self.__map = copy_matrix(self.__cm.dump())
		self.roomset = self.__cm.roomset

		self.__roots = create_matrix(self.__width, self.__height, None)
		self.__rootmap = {}
		for root in self.roomset:
			room = self.roomset[root]
			for y, x in room.pts:
				self.__roots[y][x] = root
			self.__rootmap[root] = len(self.__roots)
			self.__roots.append(root)
			dist = cavemake.lengthof(entrance, room.center)
			room.dist = dist
		
		self.__graph = create_matrix(len(self.roomset), len(self.roomset))
		self.edges = []
		
		self.__display = copy_matrix(self.__map)
		for j in xrange(len(self.__map)):
			for i in xrange(len(self.__map[0])):
				if self.__map[j][i] == CFLOOR:
					self.__display[j][i] = '.'
				else:
					self.__display[j][i] = '#'
		
	def textroom (self, root, text):
		if not root in self.roomset:
			return
		room = self.roomset[root]
		center = room.center
		col = center[1] - len(text) / 2
		if col < 1: col = 1
		row = center[0]
		if row < 1: row = 1
		drawtext(self.__display, col, row, text)

	def generate (self, ratio = 0.3):
		#self.entrance()
		self.cavegen(ratio)
		rooms = []
		for root in self.roomset:
			room = self.roomset[root]
			rooms.append((room.dist, room.center, root))
		rooms.sort()
		rooms.reverse()
		self.items = []
		if len(rooms) >= 1:
			self.textroom(rooms[0][2], "BOSS")
			self.items.append((rooms[0][1], "BOSS"))
		IN = self.__entrances[0]
		drawtext(self.__display, IN[1] - 2, IN[0] + 1, " IN ")
		from random import randint

		for i in xrange(len(rooms)):
			pos = randint(0, len(rooms)) + 1
			if (pos < 1) or (pos >= len(rooms)): continue
			root = rooms[pos][2]
			mode = randint(0, 99)
			if mode < 40:
				self.textroom(root, " X ")
				self.items.append((rooms[pos][1], "X"))
			elif mode < 60:
				self.textroom(root, " X$ ")
				self.items.append((rooms[pos][1], "X$"))
			elif mode < 80:
				self.textroom(root, " $ ")
				self.items.append((rooms[pos][1], "$"))
		
		for root in self.roomset:
			room = self.roomset[root]
			pt = room.center
			drawtext(self.__display, pt[1], pt[0], "C")
		
	
	def display (self):
		for line in self.__display:
			row = ''
			for c in line:
				row += c
			print row
		print
			


if __name__ == '__main__':
	WIDTH = 32
	HEIGHT = 32
	RATIO = 0.24
	if len(sys.argv) > 1: WIDTH = int(sys.argv[1])
	if len(sys.argv) > 2: HEIGHT = int(sys.argv[2])
	if len(sys.argv) > 3: RATIO = int(sys.argv[3]) * 0.01
	pop = population(WIDTH, HEIGHT, RATIO)
	pop.seed()
	pop.generate()
	pop.display()
