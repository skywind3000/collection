import sys
import time
import socket
import os


#----------------------------------------------------------------------
# udp echo server
#----------------------------------------------------------------------
def udp_echo_server(ip, port):
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	if sys.platform[:3] != 'win':
		sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	BUF_SIZE = 8 * 1024 * 1024
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, BUF_SIZE) 
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, BUF_SIZE) 
	#print sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)  
	#print sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)  
	try:
		sock.bind((ip, port))
	except socket.error, e:
		sys.stderr.write('bind (%s, %d) failed: %s'%(ip, port, str(e)))
		sock.close()
		return -1
	print 'listening udp on ("%s", %d)'%(ip, port)
	while True:
		try:
			data, remote = sock.recvfrom(8192)
		except socket.error, e:
			continue
		try:
			sock.sendto(data, remote)
		except socket.error, e:
			pass
	return 0


#----------------------------------------------------------------------
# entry
#----------------------------------------------------------------------
def main(args = None):
	if args == None:
		args = [ n for n in sys.argv ]
	import optparse
	p = optparse.OptionParser('usage: %prog [options] to start cron')
	p.add_option('-p', '--port', dest = 'port', help = 'config port number')
	p.add_option('-i', '--ip', dest = 'ip', help = 'config ip address')
	options, args = p.parse_args(args) 
	if options.port is None:
		print >>sys.stderr, 'no port given, Try --help for more information.'
		return 1
	if options.ip is None:
		print >>sys.stderr, 'no ip given, Try --help for more information.'
		return 1
	udp_echo_server(options.ip, int(options.port))
	return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':
	#main(['', '--port=2015', '--ip=0.0.0.0'])
	main()

