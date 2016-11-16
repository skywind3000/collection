import sys, os, time


#----------------------------------------------------------------------
# DOSBox
#----------------------------------------------------------------------
class DOSBox (object):

	def __init__ (self, path = '.'):
		self.dirhome = ''
		work = os.path.dirname(__file__)
		for dd in ['.', 'dosbox', path]:
			path = os.path.abspath(os.path.join(work, dd))
			if os.path.exists(os.path.join(path, 'dosbox.exe')):
				self.dirhome = path
				break
		if not self.dirhome:
			raise IOError('not find dos box')
		self.dosbox = self.pathshort(os.path.join(self.dirhome, 'dosbox.exe'))
		self.config = {}
		self.command = []
		self.default()

	def pathshort (self, path):
		if path == None:
			return None
		path = os.path.abspath(path)
		if sys.platform[:3] != 'win':
			return path
		kernel32 = None
		textdata = None
		GetShortPathName = None
		try:
			import ctypes
			kernel32 = ctypes.windll.LoadLibrary("kernel32.dll")
			textdata = ctypes.create_string_buffer('\000' * 1024)
			GetShortPathName = kernel32.GetShortPathNameA
			args = [ ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int ]
			GetShortPathName.argtypes = args
			GetShortPathName.restype = ctypes.c_uint32
		except: 
			pass
		if not GetShortPathName:
			return path
		retval = GetShortPathName(path, textdata, 1024)
		shortpath = textdata.value
		if retval <= 0:
			return ''
		return shortpath

	def default (self):
		self.config['fullscreen'] = 'false'
		self.config['fulldouble'] = 'false'
		self.config['fullresolution'] = 'original'
		self.config['windowresolution'] = 'original'
		self.config['memsize'] = 16
		self.config['cycles'] = 15000
		self.config['machine'] = 'svga_s3'
	
	def dump (self):
		import StringIO
		fp = StringIO.StringIO()
		fp.write('[sdl]\n')
		for n in ('fullscreen', 'fulldouble', 'fullresolution', 'windowresolution'):
			fp.write('%s=%s\n'%(n, self.config[n]))
		fp.write('output=opengl\nautolock=true\nsensitivity=100\nwaitonerror=true\n')
		fp.write('priority=higher,normal\nmapperfile=mapper-0.74.map\nusescancodes=true\n\n')
		fp.write('[dosbox]\n')
		fp.write('language=\n')
		fp.write('machine=%s\n'%self.config['machine'])
		fp.write('captures=capture\nmemsize=%s\n\n'%self.config['memsize'])
		fp.write('[render]\n')
		fp.write('frameskip=0\naspect=false\nscaler=normal2x\n\n')
		fp.write('[cpu]\n')
		fp.write('core=auto\ncputype=auto\n')
		fp.write('cycles=%s\n'%self.config['cycles'])
		fp.write('cycleup=10\ncycledown=20\n\n')
		fp.write('[mixer]\n')
		fp.write('nosound=false\nrate=44100\nblocksize=1024\nprebuffer=20\n\n')
		fp.write('[midi]\n')
		fp.write('mpu401=intelligent\nmididevice=default\nmidiconfig=\n\n')
		fp.write('[sblaster]\n')
		fp.write('sbtype=sb16\nsbbase=220\nirq=7\ndma=1\nhdma=5\nsbmixer=true\n')
		fp.write('oplmode=auto\noplemu=default\noplrate=44100\n\n')
		fp.write('[gus]\n')
		fp.write('gus=false\ngusrate=44100\ngusbase=240\ngusirq=5\ngusdma=3\n')
		fp.write('ultradir=C:\ULTRASND\n\n')
		fp.write('[speaker]\n')
		fp.write('pcspeaker=true\npcrate=44100\ntandy=auto\ntandyrate=44100\n')
		fp.write('disney=true\n\n')
		fp.write('[joystick]\n')
		fp.write('joysticktype=auto\ntimed=true\nautofire=false\nswap34=false\n')
		fp.write('buttonwrap=false\n\n')
		fp.write('[dos]\n')
		fp.write('xms=true\nems=true\numb=true\nkeyboardlayout=auto\n\n')
		fp.write('[serial]\n')
		fp.write('serial1=dummy\nserial2=dummy\nserial3=disabled\n')
		fp.write('serial4=disabled\n\n')
		fp.write('[ipx]\nipx=false\n\n\n')
		fp.write('[autoexec]\n\n')
		for cmd in self.command:
			fp.write('%s\n'%cmd)
		fp.write('\n')
		return fp.getvalue()

	def push (self, text):
		self.command.append(text)
	
	def mount (self, drive, path, opt = ''):
		path = self.pathshort(path)
		self.push('mount %s %s %s'%(drive, path, opt))

	def save (self, filename):
		text = self.dump()
		fp = open(filename, 'w')
		fp.write(text)
		fp.close()
		return text
	
	def run (self, filename):
		import subprocess
		filename = self.pathshort(filename)
		subprocess.Popen([self.dosbox, '-CONF', filename, '-NOCONSOLE'])


