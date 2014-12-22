# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)
require 'socket'

class TestZmqBeacon < ZmqTestCase
  def setup
    GC.start
  end

  def test_beacon
    beacon = ZMQ::Beacon.new(0)
    assert_instance_of ZMQ::Beacon, beacon
    assert_nil beacon.destroy
    assert_raises TypeError do
      ZMQ::Beacon.new(:invalid)
    end
  ensure
    beacon.destroy
  end

  def test_hostname
    beacon = ZMQ::Beacon.new(0)
    assert_instance_of String, beacon.hostname
  ensure
    beacon.destroy
  end

  def test_set_interval
    beacon = ZMQ::Beacon.new(0)
    beacon.interval = 100
    assert_raises TypeError do
      beacon.interval = :invalid
    end
  ensure
    beacon.destroy
  end

  def test_noecho
    beacon = ZMQ::Beacon.new(0)
    assert_nil beacon.noecho
  ensure
    beacon.destroy
  end

  def test_publish
    beacon = ZMQ::Beacon.new(0)
    assert_raises TypeError do
      beacon.publish :invalid
    end
    assert_nil beacon.publish("test")
    assert_nil beacon.silence
  ensure
    beacon.destroy
  end

  def test_subscribe
    beacon = ZMQ::Beacon.new(0)
    assert_raises TypeError do
      beacon.subscribe :invalid
    end
    assert_nil beacon.subscribe("test")
    assert_nil beacon.unsubscribe
  ensure
    beacon.destroy
  end

  def test_pipe
    GC.start
    beacon = ZMQ::Beacon.new(0)
    assert_instance_of ZMQ::Socket::Pair, beacon.pipe
    GC.start # check GC cycle with "detached" socket
  ensure
    beacon.destroy
  end

  def test_announce_lookup
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "tcp://127.0.0.1:0")
    # official ZRE port, don't know how to introspect beacon to get bound port
    beacon_port = 5670
    service_beacon = ZMQ::Beacon.new(beacon_port)
    service_beacon.publish(rep.endpoint)
    sleep(0.5)
    client_beacon = ZMQ::Beacon.new(beacon_port)
    client_beacon.subscribe("tcp")
    client_beacon.pipe.rcvtimeo = 1000
    sender_address = client_beacon.pipe.recv
    client_address = client_beacon.pipe.recv
    assert_equal rep.endpoint, client_address
    req = ctx.connect(:REQ, client_address)
    req.send "ping"
    assert_equal "ping", rep.recv
  ensure
    ctx.destroy
    service_beacon.destroy
    client_beacon.destroy
  end
end
