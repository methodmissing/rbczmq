# encoding: utf-8

$:.unshift File.expand_path('lib')
require 'zmq'

class Runner
  DEFAULT_MSG_COUNT = 100_000
  DEFAULT_MSG_SIZE = 100
  DEFAULT_ENCODING = :string

  attr_reader :msg_count, :msg_size, :encoding, :workers_count, :stats_buf

  def initialize(msg_count, msg_size, encoding, workers_count = 1)
    @msg_count = (msg_count || DEFAULT_MSG_COUNT).to_i
    @msg_size = (msg_size || DEFAULT_MSG_SIZE).to_i
    @encoding = (encoding || DEFAULT_ENCODING).to_sym
    @workers_count = (workers_count || 1).to_i
    @stats_buf, @workers = [], []
    register_signal_handlers
  end

  def start(test)
    @workers_count.times{ fork_local(test) }
    fork_remote(test)
    puts "Workers: #{@workers.join(', ')}"
    puts "Producer: #{@producer}"
    after_start
  ensure
    stop
  end

  def process_msg_count
    msg_count / workers_count
  end

  def endpoint
    self.class.const_get(:ENDPOINT)
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
    stats_buf << "message encoding: %s" % encoding
    stats_buf << "message size: %i [B]" % msg_size
    stats_buf << "message count: %i" % process_msg_count
    stats_buf << "mean throughput: %i [msg/s]" % throughput
    stats_buf << "mean throughput: %.3f [Mb/s]" % megabits
  end

  private
  def register_signal_handlers
    %w(INT TERM QUIT).each do |sig|
       trap(sig){ stop }
     end
  end

  def before_fork
    sample_mem(:before)
  end

  def after_fork
    sample_mem(:after)
    stats_buf.each{|s| puts(s) }
  end

  def sample_mem(w)
    stats_buf << "[#{$$}] Memory used #{w}: %dkb" % `ps -o rss= -p #{$$}`.to_i
  end
end

class ProcessRunner < Runner
  ENDPOINT = "tcp://127.0.0.1:5221"

  def stop
    Process.kill(:INT, @producer) rescue nil
    @workers.each{|p| Process.kill(:INT, p) rescue nil }
  end

  private
  def after_start
    Process.waitpid(@producer, Process::WNOHANG)
    @workers.each{|p| Process.waitpid(p) }
  end

  def fork_local(test)
    @workers << fork do
      before_fork
      require File.join(File.dirname(__FILE__), test.to_s, 'local')
      after_fork
    end
  end

  def fork_remote(test)
    @producer = fork do
      before_fork
      sleep 1
      require File.join(File.dirname(__FILE__), test.to_s, 'remote')
      after_fork
    end
  end
end

class ThreadRunner < Runner
  ENDPOINT = "inproc://perf"

  def stop
    @producer.kill rescue nil
    @workers.each{|t| t.kill rescue nil }
  end

  private
  def after_start
    @producer.join
    @workers.each{|t| t.join }
  end

  def fork_local(test)
    @workers << Thread.new do
      before_fork
      require File.join(File.dirname(__FILE__), test.to_s, 'local')
      after_fork
    end
  end

  def fork_remote(test)
    @producer = Thread.new do
      before_fork
      sleep 1
      require File.join(File.dirname(__FILE__), test.to_s, 'remote')
      after_fork
    end
  end
end