# encoding: utf-8

$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'
require 'pp'

# PUB / SUB topology

Thread.abort_on_exception = true

class Consumer
  attr_reader :thread
  def initialize(ctx, endpoint, topic = "")
    @thread = nil
    @connect = Proc.new do
      @socket = ctx.socket(:SUB)
      @socket.subscribe("")
      # verbose output
      @socket.verbose = true
      @socket.subscribe(topic)
      @socket.connect(endpoint)
      @socket.linger = 1
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

class Producer
  def initialize(ctx, endpoint, topic = "")
    @ctx, @endpoint, @topic, @consumers = ctx, endpoint, topic, []
    @socket = ctx.socket(:PUB)
    # verbose output
    @socket.verbose = true
    @socket.bind(endpoint)
    @socket.linger = 1
    @interrupted = false
  end

  def spawn_consumers(count = 10)
    count.times do
      @consumers << Consumer.new(@ctx, @endpoint, @topic).start
      sleep 0.01 # give each thread time to spin up
    end
  end

  def start(messages = 100)
    messages.to_i.times do
      # Tasks are hot loops with random 0 to 100k iterations
      work = "#{@topic}#{rand(100_000).to_s}"
      @socket.send(work)
      break if @interrupted
    end
    @consumers.each{|c| c.stop }
    @ctx.destroy
  end

  def stop
    @interrupted = true
  end
end

ctx = ZMQ::Context.new
producer = Producer.new(ctx, 'inproc://example.pub_sub')
producer.spawn_consumers
trap(:INT) do
  producer.stop
end
producer.start(ENV['MESSAGES'] || 1000)