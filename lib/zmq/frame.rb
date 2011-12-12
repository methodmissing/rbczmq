# encoding: utf-8

class ZMQ::Frame

  # The ZMQ::Frame class provides methods to send and receive single message rames across 0MQ sockets. When reading a frame
  # from a socket, the ZMQ::Frame#more? method indicates if the frame is part of an unfinished multipart message. The
  # ZMQ::Socket#send_frame method normally destroys the frame, but with the ZFRAME_REUSE flag, you can send the same frame
  # many times. Frames are binary, and this class has no special support for text data.

  include Comparable
end