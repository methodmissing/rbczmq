# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqSocket < ZmqTestCase
  def test_recv_timeout
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://test.socket-recv-timeout")
    start = Time.now.to_f
    rep.rcvtimeo = 300
    rep.recv
    elapsed = Time.now.to_f - start
    assert_in_delta elapsed, 0.29, 0.32
  ensure
    ctx.destroy
  end

  def test_send_timeout
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    if ZMQ.version3?
      rep.sndhwm = 1
    else
      rep.hwm = 1
    end
    rep.bind("inproc://test.socket-send-timeout")
    req = ctx.connect(:REQ, "inproc://test.socket-send-timeout")
    # Cannot test much other than normal transfer's not disrupted
    req.sndtimeo = 100
    req.send "msg"
    assert_equal "msg", rep.recv
  ensure
    ctx.destroy
  end

  def test_connect_no_io_thread
    ctx = ZMQ::Context.new(0)
    sock = ctx.socket(:REP)
    assert_raises ZMQ::Error do
      sock.bind("tcp://127.0.0.1:*")
    end
  ensure
    ctx.destroy
  end

  def test_fd
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    assert Fixnum === sock.fd
    assert_equal(-1, sock.fd)
    assert_equal sock.fd, sock.to_i
  ensure
    ctx.destroy
  end

  def test_type
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    assert_equal ZMQ::REP, sock.type
  ensure
    ctx.destroy
  end

  def test_readable_p
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    rep.bind("inproc://test.socket-readable_p")
    req = ctx.connect(:REQ, "inproc://test.socket-readable_p")
    assert req.writable?
    req.send("m")
    sleep 0.1
    assert rep.readable?
  ensure
    ctx.destroy
  end

  def test_send_socket
    ctx = ZMQ::Context.new
    push = ctx.socket(:PUSH)
    assert_raises ZMQ::Error do
      push.recv
    end
  ensure
    ctx.destroy
  end

  def test_receive_socket
    ctx = ZMQ::Context.new
    pull = ctx.socket(:PULL)
    assert_raises ZMQ::Error do
      pull.send("message")
    end
  ensure
    ctx.destroy
  end

  def test_gc_context_reaped
    pub = ZMQ::Context.new.socket(:PUB)
    GC.start
    pub.bind("inproc://test.socket-gc_context_reaped")
    GC.start
    pub.send("test")
    GC.start
    pub.close
  ensure
    ZMQ.context.destroy
  end

  def test_bind
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    assert(sock.state & ZMQ::Socket::PENDING)
    port = sock.bind("tcp://127.0.0.1:*")
    assert sock.fd != -1
    assert(sock.state & ZMQ::Socket::BOUND)
    tcp_sock = nil
    assert_nothing_raised do
      tcp_sock = TCPSocket.new("127.0.0.1", port)
    end
  ensure
    ctx.destroy
    tcp_sock.close if tcp_sock
  end

  def test_connect
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    port = rep.bind("inproc://test.socket-connect")
    req = ctx.socket(:PAIR)
    assert(req.state & ZMQ::Socket::PENDING)
    req.connect("inproc://test.socket-connect")
    assert req.fd != -1
    assert(req.state & ZMQ::Socket::CONNECTED)
  ensure
    ctx.destroy
  end

  def test_connect_all
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    port = rep.bind("inproc://test.socket-connect")
    req = ctx.socket(:PAIR)
    assert(req.state & ZMQ::Socket::PENDING)
    req.connect_all("inproc://test.socket-connect")
    assert req.fd != -1
    assert(req.state & ZMQ::Socket::CONNECTED)

    # TODO - test SRV lookups - insufficient infrastructure for resolver tests
  ensure
    ctx.destroy
  end

  def test_bind_connect_errors
    ctx = ZMQ::Context.new
    req = ctx.socket(:REQ)
    rep = ctx.socket(:REP)
    assert_raises Errno::EINVAL do
      rep.bind "bad uri"
    end
    assert_raises Errno::EINVAL do
      req.connect "bad uri"
    end
  ensure
    ctx.destroy
  end

  def test_to_s
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    rep = ctx.socket(:REP)
    port = rep.bind("tcp://127.0.0.1:*")
    req = ctx.socket(:REQ)
    assert(req.state & ZMQ::Socket::PENDING)
    req.connect("tcp://127.0.0.1:#{port}")
    assert_equal "PAIR socket", sock.to_s
    assert_equal "REP socket bound to tcp://127.0.0.1:*", rep.to_s
    assert_equal "REQ socket connected to tcp://127.0.0.1:#{port}", req.to_s


    port2 = rep.bind("tcp://127.0.0.1:*")
    req.connect("tcp://127.0.0.1:#{port2}")
    assert_equal "REP socket bound to tcp://127.0.0.1:*, tcp://127.0.0.1:*", rep.to_s
    assert_equal "REQ socket connected to tcp://127.0.0.1:#{port}, tcp://127.0.0.1:#{port2}", req.to_s
  ensure
    ctx.destroy
  end

  def test_endpoint
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    port = rep.bind("tcp://127.0.0.1:*")
    req = ctx.socket(:REQ)
    req.connect("tcp://127.0.0.1:#{port}")
    assert_equal "tcp://127.0.0.1:*", rep.endpoint
    assert_equal "tcp://127.0.0.1:#{port}", req.endpoint
  ensure
    ctx.destroy
  end

  def test_endpoints
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    port = rep.bind("tcp://127.0.0.1:*")
    req = ctx.socket(:REQ)
    req.connect("tcp://127.0.0.1:#{port}")
    assert_equal ["tcp://127.0.0.1:*"], rep.endpoints
    assert_equal ["tcp://127.0.0.1:#{port}"], req.endpoints

    port2 = rep.bind("tcp://127.0.0.1:*")
    req.connect("tcp://127.0.0.1:#{port2}")
    assert_equal ["tcp://127.0.0.1:*", "tcp://127.0.0.1:*"], rep.endpoints
    assert_equal ["tcp://127.0.0.1:#{port}", "tcp://127.0.0.1:#{port2}"], req.endpoints
  ensure
    ctx.destroy
  end

  def test_close
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    port = sock.bind("tcp://127.0.0.1:*")
    assert sock.fd != -1
    other = ctx.socket(:REQ)
    other.connect("tcp://127.0.0.1:#{port}")
    other.send("test")
    assert_equal "test", sock.recv
    sock.close
    other.close
    # zsocket_destroy in libczmq recycles the socket instance. libzmq don't support / expose underlying
    # connection state or teardown through public API, thus we can't assert a PENDING socket state on close
    # through the Ruby API as the socket instance has already been recycled.
    begin
      assert_equal ZMQ::Socket::PENDING, sock.state
    rescue => e
      assert_instance_of ZMQ::Error, e
      assert_match(/ZMQ::Socket instance \w* has been destroyed by the ZMQ framework/, e.message)
    end
    sleep 0.2
    assert_raises Errno::ECONNREFUSED do
      TCPSocket.new("127.0.0.1", port)
    end
  ensure
    ctx.destroy
  end

  def test_send_receive
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.bind("inproc://test.socket-send_receive")
    req = ctx.socket(:PAIR)
    req.connect("inproc://test.socket-send_receive")
    assert req.send("ping")
    assert_equal "ping", rep.recv
  ensure
    ctx.destroy
  end

  def test_verbose
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.verbose = true
    rep.bind("inproc://test.socket-verbose")
    req = ctx.socket(:PAIR)
    req.verbose = true
    req.connect("inproc://test.socket-verbose")
    assert req.send("ping")
    assert_equal "ping", rep.recv
    req.send_frame(ZMQ::Frame("frame"))
    assert_equal ZMQ::Frame("frame"), rep.recv_frame
  ensure
    ctx.destroy
  end

  def test_receive_nonblock
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    port = rep.bind("tcp://127.0.0.1:*")
    req = ctx.socket(:REQ)
    req.connect("tcp://127.0.0.1:#{port}")
    assert req.send("ping")
    assert_equal nil, rep.recv_nonblock
    sleep 0.2
    assert_equal "ping", rep.recv_nonblock
  ensure
    ctx.destroy
  end

  def test_send_multi
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.bind("inproc://test.socket-send_multi")
    req = ctx.socket(:PAIR)
    req.connect("inproc://test.socket-send_multi")
    assert req.sendm("batch")
    req.sendm("of")
    req.send("messages")
    assert_equal "batch", rep.recv
    assert_equal "of", rep.recv
    assert_equal "messages", rep.recv
  ensure
    ctx.destroy
  end

  def test_send_receive_frame
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    port = rep.bind("tcp://127.0.0.1:*")
    req = ctx.socket(:REQ)
    req.connect("tcp://127.0.0.1:#{port}")
    ping = ZMQ::Frame("ping")
    assert req.send_frame(ping)
    assert_equal ZMQ::Frame("ping"), rep.recv_frame
    assert rep.send_frame(ZMQ::Frame("pong"))
    assert_equal ZMQ::Frame("pong"), req.recv_frame
    assert req.send_frame(ZMQ::Frame("pong"))
    frame = rep.recv_frame_nonblock
    if frame
      assert_equal ZMQ::Frame("pong"), frame
    else
      sleep 0.3
      assert_equal ZMQ::Frame("pong"), rep.recv_frame_nonblock
    end
  ensure
    ctx.destroy
  end

  def test_send_frame_more
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.bind("inproc://test.socket-send_frame_more")
    req = ctx.socket(:PAIR)
    req.connect("inproc://test.socket-send_frame_more")
    5.times do |i|
      frame = ZMQ::Frame("m#{i}")
      req.send_frame(frame, ZMQ::Frame::MORE)
    end
    req.send_frame(ZMQ::Frame("m6"))
    expected, frames = %w(m0 m1 m2 m3 m4), []
    5.times do
      frames << rep.recv_frame.data
    end
    assert_equal expected, frames
  ensure
    ctx.destroy
  end

  def test_send_frame_reuse
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.bind("inproc://test.socket-send_frame_reuse")
    req = ctx.socket(:PAIR)
    req.connect("inproc://test.socket-send_frame_reuse")
    frame = ZMQ::Frame("reused_frame")
    5.times do |i|
      req.send_frame(frame, :REUSE)
    end
    expected, frames = ( %w(reused_frame) * 5), []
    5.times do
      frames << rep.recv_frame.data
    end
    assert_equal expected, frames
  ensure
    ctx.destroy
  end

  def test_send_frame_dontwait
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.bind("inproc://test.socket-send_frame_dontwait")
    req = ctx.socket(:PAIR)
    req.connect("inproc://test.socket-send_frame_dontwait")
    5.times do |i|
      frame = ZMQ::Frame("m#{i}")
      req.send_frame(frame, ZMQ::Frame::DONTWAIT)
    end
    expected, frames = %w(m0 m1 m2 m3 m4), []
    5.times do
      frames << rep.recv_frame.data
    end
    assert_equal expected, frames
  ensure
    ctx.destroy
  end

  def test_send_receive_message
    ctx = ZMQ::Context.new
    rep = ctx.socket(:PAIR)
    rep.verbose = true
    rep.bind("inproc://test.socket-send_receive_message")
    req = ctx.socket(:PAIR)
    req.verbose = true
    req.connect("inproc://test.socket-send_receive_message")

    msg = ZMQ::Message.new
    msg.push ZMQ::Frame("header")

    assert_nil req.send_message(msg)

    recvd_msg = rep.recv_message
    assert_instance_of ZMQ::Message, recvd_msg
    assert_equal ZMQ::Frame("header"), recvd_msg.pop
  ensure
    ctx.destroy
  end

  def test_type_str
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    assert_equal "PAIR", sock.type_str
  ensure
    ctx.destroy
  end

  def test_sock_options
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    sock.verbose = true

    if ZMQ.version3?
      assert_equal 1000, sock.sndhwm
      sock.sndhwm = 10000
      assert_equal 10000, sock.sndhwm      

      assert_equal 1000, sock.rcvhwm
      sock.rcvhwm = 10000
      assert_equal 10000, sock.rcvhwm
    else
      assert_equal 0, sock.hwm
      sock.hwm = 1000
      assert_equal 1000, sock.hwm

      assert_equal 0, sock.swap
      sock.swap = 1000
      assert_equal 1000, sock.swap

      assert_equal(-1, sock.recovery_ivl_msec)
      sock.recovery_ivl_msec = 20
      assert_equal 20, sock.recovery_ivl_msec
    end

    assert_equal 0, sock.affinity
    sock.affinity = 1
    assert_equal 1, sock.affinity

    if ZMQ.version3?
      assert_equal 100, sock.rate
      sock.rate = 50000
      assert_equal 50000, sock.rate
    else
      assert_equal 40000, sock.rate
      sock.rate = 50000
      assert_equal 50000, sock.rate
    end

    if ZMQ.version3?
      assert_equal 10000, sock.recovery_ivl
      sock.recovery_ivl = 20
      assert_equal 20, sock.recovery_ivl
    else
      assert_equal 10, sock.recovery_ivl
      sock.recovery_ivl = 20
      assert_equal 20, sock.recovery_ivl
    end

    if ZMQ.version2?
      assert_equal true, sock.mcast_loop?
      sock.mcast_loop = false
      assert !sock.mcast_loop?
    end

    assert_equal 0, sock.sndbuf
    sock.sndbuf = 1000
    assert_equal 1000, sock.sndbuf

    assert_equal 0, sock.rcvbuf
    sock.rcvbuf = 1000
    assert_equal 1000, sock.rcvbuf

    assert_equal(-1, sock.linger)
    sock.linger = 10
    assert_equal 10, sock.linger

    assert_equal 100, sock.backlog
    sock.backlog = 200
    assert_equal 200, sock.backlog

    assert_equal 100, sock.reconnect_ivl
    sock.reconnect_ivl = 200
    assert_equal 200, sock.reconnect_ivl

    assert_equal 0, sock.reconnect_ivl_max
    sock.reconnect_ivl_max = 5
    assert_equal 5, sock.reconnect_ivl_max

    assert_equal -1, sock.rcvtimeo
    sock.rcvtimeo = 200
    assert_equal 200, sock.rcvtimeo

    assert_equal -1, sock.sndtimeo
    sock.sndtimeo = 200
    assert_equal 200, sock.sndtimeo

    sock.identity = "anonymous"
    assert_raises ZMQ::Error do
      sock.identity = ""
    end
    assert_raises ZMQ::Error do
      sock.identity = ("*" * 256)
    end

    assert !sock.rcvmore?

    assert_equal 0, sock.events

    sub_sock = ctx.socket(:SUB)
    sub_sock.verbose = true
    sub_sock.subscribe("ruby")
    sub_sock.unsubscribe("ruby")
  ensure
    ctx.destroy
  end
end
