# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)
require 'socket'

class TestZmqBeacon < ZmqTestCase
  def setup
    r = Random.new
    begin
      # find a random port number that we are able to bind to, and use this port
      # for the beacon tests.
      udp = UDPSocket.new
      @port = 10000 + r.rand(50000)
      udp.bind('0.0.0.0', @port)
    rescue Errno::EADDRINUSE
      udp.close
      retry
    ensure
      udp.close
    end
  end

  def test_beacon
    beacon = ZMQ::Beacon.new(@port)
    assert_instance_of ZMQ::Beacon, beacon
    assert_nil beacon.destroy
    assert_raises TypeError do
      ZMQ::Beacon.new(:invalid)
    end
  ensure
    beacon.destroy
  end

  def test_hostname
    beacon = ZMQ::Beacon.new(@port)
    assert_instance_of String, beacon.hostname
  ensure
    beacon.destroy
  end

  def test_set_interval
    beacon = ZMQ::Beacon.new(@port)
    beacon.interval = 100
    assert_raises TypeError do
      beacon.interval = :invalid
    end
  ensure
    beacon.destroy
  end

  def test_noecho
    beacon = ZMQ::Beacon.new(@port)
    assert_nil beacon.noecho
  ensure
    beacon.destroy
  end

  def test_publish
    beacon = ZMQ::Beacon.new(@port)
    assert_raises TypeError do
      beacon.publish :invalid
    end
    assert_nil beacon.publish("test")
    assert_nil beacon.silence
  ensure
    beacon.destroy
  end

  def test_subscribe
    beacon = ZMQ::Beacon.new(@port)
    assert_raises TypeError do
      beacon.subscribe :invalid
    end
    assert_nil beacon.subscribe("test")
    assert_nil beacon.unsubscribe
  ensure
    beacon.destroy
  end

  def test_pipe
    beacon = ZMQ::Beacon.new(@port)
    assert_instance_of ZMQ::Socket::Pair, beacon.pipe
    GC.start # check GC cycle with "detached" socket
  ensure
    beacon.destroy
  end

  def test_announce_lookup
    ctx = ZMQ::Context.new
    address = "tcp://127.0.0.1:90000"
    rep = ctx.bind(:REP, address)
    service_beacon = ZMQ::Beacon.new(@port)
    service_beacon.publish(address)
    sleep(0.5)
    client_beacon = ZMQ::Beacon.new(@port)
    client_beacon.subscribe("tcp")
    client_beacon.pipe.rcvtimeo = 1000
    sender_address = client_beacon.pipe.recv
    client_address = client_beacon.pipe.recv
    assert_equal address, client_address
    req = ctx.connect(:REQ, client_address)
    req.send "ping"
    assert_equal "ping", rep.recv
  ensure
    ctx.destroy
    service_beacon.destroy
    client_beacon.destroy
  end
end
