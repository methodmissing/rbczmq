# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqMessage < ZmqTestCase
  def test_message
    msg = ZMQ::Message.new
    assert_instance_of ZMQ::Message, msg
    assert_nil msg.destroy
  end

  def test_destroyed
    msg = ZMQ::Message("one", "two")
    assert_not_nil msg.encode
    assert !msg.gone?
    msg.destroy
    assert msg.gone?
    assert_nil msg.encode
  end

  def test_message_sugar
    msg = ZMQ::Message("one", "two", "three")
    assert_equal "one", msg.popstr
    assert_equal "two", msg.popstr
    assert_equal "three", msg.popstr
  end

  def test_size
    msg = ZMQ::Message.new
    assert_equal 0, msg.size
    msg.pushstr "test"
    assert_equal 1, msg.size
  end

  def test_content_size
    msg = ZMQ::Message.new
    assert_equal 0, msg.content_size
    msg.pushstr "test"
    assert_equal 4, msg.content_size
  end

  def test_push_pop
    msg = ZMQ::Message.new
    assert msg.push ZMQ::Frame("body")
    assert_equal 1, msg.size
    assert_equal 4, msg.content_size
    assert msg.push ZMQ::Frame("header")
    assert_equal 2, msg.size
    assert_equal 10, msg.content_size

    assert_equal ZMQ::Frame("header"), msg.pop
    assert_equal 1, msg.size
    assert_equal 4, msg.content_size

    assert_equal ZMQ::Frame("body"), msg.pop
    assert_equal 0, msg.size
    assert_equal 0, msg.content_size
  end

  def test_add
    msg = ZMQ::Message.new
    msg.push ZMQ::Frame("header")
    assert msg.add ZMQ::Frame("body")
    assert_equal 2, msg.size
    assert_equal ZMQ::Frame("header"), msg.pop
    assert_equal ZMQ::Frame("body"), msg.pop
  end

  def test_print
    msg = ZMQ::Message.new
    msg.push ZMQ::Frame("header")
    msg.add ZMQ::Frame("body")
    assert_nil msg.print
  end

  def test_first
    msg = ZMQ::Message.new
    assert_nil msg.first
    msg.push ZMQ::Frame("header")
    msg.add ZMQ::Frame("body")
    assert_equal ZMQ::Frame("header"), msg.first
  end

  def test_next
    msg = ZMQ::Message.new
    assert_nil msg.next
    msg.push ZMQ::Frame("header")
    msg.add ZMQ::Frame("body")
    assert_equal ZMQ::Frame("header"), msg.next
    assert_equal ZMQ::Frame("body"), msg.next
  end

  def test_last
    msg = ZMQ::Message.new
    assert_nil msg.last
    msg.push ZMQ::Frame("header")
    msg.add ZMQ::Frame("body")
    assert_equal ZMQ::Frame("body"), msg.last
  end

  def test_to_a
    assert_equal [], ZMQ::Message.new.to_a
    msg = ZMQ::Message("header", "body")
    assert_equal [ZMQ::Frame("header"), ZMQ::Frame("body")], msg.to_a
  end

  def test_remove
    msg = ZMQ::Message.new
    header = ZMQ::Frame("header")
    body = ZMQ::Frame("body")
    msg.push header
    msg.add body
    msg.remove(body)
    assert_equal 1, msg.size
    assert_equal header, msg.pop
  end

  def test_pushstr_popstr
    msg = ZMQ::Message.new
    assert msg.pushstr "body"
    assert_equal 1, msg.size
    assert_equal 4, msg.content_size
    assert msg.pushstr "header"
    assert_equal 2, msg.size
    assert_equal 10, msg.content_size

    assert_equal "header", msg.popstr
    assert_equal 1, msg.size
    assert_equal 4, msg.content_size

    assert_equal "body", msg.popstr
    assert_equal 0, msg.size
    assert_equal 0, msg.content_size
  end

  def test_addstr
    msg = ZMQ::Message.new
    msg.pushstr "header"
    assert msg.addstr "body"
    assert_equal 2, msg.size
    assert_equal "header", msg.popstr
    assert_equal "body", msg.popstr
  end

  def test_wrap_unwrap
    msg = ZMQ::Message.new
    body = ZMQ::Frame("body")
    assert_nil msg.wrap(body)
    assert_equal 2, msg.size
    assert_equal 4, msg.content_size
    assert_equal body, msg.pop
    assert_equal ZMQ::Frame(""), msg.pop

    assert_equal 0, msg.size
    assert_nil msg.wrap(body)
    assert_equal 2, msg.size
    assert_equal body, msg.unwrap
    assert_equal 0, msg.size
  end

  def test_dup
    msg =  ZMQ::Message.new
    msg.pushstr "header"
    dup_msg = msg.dup
    assert_not_equal dup_msg.object_id, msg.object_id
    assert_equal msg.size, dup_msg.size
    assert_equal "header", msg.popstr
  end

  def test_encode_decode
    msg =  ZMQ::Message.new
    msg.pushstr "body"
    msg.pushstr "header"

    expected = "\006header\004body"
    assert_equal expected, msg.encode

    decoded = ZMQ::Message.decode(expected)
    assert_equal "header", decoded.popstr
    assert_equal "body", decoded.popstr

    assert_nil ZMQ::Message.decode("tainted")
  end

  def test_equals
    msg = ZMQ::Message.new
    msg.pushstr "body"
    msg.pushstr "header"

    dup = msg.dup
    assert_equal msg, msg
    assert msg.eql?(dup)

    other =  ZMQ::Message.new
    other.pushstr "header"
    other.pushstr "body"

    assert_not_equal msg, other
    assert !msg.eql?(other)
    assert other.eql?(other)
  end

  def test_message_with_frames
    msg = ZMQ::Message.new
    frame = ZMQ::Frame.new("hello")
    assert_equal "hello", frame.data
    msg.add frame
    # frame is owned by message, but message is still owned by ruby,
    # so the frame data should still be accessible:
    assert_equal "hello", frame.data
  end

  def test_messge_add_frame_twice
    msg = ZMQ::Message.new
    frame = ZMQ::Frame.new("hello")
    msg.add frame
    assert_raises ZMQ::Error do
      msg.add frame
    end
  end

  def test_message_is_gone_after_send
    ctx = ZMQ::Context.new
    endpoint = "inproc://test.test_message_is_gone_after_send"
    push = ctx.bind(:PUSH, endpoint)
    pull = ctx.connect(:PULL, endpoint)
    msg = ZMQ::Message.new
    frame = ZMQ::Frame.new("hello")
    msg.add frame
    push.send_message(msg)
    assert msg.gone?
    assert frame.gone?
  ensure
    ctx.destroy
  end

  def test_message_has_frames_on_receive
    ctx = ZMQ::Context.new
    endpoint = "inproc://test.test_message_is_gone_after_send"
    push = ctx.bind(:PUSH, endpoint)
    pull = ctx.connect(:PULL, endpoint)
    msg = ZMQ::Message.new
    msg.add ZMQ::Frame.new("hello")
    msg.add ZMQ::Frame.new("world")
    push.send_message(msg)
    received = pull.recv_message
    assert_not_nil received
    assert_equal 2, received.size
    assert_equal "hello", received.first.data
    assert_equal "world", received.next.data
  ensure
    ctx.destroy
  end

  def test_message_remove_frame
    msg = ZMQ::Message.new
    frame = ZMQ::Frame.new("hello")
    msg.add frame
    assert_equal 1, msg.size
    # same object is returned
    assert_equal frame.object_id, msg.first.object_id
    msg.remove frame
    assert_equal 0, msg.size
  end

  def test_message_array_same_frame_objects
    msg = ZMQ::Message.new
    frame = ZMQ::Frame.new("hello")
    msg.add frame
    ary = msg.to_a
    assert_equal 1, ary.count
    assert_equal frame, ary.first
    assert_equal frame.object_id, ary.first.object_id
  end

  def test_message_unwrap_dup
    ctx = ZMQ::Context.new
    router = ctx.socket(ZMQ::ROUTER)
    port = router.bind("tcp://127.0.0.1:*")
    req = ctx.connect(ZMQ::REQ, "tcp://127.0.0.1:#{port}")
    msg = ZMQ::Message.new
    msg.addstr "hello world"
    req.send_message msg
    msg2 = router.recv_message
    identity = msg2.unwrap
    assert_equal "hello world", msg2.first.data
    msg3 = ZMQ::Message.new
    msg3.addstr "reply"
    msg3.wrap identity.dup
    assert_equal 3, msg3.size
  ensure
    ctx.destroy
  end
end
