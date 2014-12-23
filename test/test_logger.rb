# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)

class TestZmqLogger < ZmqTestCase
  def test_invalid_socket
    ctx = ZMQ::Context.new
    socket = ctx.socket(ZMQ::REQ)
    assert_raises ZMQ::Logger::InvalidSocketError do
      ZMQ::Logger.new(socket)
    end
  ensure
    ctx.destroy
  end

  def test_valid_socket
    ctx = ZMQ::Context.new
    socket = ctx.socket(ZMQ::PUSH)
    assert ZMQ::Logger.new(socket)
  ensure
    ctx.destroy
  end

  def test_send_message
    ctx = ZMQ::Context.new
    reader = ctx.socket(ZMQ::PULL)
    port = reader.bind("tcp://*:*")
    assert port > 0
    writer = ctx.socket(ZMQ::PUSH)
    writer.connect("tcp://localhost:#{port}")
    logger = ZMQ::Logger.new(writer)
    assert logger.debug("hellö world")
    message = reader.recv.force_encoding Encoding::UTF_8
    assert message =~ /hellö world/
  ensure
    ctx.destroy
  end
end
