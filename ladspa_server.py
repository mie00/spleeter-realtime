#!/usr/bin/env python
# coding: utf8

"""
    Ladspa server implementatoin

    USAGE: import ladspa_server
"""

import socketserver
import threading

import numpy as np

__email__ = 'mohamed@elawadi.net'
__author__ = 'Mohamed Elawadi'
__license__ = 'MIT License'

class LADSPA_TCPServer(socketserver.BaseRequestHandler):
    def process(self, channel, sample_rate, sound):
        """Process one chunk of data coming from channel

        Parameters
        ----------
        channel : int
            Channel Number from 0 to `k`.
        sample_rate : int
            Sample rate (ex. `44100`).
        sound: numpy array of shape (n,) and type float32
            Input sound

        Returns
        ------
        numpy array of shape (n,) and type float32
            The processed sound
        """
        raise NotImplementedError("process should be implemented")

    def handle(self):
        print("got a new request")
        while True:
            longdt = np.dtype('int64').newbyteorder('<')
            floatdt = np.dtype('float32').newbyteorder('<')
            channel_str = self.request.recv(8)
            if len(channel_str) == 0:
                print("request over")
                return
            channel = np.frombuffer(channel_str, dtype=longdt)[0]
            sample_rate = np.frombuffer(self.request.recv(8), dtype=longdt)[0]
            size = np.frombuffer(self.request.recv(8), dtype=longdt)[0]
            received = self.request.recv(size * 4)
            while len(received) < size * 4:
                received += self.request.recv(size * 4 - len(received))
            data = np.frombuffer(received, dtype=floatdt)
            ret = self.process(channel, sample_rate, data)
            if ret.shape != data.shape:
                print("the shape of the returned sound: %s doesn't match the shape of the received one: %s", ret.shape, data.shape)
                ret = data
            if ret.dtype != 'float32':
                print("the datatype of the returned data `%s` is not float32", ret.dtype)
                ret = data
            self.request.sendall(ret.tobytes())

    @classmethod
    def serve_forever(cls, port, host="127.0.0.1"):
        tcp_server = socketserver.ThreadingTCPServer((host, port), cls)
        tcp_server.serve_forever()


if __name__ == "__main__":
    class Demo_TCPServer(LADSPA_TCPServer):
        def process(self, channel, sample_rate, sound):
            return sound

    print("listening on 8083")
    Demo_TCPServer.serve_forever(8083)