#!/usr/bin/env python

import zmq
from sys import argv
from pprint import pprint

def main():
    context = zmq.Context()

    request = context.socket(zmq.REQ)
    request.connect('tcp://localhost:3333')

    # Statistics
    request.send_json({
    	'handle': 'all',
        'action': 'info'
    })

    pprint(request.recv_json())

if __name__ == "__main__":
    main()
