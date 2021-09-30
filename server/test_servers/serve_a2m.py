#!/usr/bin/python

import re
import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind('tcp://*:5566')

a2m_rex = re.compile('[^a-m]')

print('Serving a2m listening at tcp://*:5566\n')

print('Ready', end = ' ', flush = True)
while True:
	message = socket.recv()

	input = message.decode('utf-8')

	result = a2m_rex.sub('', input)

	socket.send_string(result)
	print(end = '.', flush = True)
