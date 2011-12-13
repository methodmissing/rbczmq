# encoding: utf-8

#--
#
# Author:: Lourens Naudé
# Homepage::  http://github.com/methodmissing/rbczmq
# Date:: 20111213
#
#----------------------------------------------------------------------------
#
# Copyright (C) 2011 by Lourens Naudé. All Rights Reserved.
# Email: lourens at methodmissing dot com
#
# (The MIT License)
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# 'Software'), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#---------------------------------------------------------------------------
#
#

class ZMQ::Context

  # Before using any ØMQ library functions the caller must initialise a ØMQ context.
  #
  # The context manages open sockets and automatically closes these before termination. Other responsibilities include :
  #
  # * A simple way to set the linger timeout on sockets
  # * C onfigure contexts for a number of I/O threads
  # * Sets-up signal (interrrupt) handling for the process.

  # Sugaring for spawning a new socket and bind to a given endpoint
  #
  # ctx.bind(:PUB, "tcp://127.0.0.1:5000")
  #
  def bind(sock_type, endpoint)
    s = socket(sock_type)
    s.bind(endpoint)
    s
  end

  # Sugaring for spawning a new socket and connect to a given endpoint
  #
  # ctx.connect(:SUB, "tcp://127.0.0.1:5000")
  #
  def connect(sock_type, endpoint)
    s = socket(sock_type)
    s.connect(endpoint)
    s
  end
end