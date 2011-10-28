$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'
require 'pp'

# PUSH / PULL topology that demonstrates work being distributed in round robin fashion to a pool of worker threads.
# This pattern is a good start where little operational complexity is crucial and can scale out to multiple processes
# and boxes by just swapping out the transport.

class Worker
  def initialize(ctx, endpoint, watermark = 1000)
    @thread = nil
    @socket = ctx.socket(:PULL)
    # verbose output
    @socket.verbose = true
    @socket.connect(endpoint)
    # Limit the amount of work queued for this worker. When the high water mark ceiling hits, a particular worker
    # is ignored during round robin distribution
    @socket.hwm = watermark
    # wait up to 100msec for socket close
    @socket.linger = 100
    @jobs, @working = 0, 0.0
  end

  def start
    @thread = Thread.new do
      loop{ perform(@socket.recv) }
    end
    self
  end

  def stop
    @thread.kill if @thread
    @socket.close
  end

  def perform(work)
    # Random hot loop to simulate CPU intensive work
    start = Time.now
    work.to_i.times{}
    @jobs += 1
    @working += (Time.now - start).to_f
  end

  def stats
    "Processed #{@jobs} jobs in %.4f seconds" % @working
  end
end

class Master
  attr_reader :workers
  def initialize(ctx, endpoint)
    @ctx, @endpoint, @workers = ctx, endpoint, []
    @socket = ctx.socket(:PUSH)
    # verbose output
    @socket.verbose = true
    @socket.bind(endpoint)
    @interrupted = false
  end

  def spawn_workers(count = 10)
    count.times do
      @workers << Worker.new(@ctx, @endpoint).start
    end
  end

  def start
    loop do
      break if @interrupted
      # Tasks are hot loops with random 0 to 100k iterations
      work = rand(100_000).to_s
      @socket.send(work)
    end
    @socket.close
    @workers.each{|w| w.stop }
    pp @workers.map(&:stats)
  end

  def stop
    @interrupted = true
  end
end

ctx = ZMQ::Context.new
master = Master.new(ctx, 'inproc://workers.example')
master.spawn_workers
trap(:INT) do
  master.stop
end
master.start