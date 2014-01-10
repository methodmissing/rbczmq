# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)

class TestZmqLogger < ZmqTestCase

  def setup
    @ctx = ZMQ::Context.new
  end

  def teardown
    @ctx.destroy
  end

  def test_invalid_socket
    socket = @ctx.socket(ZMQ::REQ)
    assert_raises ZMQ::Logger::InvalidSocketError do
      ZMQ::Logger.new(socket)
    end
  end

  def test_valid_socket
    socket = @ctx.socket(ZMQ::PUSH)
    assert_nothing_raised do
      ZMQ::Logger.new(socket)
    end
  end

  def test_send_message
    reader = @ctx.socket(ZMQ::PULL)
    port = reader.bind("tcp://*:*")
    assert port > 0
    writer = @ctx.socket(ZMQ::PUSH)
    writer.connect("tcp://localhost:#{port}")
    logger = ZMQ::Logger.new(writer)
    assert logger.debug("hellö world")
    message = reader.recv.force_encoding Encoding::UTF_8
    assert_not_nil message =~ /hellö world/
  end

end
