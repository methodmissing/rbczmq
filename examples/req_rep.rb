$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'
require 'pp'

# REQ / REP topology

Thread.abort_on_exception = true

class Server
  def initialize(ctx, endpoint)
    @thread = nil
    @connect = Proc.new do
      @socket = ctx.socket(:REP)
      # verbose output
      @socket.verbose = true
      @socket.bind(endpoint)
      @socket.linger = 1
    end
    @jobs, @working = 0, 0.0
  end

  def start
    @thread = Thread.new do
      @connect.call
      loop do
        perform(@socket.recv)
        break if Thread.current[:interrupted]
      end
    end
    self
  end

  def stop
    return unless @thread
    @thread[:interrupted] = true
    @thread.join(0.1) unless @thread.stop?
    stats
  end

  def perform(work)
    # Random hot loop to simulate CPU intensive work
    start = Time.now
    work.to_i.times{}
    @jobs += 1
    @working += (Time.now - start).to_f
    @socket.send "done"
  end

  private
  def stats
    puts "Processed #{@jobs} jobs in %.4f seconds" % @working
    $stdout.flush
  end
end

class Client
  def initialize(ctx, endpoint)
    @ctx, @endpoint, @server, @interrupted = ctx, endpoint, nil, false
    @socket = ctx.socket(:REQ)
    # verbose output
    @socket.verbose = true
  end

  def spawn_server
    @server = Server.new(@ctx, @endpoint).start
    sleep 0.01 # give each thread time to spin up
    connect
  end

  def start(messages = 100)
    messages.to_i.times do
      request = "#{@topic}#{rand(100_000).to_s}"
      @socket.send(request)
      response = @socket.recv
      break if @interrupted
    end
    @server.stop
    @ctx.destroy
  end

  def stop
    @interrupted = true
  end
  private
  def connect
    @socket.connect(@endpoint)
    @socket.linger = 1
  end
end

ctx = ZMQ::Context.new
client = Client.new(ctx, 'inproc://example.req_rep')
client.spawn_server
trap(:INT) do
  client.stop
end
client.start(ENV['MESSAGES'] || 1000)