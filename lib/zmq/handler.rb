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

class ZMQ::Handler

  # The ZMQ::Socket instance wrapped by this handler.
  attr_reader :socket

  # A ZMQ::Socket instance is compulsary on init, with support for optional arguments if a subclasses do require them.
  #
  # pub = ctx.bind(:PUB, "tcp://127.0.0.1:5000") # lower level API
  # pub.handler = ZMQ::Handler.new(pub)
  #
  # class ProducerHandler < ZMQ::Handler
  #   def initialize(socket, producer)
  #     super
  #     @producer = producer
  #   end
  #
  #   def on_writable
  #     @producer.work
  #   end
  # end
  #
  # ZMQ::Loop.bind(:PUB, "tcp://127.0.0.1:5000", ProducerHandler, producer) # higher level API
  #
  def initialize(socket, *args)
    raise TypeError.new("#{socket.inspect} is not a valid ZMQ::Socket instance") unless ZMQ::Socket === socket
    @socket = socket
  end

  # Callback invoked from ZMQ::Loop handlers when the socket is ready for reading. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to read in a non-blocking manner
  # from within this callback.
  #
  # def on_readable
  #   msgs << socket.recv_nonblock
  # end
  #
  def on_readable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_readable contract"
  end

  # Callback invoked from ZMQ::Loop handlers when the socket is ready for writing. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to write data out as fast as possible
  # from within this callback.
  #
  # def on_writable
  #   socket.send buffer.shift
  # end
  #
  def on_writable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_writable contract"
  end

  # Callback for error conditions such as socket errors on poll and exceptions raised in callbacks. Receives an exception
  # instance as argument and raises by default.
  #
  # handler.on_error(err)  =>  raise
  #
  def on_error(exception)
    raise exception
  end
end