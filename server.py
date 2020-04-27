#!/usr/bin/env python
# coding: utf8

"""
    Entrypoint provider for tcp server.

    USAGE: python -m spleeter.server
"""

import signal
from spleeter.separator import Separator

from .ladspa_server import LADSPA_TCPServer

import numpy as np

__email__ = 'mohamed@elawadi.net'
__author__ = 'Mohamed Elawadi'
__license__ = 'MIT License'

should_process = True

def handler(signum, frame):
    global should_process
    should_process = not should_process
    if should_process:
        print("converting")
    else:
        print("not converting")


signal.signal(signal.SIGUSR1, handler)

sep = Separator(
    'spleeter:2stem-finetune-realtime',
    MWF=False,
    stft_backend='tensorflow',
    multiprocess=False)

class Spleeter_Server(LADSPA_TCPServer):
    def process(self, channel, sample_rate, data):
        if np.max(data) == np.min(data) == 0:
            return data
        if should_process:
            processed = sep.separate(data.astype('float64').reshape((-1, 1)))
            if channel == 0:
                return processed['vocals'].astype('float32')[:,0]
            else:
                return processed['vocals'].astype('float32')[:,0]
                return processed['accompaniment'].astype('float32')[:,0]
        else:
            return data


print("warming up")
sep.separate(np.zeros((1024, 2)))
print("serving on :8083")
Spleeter_Server.serve_forever(8083)
