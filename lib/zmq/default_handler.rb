# encoding: utf-8

class ZMQ::DefaultHandler < ZMQ::Handler

  # A default / blanket pollitem callback handler for when a socket or IO is registered on a ZMQ::Loop instance.
  #
  # XXX: Likely a massive fail for some socket / IO pairs as a default - look into removing this.

  def on_readable
    p recv
  end

  def on_writable
    send("")
  end
end