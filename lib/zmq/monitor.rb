# encoding: utf-8

class ZMQ::Monitor
  def on_connected(addr, fd)
  end

  def on_connect_delayed(addr, err)
  end

  def on_connect_retried(addr, interval)
  end

  def on_listening(addr, fd)
  end

  def on_bind_failed(addr, err)
  end

  def on_accepted(addr, fd)
  end

  def on_accept_failed(addr, err)
  end

  def on_closed(addr, fd)
  end

  def on_close_failed(addr, err)
  end

  def on_disconnected(addr, fd)
  end
end