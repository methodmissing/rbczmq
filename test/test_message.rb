require File.join(File.dirname(__FILE__), 'helper')

class TestZmqMessage < ZmqTestCase
  def test_message
    msg = ZMQ::Message.new
    assert_instance_of ZMQ::Message, msg
    assert_nil msg.destroy
  end

  def test_size
    msg = ZMQ::Message.new
    assert_equal 0, msg.size
  end

  def test_content_size
    msg = ZMQ::Message.new
    assert_equal 0, msg.content_size
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
    assert msg.wrap(body)
    assert_equal 2, msg.size
    assert_equal 4, msg.content_size
    assert_equal body, msg.pop
    assert_equal ZMQ::Frame(""), msg.pop

    assert_equal 0, msg.size
    assert msg.wrap(body)
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
    msg =  ZMQ::Message.new
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
end