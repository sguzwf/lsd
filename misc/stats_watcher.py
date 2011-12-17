#!/usr/bin/env python

import zmq
from sys import argv
from pprint import pprint

def main():
    context = zmq.Context()

    request = context.socket(zmq.REQ)
    request.connect('tcp://localhost:3333')

    # Statistics
    # cache_stats / config / all_services
    request.send_json({
    	'version' : 1,
        'action': 'all_services'
    })

    pprint(request.recv_json())

if __name__ == "__main__":
    main()
