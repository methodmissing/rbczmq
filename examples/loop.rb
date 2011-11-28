$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'
require 'pp'

class ConsumerHandler < ZMQ::Handler
  def initialize(socket, consumer)
    super
    @consumer = consumer
  end

  def on_readable
    @consumer.perform socket.recv_nonblock
  end
end

class ProducerHandler < ZMQ::Handler
  def initialize(socket, producer)
    super
    @producer = producer
  end

  def on_writable
    @producer.work
  end
end

class Consumer
  attr_reader :thread
  def initialize(ctx, endpoint, topic = "")
    @socket = ctx.socket(:SUB)
    @socket.connect(endpoint)
    # verbose output
    @socket.verbose = true
    @socket.subscribe(topic)
    @jobs, @working = 0, 0.0
  end

  def start
    ZL.register_readable(@socket, ConsumerHandler, self)
    self
  end

  def stop
    ZL.remove_socket(@socket)
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
    end
  end

  def start
    ZL.register_writable(@socket, ProducerHandler, self)
  end

  def stop
    @consumers.each{|c| c.stop }
    ZL.remove_socket(@socket)
    ZL.stop
    @ctx.destroy
  end

  def work
    work = "#{@topic}#{rand(100_000).to_s}"
    @socket.send(work)
  end
end

ZL.run do
  ctx = ZMQ::Context.new
  producer = Producer.new(ctx, 'inproc://example.loop')
  producer.spawn_consumers
  trap(:INT) do
    producer.stop
  end
  producer.start
end