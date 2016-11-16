#! /usr/bin/python
#======================================================================
#
# cavemake.py - cavemake routine
#
# HISTORY:
# May.8  2008 skywind  - create this file with some basic interface
# May.19 2008 skywind  - add file saving interface
# Apr.25 2008 skywind  - add updating iterator
# Oct.8  2008 skywind  - add faultage level
# Oct.9  2008 skywind  - upgrade point choosing method
#
#======================================================================

import sys, random

# to change this
randint = random.randint

def create_matrix(w = 8, h = 8, c = 0):
	m = [ [ c for j in xrange(w) ] for i in xrange(h) ]
	return m

def copy_matrix(m):
	return [ [ n for n in line ] for line in m ]

def matrix_size(m):
	if len(m) == 0: return 0, 0
	return len(m[0]), len(m)

def copy_to(dst, src):
	w, h = matrix_size(src)
	for j in xrange(h):
		for i in xrange(w):
			dst[j][i] = src[j][i]

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
	print ''


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

lengthof = lambda p1, p2: ((p1[0] - p2[0]) ** 2) + ((p1[1] - p2[1]) ** 2)

INCX = (0, 1, 1, 1, 0, -1, -1, -1)
INCY = (-1, -1, 0, 1, 1, 1, 0, -1)

CPERMWALL = 100
CWALL = 110
CFLOOR = 200

