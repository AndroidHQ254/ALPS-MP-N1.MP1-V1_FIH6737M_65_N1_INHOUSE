#!/usr/bin/env python
import os
import re
import signal
import socket
import stat
import sys

def check_port_open(ip, port):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		s.connect((ip, int(port)))
		s.shutdown(2)
		return True
	except:
		return False

def find_port_range(ip, port_start, port_end, port_count):
	pstring = 'com.android.jack.launcher.ServerLauncher'
	pattern = re.compile('\/\.jack-server_(\d+)\/launcher\.jar')
	for line in os.popen('ps ux | grep ' + pstring + ' | grep -v grep'):
		fields = line.split(None, 10)
		cmd = fields[-1]
		result = pattern.search(line)
		if result:
			port_found = int(result.group(1))
			return range(port_found, port_found + port_count)
	if port_end - port_start + 1 <= port_count:
		return []
	port_first = 0
	port_current = port_start
	while port_current % port_count != 0:
		port_current = port_current + 1
	while port_current <= port_end:
		if check_port_open(ip, port_current):
			port_first = 0
		else:
			if port_first == 0:
				port_first = port_current
			if port_current + 1 - port_first == port_count:
				#return ' '.join(map(str, range(port_first, port_current + 1)))
				return range(port_first, port_current + 1)
		port_current = port_current + 1
	return []

def get_jack_root():
	mtk_jack_base = os.getenv('MTK_JACK_BASE')
	#mtk_jack_home = os.getenv('HOME')
	mtk_jack_user = os.getenv('USER')
	if mtk_jack_base is None or mtk_jack_base == '':
		#if mtk_jack_home is not None or mtk_jack_home == '':
		#	return mtk_jack_home
		mtk_jack_base = '/tmp'
	if mtk_jack_user is None or mtk_jack_user == '':
		return mtk_jack_base
	else:
		return os.path.join(mtk_jack_base, 'mtk-jack-' + mtk_jack_user)

def usage(argv):
	print 'Usage:'
	print argv[0] + ' find_port log server_host port_start port_end port_count'
	print argv[0] + ' kill_port log server_host port_start port_end port_count'
	print argv[0] + ' update_client log server_host port_start port_end port_count'
	print 'Example:'
	print argv[0] + ' find_port out/mtk-jack-env.log 127.0.0.1 50000 60000 2'
	print argv[0] + ' kill_port out/mtk-jack-env.log 127.0.0.1 50000 60000 2'
	print argv[0] + ' update_client out/mtk-jack-env.log 127.0.0.1 50000 60000 2'
	sys.exit(1)

def main(argv):
	if len(argv) != 7:
		usage(argv)
	action = argv[1]
	env_file = argv[2]
	server_host = argv[3]
	port_start = argv[4]
	port_end = argv[5]
	port_count = argv[6]
	env_dict = {}
	if action == 'find_port':
		ports = find_port_range(server_host, int(port_start), int(port_end), int(port_count))
		if len(ports) != 2:
			print 'Fail to find ' + port_count + ' free ports from ' + port_start + ' to ' + port_end
			sys.exit(2)
		mtk_jack_root = get_jack_root()
		if not os.path.exists(mtk_jack_root):
			os.makedirs(mtk_jack_root)
		env_dict['SERVER_PORT_SERVICE'] = str(ports[0])
		env_dict['SERVER_PORT_ADMIN'] = str(ports[1])
		env_dict['JACK_HOME'] = os.path.join(mtk_jack_root, '.jack-server' + '_' + env_dict['SERVER_PORT_SERVICE'])
		env_dict['CLIENT_SETTING'] = os.path.join(mtk_jack_root, '.jack-settings' + '_' + env_dict['SERVER_PORT_SERVICE'])
		with open(env_file, 'w') as f:
			f.write('JACK_HOME=' + env_dict['JACK_HOME'] + '\n')
			f.write('CLIENT_SETTING=' + env_dict['CLIENT_SETTING'] + '\n')
			f.write('SERVER_PORT_SERVICE=' + env_dict['SERVER_PORT_SERVICE'] + '\n')
			f.write('SERVER_PORT_ADMIN=' + env_dict['SERVER_PORT_ADMIN'] + '\n')
		os.chmod(env_file, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
		print('Write ' + env_file + ' with JACK_HOME=' + env_dict['JACK_HOME'] + ' CLIENT_SETTING=' + env_dict['CLIENT_SETTING'])
	elif action == 'update_client':
		with open(env_file, 'r') as f:
			for line in f.readlines():
				items = line.strip().split('=', 1)
				env_dict[items[0]] = items[1]
		client_file = os.path.join(env_dict['JACK_HOME'], 'config.properties')
		with open(client_file, 'w') as ff:
			ff.write('jack.server.max-jars-size=104857600' + '\n')
			ff.write('jack.server.max-service=4' + '\n')
			ff.write('jack.server.service.port=' + env_dict['SERVER_PORT_SERVICE'] + '\n')
			ff.write('jack.server.max-service.by-mem=1\\=2147483648\\:2\\=3221225472\\:3\\=4294967296' + '\n')
			ff.write('jack.server.admin.port=' + env_dict['SERVER_PORT_ADMIN'] + '\n')
			ff.write('jack.server.config.version=2' + '\n')
			ff.write('jack.server.time-out=7200' + '\n')
		os.chmod(client_file, stat.S_IRUSR | stat.S_IWUSR)
		print('Update ' + client_file + ' with SERVER_PORT_SERVICE=' + env_dict['SERVER_PORT_SERVICE'] + ' SERVER_PORT_ADMIN=' + env_dict['SERVER_PORT_ADMIN'])
	elif action == 'kill_port':
		with open(env_file, 'r') as f:
			for line in f.readlines():
				items = line.strip().split('=', 1)
				env_dict[items[0]] = items[1]
		pstring = 'com.android.jack.launcher.ServerLauncher'
		for line in os.popen('ps ux | grep ' + pstring + ' | grep -v grep'):
			fields = line.split(None, 10)
			cmd = fields[-1]
			if '-cp ' + env_dict['JACK_HOME'] + '/launcher.jar' in cmd:
				pid = fields[1]
				print 'Kill ' + pid + ': ' + cmd.strip()
				os.kill(int(pid), signal.SIGKILL)
	else:
		usage(argv)
	sys.exit(0)

if __name__ == '__main__':
	main(sys.argv)
