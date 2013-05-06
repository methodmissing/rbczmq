# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqBeacon < ZmqTestCase
  def test_beacon
    beacon = ZMQ::Beacon.new(80000)
    assert_instance_of ZMQ::Beacon, beacon
    assert_nil beacon.destroy
    assert_raises TypeError do
      ZMQ::Beacon.new(:invalid)
    end
  ensure
    beacon.destroy
  end

  def test_hostname
    beacon = ZMQ::Beacon.new(80000)
    assert_equal "127.0.0.1", beacon.hostname
  ensure
    beacon.destroy
  end

  def test_set_interval
    beacon = ZMQ::Beacon.new(80000)
    beacon.interval = 100
    assert_raises TypeError do
      beacon.interval = :invalid
    end
  ensure
    beacon.destroy
  end

  def test_noecho
    beacon = ZMQ::Beacon.new(80000)
    assert_nil beacon.noecho
  ensure
    beacon.destroy
  end

  def test_publish
    beacon = ZMQ::Beacon.new(80000)
    assert_raises TypeError do
      beacon.publish :invalid
    end
    assert_nil beacon.publish("test")
    assert_nil beacon.silence
  ensure
    beacon.destroy
  end

  def test_subscribe
    beacon = ZMQ::Beacon.new(80000)
    assert_raises TypeError do
      beacon.subscribe :invalid
    end
    assert_nil beacon.subscribe("test")
    assert_nil beacon.unsubscribe
  ensure
    beacon.destroy
  end
end