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

$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'
require 'pp'

# PUSH / PULL topology that demonstrates work being distributed in round robin fashion to a pool of worker threads.
# This pattern is a good start where little operational complexity is crucial and can scale out to multiple processes
# and boxes by just swapping out the transport.

Thread.abort_on_exception = true

class Worker
  attr_reader :thread
  def initialize(ctx, endpoint, watermark = 1000)
    @thread = nil
    @connect = Proc.new do
      @socket = ctx.socket(:PULL)
      # verbose output
      @socket.verbose = true
      @socket.connect(endpoint)
      # Limit the amount of work queued for this worker. When the high water mark ceiling hits, a particular worker
      # is ignored during round robin distribution
      @socket.hwm = watermark
      @socket.linger = 0
    end
    @jobs, @working = 0, 0.0
  end

  def start
    @thread = Thread.new do
      @connect.call
      loop do
        break if Thread.current[:interrupted]
        perform(@socket.recv)
      end
    end
    self
  end

  def stop
    return unless @thread
    @thread[:interrupted] = true
    @thread.join(0.1)
    stats
  end

  def perform(work)
    # Random hot loop to simulate CPU intensive work
    start = Time.now
    work.to_i.times{}
    @jobs += 1
    @working += (Time.now - start).to_f
  end

  private
  def stats
    puts "Processed #{@jobs} jobs in %.4f seconds" % @working
    $stdout.flush
  end
end

class Master
  def initialize(ctx, endpoint)
    @ctx, @endpoint, @workers = ctx, endpoint, []
    @socket = ctx.socket(:PUSH)
    # verbose output
    @socket.verbose = true
    @socket.bind(endpoint)
    @socket.linger = 0
    @interrupted = false
  end

  def spawn_workers(count = 10)
    count.times do
      @workers << Worker.new(@ctx, @endpoint).start
      sleep 0.01 # give each thread time to spin up
    end
  end

  def start(messages = 100)
    messages.to_i.times do
      # Tasks are hot loops with random 0 to 100k iterations
      work = rand(100_000).to_s
      @socket.send(work)
      break if @interrupted
    end
    @workers.each{|w| w.stop }
    @ctx.destroy
  end

  def stop
    @interrupted = true
  end
end

ctx = ZMQ::Context.new
master = Master.new(ctx, 'inproc://example.push_pull')
master.spawn_workers
trap(:INT) do
  master.stop
end
master.start(ENV['MESSAGES'] || 1000)