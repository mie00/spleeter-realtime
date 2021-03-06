#!/usr/bin/env python
# coding: utf8

"""
    Entrypoint provider for tcp server.

    USAGE: python spleeter_realtime.py
"""

import signal
from spleeter.separator import Separator

from ladspa_server import LADSPA_TCPServer

import numpy as np
import os

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

sep = Separator(
    './2stem-finetune-realtime.json',
    MWF=False,
    stft_backend='tensorflow',
    multiprocess=False)

class Spleeter_Server(LADSPA_TCPServer):
    def process(self, channel, sample_rate, data):
        if np.max(data) == np.min(data) == 0:
            return data
        if should_process:
            processed = sep.separate(data.astype('float64').reshape((-1, 1)))
            return processed['vocals'].astype('float32')[:,0]
        else:
            return data


if __name__ == "__main__":
    signal.signal(signal.SIGUSR1, handler)
    
    print("warming up")
    sep.separate(np.zeros((1024, 2)))
    print("run kill -SIGUSR1 %d to toggle the service on/off"%os.getpid())
    print("serving on :18083")
    Spleeter_Server.serve_forever(18083)
