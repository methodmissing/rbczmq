# zmq_logger.rb
#
# Logging via ZMQ push/pull sockets.

require 'rbczmq'
require 'logger'

module ZMQ

  # A logging class that sends its output to a ZMQ socket. This allows log messages to be sent
  # asynchronously to another process and across a network if required. The receiving socket
  # will receive messages, each with text of one log message.
  #
  # Example usage:
  #
  #   socket = context.socket(ZMQ::PUSH)
  #   socket.connect("tcp://logserver:5555")
  #   logger = ZMQ::Logger.new(socket)
  #   logger.debug("Hello logger")
  #
  class Logger < ::Logger

    class InvalidSocketError < ZMQ::Error ; end

    # only these socket classes are allowed to be used for sending
    VALID_SOCKET_CLASSES = [
      Socket::Pub,
      Socket::Push,
      Socket::Pair
    ]

    # initialise a new log object. The logger sends log messages to a push socket.
    # the caller should connect the push socket to pull socket where the log messages
    # will be processed. socket should be a ZMQ::Socket::Push or ZMQ::Socket::Pub socket
    def initialize(socket)
      raise InvalidSocketError unless VALID_SOCKET_CLASSES.any? { |klass| socket.is_a?(klass) }
      super(nil)
      @logdev = LogDevice.new(socket)
    end

    # implements an interface similar to ::Logger::LogDevice. This is the recipient of the
    # formatted log messages
    class LogDevice
      # :nodoc:
      def initialize(socket)
        @socket = socket
      end

      # write the formatted log message to the socket.
      def write(message)
        @socket.send(message)
      end

      def close
      end
    end

  end
end
