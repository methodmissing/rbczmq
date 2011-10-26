# encoding: utf-8

class ZMQ::Timer
  def on_error(exception)
    raise exception
  end
end