#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------
def main(args = None):
	if args == None:
		args = [ n for n in sys.argv ]
	args = [ n for n in args ]
	import optparse
	p = optparse.OptionParser('usage: %prog [options] to start dosbox')
	p.add_option('-e', '--exe', dest = 'exe', help = 'executable path')
	p.add_option('-f', '--fullscreen', dest = 'fullscreen', action = 'store_true', help = 'full screen')
	p.add_option('-c', '--cycles', dest = 'cycles', help = 'cpu cycles, default is 15000')
	p.add_option('-s', '--fullsize', dest = 'fullsize', help = 'full-screen resolution')
	p.add_option('-w', '--windowsize', dest = 'windowsize', help = 'window size')
	p.add_option('-m', '--memsize', dest = 'memsize', help = 'memory size in MB')
	p.add_option('-x', '--exit', dest = 'exit', action = 'store_true', help = 'exit after finish')
	p.add_option('-d', '--dosbox', dest = 'dosbox', help = 'dos box location')
	p.add_option('-p', '--prelaunch', dest = 'prelaunch', help = 'pre-launch bat file')
	p.add_option('-t', '--post', dest = 'postlaunch', help = 'post-launch bat file')
	p.add_option('-v', '--vga', dest = 'vga', help = 'vga mode: default is svga_s3')
	p.add_option('-b', '--verbose', dest = 'verbose', action = 'store_true', help = 'not show cmd')
	options, args = p.parse_args(args) 
	if not options.exe:
		print >>sys.stderr, 'No executable file name. Try --help for more information.'
		return 2
	#print 'exename', options.exe
	if not os.path.exists(options.exe):
		print >>sys.stderr, 'not find executable file %s, Try --help for more information.'%options.exe
		return 3
	exename = os.path.abspath(options.exe)
	exepath = os.path.dirname(exename)
	dosbox = options.dosbox and options.dosbox or ''
	DB = DOSBox(dosbox)
	exename = DB.pathshort(exename)
	exemain = os.path.split(exename)[-1]
	cfgname = os.path.splitext(exemain)[0] + '.dos'

	if options.fullscreen:
		DB.config['fullscreen'] = 'true'
	if options.cycles:
		DB.config['cycles'] = options.cycles
	if options.fullsize:
		DB.config['fullresolution'] = options.fullsize
	if options.windowsize:
		DB.config['windowresolution'] = options.windowsize
	if options.memsize:
		DB.config['memsize'] = options.memsize
	if options.vga:
		DB.config['machine'] = options.vga
	if options.verbose:
		DB.push('@echo off')

	DB.mount('c', exepath)
	
	if options.prelaunch:
		if os.path.exists(options.prelaunch):
			DB.push('REM pre-launch: ' + options.prelaunch)
			for line in open(options.prelaunch):
				line = line.rstrip('\r\n\t')
				if not line:
					continue
				DB.push(line)
			DB.push('')
			
	DB.push('REM execute: ' + options.exe)
	DB.push('C:')
	DB.push('CD \\')
	DB.push(exemain)
	
	if options.postlaunch:
		if os.path.exists(options.postlaunch):
			DB.push('REM post-launch: ' + options.postlaunch)
			for line in open(options.postlaunch):
				line = line.rstrip('\r\n\t')
				if not line:
					continue
				DB.push(line)
			DB.push('')

	if options.exit:
		DB.push('EXIT')

	os.chdir(exepath)
	print DB.save(cfgname)
	DB.run(cfgname)

	return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':

	def test1():
		db = DOSBox()
		db.mount('d', 'd:/games/tt/tt')
		db.push('d:')
		db.push('tt.exe')
		print db.dump()
		db.save('d:/temp/tt.conf')
		db.run('d:/temp/tt.conf')

	def test2():
		#main(['', '--help'])
		main(['', '--exe=d:/games/tt/tt/tt.exe', '-w', '1000x800', '-x', '--dosbox=d:\\games\\tt\\dosbox'])
		return 0

	#test2()
	main()