class cavemake:
	def __init__(self, width = 32, height = 20, initial = 0.4):
		self.__width = width
		self.__height = height
		self.__initial = initial
		self.__map = [ [0 for i in xrange(width)] for j in xrange(height) ]
		self.__ds = disjointset()
		self.edges = []
		self.roomset = []

	def clear(self):
		m = self.__map
		for j in xrange(self.__height):
			for i in xrange(self.__width):
				m[j][i] = CWALL
		for j in xrange(self.__height):
			m[j][0] = CPERMWALL
			m[j][self.__width - 1] = CPERMWALL
		for i in xrange(self.__width):
			m[0][i] = CPERMWALL
			m[self.__height - 1][i] = CPERMWALL
		self.roomset = []
		self.edges = []
		self.__ds.clear()

	def print_cave(self):
		m = self.__map
		for j in xrange(self.__height):
			row = ''
			for n in m[j]:
				if n == CFLOOR: row += '.'
				elif n == CWALL or n == CPERMWALL: row += '#'
				elif (n >= 0) and (n <= 9): row += str(n)
				else: row += 'X'
				
			print row
		print
	
	def __getitem__ (self, row):
		if (row < 0) or (row >= self.__height):
			raise KeyError("row out of range")
		return self.__map[row]
	
	def __iter__ (self):
		return self.__map.__iter__()

	def dump(self):
		return self.__map

	def pass_ca(self, no_new_wall = False, keep = False, use_backup = False):
		m = self.__map
		incx = (0, 1, 1, 1, 0, -1, -1, -1)
		incy = (-1, -1, 0, 1, 1, 1, 0, -1)
		n = m
		if use_backup: 
			n = [ [ x for x in line ] for line in m ]
		neighbor = [ CFLOOR for x in xrange(8) ]
		for y in xrange(1, self.__height - 1):
			for x in xrange(1, self.__width - 1):
				wall_count = 0
				for d in xrange(8):
					neighbor[d] = n[y + incy[d]][x + incx[d]]
					if neighbor[d] != CFLOOR:
						wall_count += 1
				adjacence = 0
				for d in xrange(8):
					if neighbor[d] == CFLOOR: break
				if d < 8:
					for step in xrange(8):
						if neighbor[(d + step) & 7] != CFLOOR:
							break
						adjacence += 1
				canwall = False
				if (adjacence + wall_count == 8) or (not keep):
					canwall = True
				if (wall_count < 4) and (m[y][x] == CWALL):
					m[y][x] = CFLOOR
				elif (wall_count > 5) and (m[y][x] == CFLOOR):
					if (not no_new_wall) and canwall: 
						m[y][x] = CWALL

	def initialize(self):
		m = self.__map
		count = int(self.__width * self.__height * self.__initial)
		maxcount = self.__width * self.__height * 2
		self.clear()
		while count > 0:
			x = randint(1, self.__width - 2)
			y = randint(1, self.__height - 2)
			if m[y][x] == CWALL:
				m[y][x] = CFLOOR
				count -= 1
			maxcount -= 1
			if maxcount <= 0:
				break

	def __search_rooms(self):
		ds = self.__ds
		m = self.__map
		ds.clear()
		for y in xrange(1, self.__height - 1):
			for x in xrange(1, self.__width - 1):
				if m[y][x] != CFLOOR:
					continue
				root = ds[(y, x)]
				if m[y][x + 1] == CFLOOR:
					ds.union(root, (y, x + 1))
				if m[y + 1][x] == CFLOOR:
					ds.union(root, (y + 1, x))
				if m[y + 1][x + 1] == CFLOOR:
					if (m[y][x + 1] == CFLOOR) or (m[y + 1][x] == CFLOOR):
						ds.union(root, (y + 1, x + 1))
				if m[y + 1][x - 1] == CFLOOR:
					if (m[y][x - 1] == CFLOOR) or (m[y + 1][x] == CFLOOR):
						ds.union(root, (y + 1, x - 1))

	def __choose_dir(self, src, dest, noisy = True):
		if src[0] < dest[0]: incy = 1
		elif src[0] > dest[0]: incy = -1
		else: incy = 0
		if src[1] < dest[1]: incx = 1
		elif src[1] > dest[1]: incx = -1
		else: incx = 0
		if (noisy) and (incx != 0) and (incy != 0):
			n = randint(0, 1)
			if n == 0: incx = 0
			elif n == 1: incy = 0
		return incy, incx

	def __join_room(self, pt, destpt):
		ds = self.__ds
		m = self.__map
		pair = None
		while True:
			incy, incx = self.__choose_dir(pt, destpt, True)
			npt = (pt[0] + incy, pt[1] + incx)
			root = ds[pt]
			need_stop = False
			if npt == destpt: 
				need_stop = True
				pair = (root, ds[destpt])
			elif m[npt[0]][npt[1]] == CFLOOR:
				if ds[npt] != root:
					pair = (root, ds[npt])
					need_stop = True
			m[npt[0]][npt[1]] = CFLOOR
			ds.union(root, ds[npt])
			if randint(0, 1) == 0:
				r = randint(0, 7)
				noisy = pt[0] + INCY[r], pt[1] + INCX[r]
				if m[noisy[0]][noisy[1]] == CWALL:
					m[noisy[0]][noisy[1]] = CFLOOR
					ds.union(root, ds[noisy])
					pass
			pt = npt
			if need_stop: break
		return pair

	def __update_rooms(self):
		self.__search_rooms()
		sets = self.__ds.split()
		roomset = {}
		for n in sets:
			room = simplebunch(root = n, pts = sets[n], size=0)
			xmin, ymin = self.__width, self.__height
			xmax, ymax = 0, 0
			for y, x in sets[n]:
				if x < xmin: xmin = x
				if x > xmax: xmax = x
				if y < ymin: ymin = y
				if y > ymax: ymax = y
			cx = (xmin + xmax + 1) * 0.5
			cy = (ymin + ymax + 1) * 0.5
			best_dist = (self.__width + self.__height) ** 3
			best_pt = room.root
			for pt in sets[n]:
				dist = (cy - pt[0]) ** 2 + (cx - pt[1]) ** 2
				if dist < best_dist: 
					best_dist, best_pt = dist, pt
			room.center = best_pt
			room.size = len(room.pts)
			#print room.center, (cy, cx), room.root
			roomset[n] = room
		return roomset

	def __choose_entrance(self):
		roomset = self.__update_rooms()
		border = []
		for i in xrange(1, self.__width - 1):
			border.append((1, i))
			border.append((self.__height - 2, i))
		for i in xrange(1, self.__height - 1):
			border.append((i, 1))
			border.append((i, self.__width - 2))
		dists = []
		for pt in border:
			dist_sum = 0
			for key in roomset:
				room = roomset[key]
				dist_sum += lengthof(room.center, pt)
			dists.append((dist_sum, pt))
		dists.sort()
		dists.reverse()
		if len(dists) < 4: return dists[0][1]
		pos = len(dists)
		pos = randint(0, int(pos / 2))
		return dists[pos][1]

	def open_area(self, entrance):
		for incy in (-1, 0, 1):
			for incx in (-1, 0, 1):
				y = entrance[0] + incy
				x = entrance[1] + incx
				#if abs(incx) + abs(incy) >= 2:
				#	if randint(0, 1) == 0: continue
				if (y < 0) or (y >= self.__height): continue
				if (x < 0) or (x >= self.__width): continue
				self.__map[y][x] = CFLOOR

	def generate(self, printraw = False, FLOORLST = [], iter = 0):
		self.initialize()
		#self.print_cave()
		self.pass_ca(use_backup = False)
		#self.print_cave()

		entrance = self.__choose_entrance()
		entrance = self.__height - 2, int(self.__width * 3 / 4)
		self.entrance = entrance
		#self.__map[entrance[0]][entrance[1]] = CFLOOR

		try:
			for y, x in FLOORLST:
				self.__map[y][x] = CFLOOR
		except: pass

		if printraw:
			self.print_cave()

		m_original = copy_matrix(self.__map)
		m_result = copy_matrix(self.__map)

		count = 4;

		for ii in xrange(iter + 1):
			self.clear()
			copy_to(self.__map, m_original)
			self.roomset = self.__update_rooms()

			while True:
				roomset = self.__update_rooms()
				if len(roomset) <= 1: break
				roomlst = roomset.keys()
				pair_list = []
				for i in xrange(len(roomlst) - 1):
					rooma = roomset[roomlst[i]]
					for j in xrange(i + 1, len(roomlst)):
						roomb = roomset[roomlst[j]]
						dist = lengthof(rooma.center, roomb.center)
						pair_list.append((dist, (rooma, roomb)))
				pair_list.sort()
				limit = 500
				index = 0
				while True:
					if index >= len(pair_list) - 1: break
					if randint(0, 100) < limit: break
					limit += 10
				rooma, roomb = pair_list[0][1]
				#print rooma.root, roomb.root
				#pair = self.__join_room(rooma.root, roomb.root)
				point1 = rooma.pts[random.randint(0, rooma.size - 1)]
				point2 = roomb.pts[random.randint(0, roomb.size - 1)]
				pair = self.__join_room(point1, point2)
				self.edges.append(pair)
				#self.print_cave()

			#self.__open_entrance(entrance)
			#self.pass_ca(True)
			m = self.__map
			for j in xrange(self.__height):
				for i in xrange(self.__width):
					if m[j][i] == CFLOOR:
						m_result[j][i] = CFLOOR
			
		copy_to(self.__map, m_result)
		#self.pass_ca(False, True)
		self.pass_ca(True)
		
		return 0

	def faultage(self, level = 2, ratio = 0.3):
		m_original = copy_matrix(self.__map)
		level = level < 8 and level or 8
		for i in xrange(level):
			count = 0
			m = self.__map
			for line in m:
				for n in line:
					if n == CWALL: count += 1
			count = int(count * ratio)
			limit = self.__width * self.__height * 3
			while (limit > 0) and (count > 0):
				x = randint(0, self.__width - 1)
				y = randint(0, self.__height - 1)
				if m[y][x] in (CWALL, CPERMWALL):
					m[y][x] = CFLOOR
					count -= 1
				limit -= 1
			self.pass_ca(use_backup = False)
			#self.print_cave()
			for y in xrange(self.__height):
				for x in xrange(self.__width):
					if m_original[y][x] in (CWALL, CPERMWALL):
						m_original[y][x] = 1
					if m_original[y][x] >= 1 and m_original[y][x] <= 9:
						if m[y][x] in (CWALL, CPERMWALL): 
							m_original[y][x] += 1
					elif m_original[y][x] == CFLOOR:
						m_original[y][x] = 0
		copy_to(self.__map, m_original)
		m = self.__map
		incx = [ 0, 1, 1, 1, 0, -1, -1, -1 ]
		incy = [ -1, -1, 0, 1, 1, 1, 0, -1 ]
		incyx = zip(incy, incx)
		for j in xrange(0, self.__height):
			for i in xrange(0, self.__width):
				if m[j][i] in (0, 1): continue
				floors = 0
				for dy, dx in incyx:
					u, v = i + dx, j + dy
					if	u < 0 or u >= self.__width or \
						v < 0 or v >= self.__height: 
						floors += 1
						continue
					if m[v][u] == 0: floors += 1
				if floors > 0: m[j][i] = 1

	def test2(self):
		self.initialize()
		self.pass_ca(use_backup = True)
		self.print_cave()
		self.__update_rooms()
		self.__search_rooms()
		rooms = self.__ds.split()
		#for n in rooms: print n, self.__ds.weight(n)
		n = randint(0, len(rooms.keys()) - 1)
		center = rooms.keys()[n]
		print 'center', center
		for room in rooms:
			self.__join_room(rooms[room][0], center)
		for i in xrange(1):
			self.pass_ca(True, False)
		self.print_cave()


