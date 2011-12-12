# encoding: utf-8

class ZMQ::Timer

  # Callback for error conditions such as exceptions raised in timer callbacks. Receives an exception instance as argument and raises by default.
  #
  # handler.on_error(err)  =>  raise
  #
  def on_error(exception)
    raise exception
  end
end