#!/usr/bin/python

import re
import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind('tcp://*:5555')

alpha_rex = re.compile('[^a-zA-Z0-9]')

print('Serving alpha listening at tcp://*:5555\n')

print('Ready', end = ' ', flush = True)
while True:
	message = socket.recv()

	input = message.decode('utf-8')

	ii = input.split('\n')
	result = []

	for i in ii:
		result.append(alpha_rex.sub('', i))

	socket.send_string('\n'.join(result))
	print(end = '.', flush = True)