def cavesave(fp, width, height, ratio = 0.24, entrances = [], iter = 0, \
			fault = 0, fratio = 0.4):
	cm = cavemake(width, height, ratio)
	cm.generate(False, entrances, iter)
	if type(fp) == type(''):
		fp = file(fp, 'w')
	for e in entrances:
		cm.open_area(e)
	if fault > 0:
		fault = fault <= 8 and fault or 8
		cm.faultage(fault, fratio)
	for line in cm:
		row = ''
		for n in line:
			if n == CFLOOR or n == 0: row += '.'
			elif n == CWALL or n == CPERMWALL: row += '#'
			elif (n >= 0) and (n <= 9): row += str(n)
			else:
				row += '?'
		fp.write(row + '\n')
	fp.flush()
	fp.write('\n')
	roomset = cm.roomset
	for root in roomset:
		room = roomset[root]
		center = room.center
		fp.write('AREA %d %d\n'%(center[0], center[1]))
	for e in entrances:
		fp.write('ENTRANCE %d %d\n'%(e[0], e[1]))
	fp.flush()


try:
	import psyco
	psyco.bind(disjointset)
	psyco.bind(cavemake)
except ImportError:
	pass


if __name__ == '__main__':
	FILE = '-'
	WIDTH = 72
	HEIGHT = 21
	RATIO = 0.24
	FAULT = -1
	FRATIO = 0.4
	FLOORS = ''
	ENTRANCE = 0
	import time
	SEED = int(time.time())
	if len(sys.argv) > 1: FILE = sys.argv[1]
	if len(sys.argv) > 2: WIDTH = int(sys.argv[2])
	if len(sys.argv) > 3: HEIGHT = int(sys.argv[3])
	if len(sys.argv) > 4: RATIO = int(sys.argv[4]) * 0.01
	if len(sys.argv) > 5: FAULT = int(sys.argv[5])
	if len(sys.argv) > 6: FRATIO = int(sys.argv[6]) * 0.01
	if len(sys.argv) > 7: FLOORS = sys.argv[7]
	if len(sys.argv) > 8: ENTRANCE = int(sys.argv[8])
	if len(sys.argv) > 9: SEED = int(sys.argv[9])
	if FILE == '-': FILE = sys.stdout
	entrance = []
	for pos in FLOORS.split('-'):
		try: posy, posx = pos.split(':')
		except: continue
		posy, posx = int(posy), int(posx)
		entrance.append((posy, posx))
	#ENTRANCE = 8 | 4 | 2 | 1
	#ENTRANCE = 2
	#FAULT = 4
	if ENTRANCE & 2: entrance.append((HEIGHT - 2, int(WIDTH * 3 / 4)))
	if ENTRANCE & 8: entrance.append((1, int(WIDTH * 1 / 4)))
	if ENTRANCE & 1: entrance.append((int(HEIGHT * 1 / 4), WIDTH - 2))
	if ENTRANCE & 4: entrance.append((int(HEIGHT * 3 / 4), 1))

	import random
	random.seed(SEED)
	cavesave(FILE, WIDTH, HEIGHT, RATIO, entrance, 0, FAULT, FRATIO)


