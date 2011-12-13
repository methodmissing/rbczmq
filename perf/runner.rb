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

$:.unshift File.expand_path('lib')
require 'zmq'
require 'pp'

module Runner
  REMOTE_ENDPOINT = "tcp://127.0.0.1:5221"
  STATS_ENDPOINT = "tcp://127.0.0.1:5222"
  DEFAULT_MSG_COUNT = 100_000
  DEFAULT_MSG_SIZE = 100
  DEFAULT_ENCODING = :string
  STATS = Hash.new{|h,k| h[k] = [] }

  %w(INT TERM QUIT).each do |sig|
     trap(sig){ stop }
   end

  class << self
    attr_reader :msg_count, :msg_size, :encoding, :processes, :stats_buf
  end

  def start(test, count, size, encoding, processes = 1)
    @msg_count = (count || DEFAULT_MSG_COUNT).to_i
    @msg_size = (size || DEFAULT_MSG_SIZE).to_i
    @encoding = (encoding || DEFAULT_ENCODING).to_sym
    @processes = (processes || 1).to_i
    @stats_buf, @local_pids = [], []
    @processes.times do
      @local_pids << fork do
        before_fork
        require File.join(File.dirname(__FILE__), test.to_s, 'local')
        after_fork
      end
    end
    @remote_pid = fork do
      before_fork
      sleep 1
      require File.join(File.dirname(__FILE__), test.to_s, 'remote')
      after_fork
    end
    puts "Local pids: #{@local_pids.join(', ')}"
    puts "Remote pid: #{@remote_pid}"
    Process.waitpid(@remote_pid, Process::WNOHANG)
    @local_pids.each{|p| Process.waitpid(p) }
  ensure
    stop
  end

  def process_msg_count
    msg_count / processes
  end

  def before_fork
    if RUBY_PLATFORM =~ /darwin/
      ENV["MallocStackLogging"] = "1"
      ENV["MallocScribble"] = "1"
    end
    sample_mem(:before)
  end

  def after_fork
    sample_mem(:after)
    stats_buf.each{|s| String === s ? puts(s) : pp(s) }
  end

  def stop
    Process.kill(:INT, @remote_pid) rescue nil
    @local_pids.each{|p| Process.kill(:INT, p) rescue nil }
  end

  def payload
    @payload ||= "#{'0'*msg_size}"
  end

  def stats(start_time)
    end_time = Time.now
    elapsed = (end_time.to_f - start_time.to_f) * 1000000
    elapsed = 1 if elapsed == 0

    throughput = process_msg_count * 1000000 / elapsed
    megabits = throughput * msg_size * 8 / 1000000
    stats_buf << "====== [#{Process.pid}] transfer stats ======"
    stats_buf << "message size: %i [B]" % msg_size
    stats_buf << "message count: %i" % process_msg_count
    stats_buf << "mean throughput: %i [msg/s]" % throughput
    stats_buf << "mean throughput: %.3f [Mb/s]" % megabits
  end

  private
  def sample_mem(w)
    stats_buf << "====== [#{Process.pid}] ======"
    stats_buf << "Memory used #{w}: %dkb" % [mem_usage]
  end

  def mem_usage
    `ps -o rss= -p #{Process.pid}`.to_i
  end
  extend self
end