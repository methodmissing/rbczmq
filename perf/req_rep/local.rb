# encoding: utf-8

ctx = ZMQ::Context.new
req = ctx.socket(:REQ)
req.connect(Runner::REMOTE_ENDPOINT)

msg = Runner.payload

messages, start_time = 0, nil
while (case Runner.encoding
       when :string
         req.send(msg)
       when :frame
         req.send_frame(ZMQ::Frame(msg))
       when :message
         m =  ZMQ::Message.new
         m.pushstr "header"
         m.pushstr msg
         m.pushstr "body"
         req.send_message(m)
       end) do
  start_time ||= Time.now
  messages += 1
  case Runner.encoding
    when :string
      req.recv
    when :frame
      req.recv_frame
    when :message
      req.recv_message
    end
  break if messages == Runner.msg_count
end

Runner.stats(start_time)