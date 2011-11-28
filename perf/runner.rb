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
    GC::Profiler.enable if defined?(GC::Profiler)
  end

  def after_fork
    GC::Profiler.disable if defined?(GC::Profiler)
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
    return # disabled in the interim - too verbose
    stats_buf << "====== [#{Process.pid}] ======"
    stats_buf << "Memory used #{w}: %dkb" % [mem_usage]
    stats_buf << GC::Profiler.result if defined?(GC::Profiler)
    stats_buf << GC.stat if GC.respond_to?(:stat)
  end

  def mem_usage
    `ps -o rss= -p #{Process.pid}`.to_i
  end
  extend self
end