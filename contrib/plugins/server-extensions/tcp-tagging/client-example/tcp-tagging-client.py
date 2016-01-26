#!/usr/bin/python3

# Example of tcp tagging client

import sys
import socket
from time import time, sleep

# host and port of tcp tagging server
HOST = '127.0.0.1'
PORT = 15361

# Event identifier
EVENT_ID = 5+0x8100

# Artificial delay (ms). It may need to be increased if the time to send the tag is too long and causes tag loss.
DELAY=0

# transform a value into an array of byte values in little-endian order.
def to_byte(value, length):
    for x in range(length):
        yield value%256
        value//=256

# connect 
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

for i in range(100):
    # create the three pieces of the tag, padding, event_id and timestamp
    padding=[0]*8
    event_id=list(to_byte(EVENT_ID, 8))

    # timestamp can be either the posix time in ms, or 0 to let the acquisition server timestamp the tag itself.
    timestamp=list(to_byte(int(time()*1000)+DELAY, 8))

    # send tag and sleep
    s.sendall(bytes(padding+event_id+timestamp))
    sleep(1)

s.close()

