# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestReqSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REQ)
    assert_equal ZMQ::REQ, sock.type
    assert_equal "REQ socket", sock.to_s
  ensure
    ctx.destroy
  end
end