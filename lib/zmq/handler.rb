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

  # The ZMQ::Socket or IO instance wrapped by this handler.
  attr_reader :pollable

  # A ZMQ::Socket or IO instance is compulsary on init, with support for optional arguments if a subclasses do require them.
  #
  # pub = ctx.bind(:PUB, "tcp://127.0.0.1:5000") # lower level API
  # item = ZMQ::Pollitem.new(pub)
  # item.handler = ZMQ::Handler.new(pub)
  #
  # class ProducerHandler < ZMQ::Handler
  #   def initialize(pollable, producer)
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
  def initialize(pollable, *args)
    # XXX: shouldn't leak into handlers
    unless ZMQ::Socket === pollable || IO === pollable
      raise TypeError.new("#{pollable.inspect} is not a valid ZMQ::Socket instance")
    end
    @pollable = pollable
  end

  # Callback invoked from ZMQ::Loop handlers when the pollable item is ready for reading. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to read in a non-blocking manner
  # from within this callback.
  #
  # def on_readable
  #   msgs << recv
  # end
  #
  def on_readable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_readable contract"
  end

  # Callback invoked from ZMQ::Loop handlers when the pollable item is ready for writing. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to write data out as fast as possible
  # from within this callback.
  #
  # def on_writable
  #   send buffer.shift
  # end
  #
  def on_writable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_writable contract"
  end

  # Callback for error conditions such as pollable item errors on poll and exceptions raised in callbacks. Receives an exception
  # instance as argument and raises by default.
  #
  # handler.on_error(err)  =>  raise
  #
  def on_error(exception)
    raise exception
  end

  # API that allows handlers to send data regardless of the underlying pollable item type (ZMQ::Socket or IO).
  # XXX: Expose Pollitem#send(data) instead ?
  #
  def send(data)
    case pollable
    when IO
      pollable.write_nonblock(data)
    when ZMQ::Socket
      pollable.send(data)
    end
  end

  # API that allows handlers to receive data regardless of the underlying pollable item type (ZMQ::Socket or IO).
  # XXX: Expose Pollitem#send(data) instead ?
  #
  def recv
    case pollable
    when IO
      # XXX assumed page size
      pollable.read_nonblock(4096)
    when ZMQ::Socket
      pollable.recv_nonblock
    end
  end
end