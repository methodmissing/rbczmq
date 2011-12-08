# encoding: utf-8

ctx = ZMQ::Context.new
rep = ctx.socket(:REP);
rep.bind(Runner::REMOTE_ENDPOINT);

Runner.msg_count.times do
  msg = case Runner.encoding
    when :string
      rep.recv
    when :frame
      rep.recv_frame
    when :message
      rep.recv_message
    end

    case Runner.encoding
      when :string
        rep.send(msg)
      when :frame
        rep.send_frame(msg)
      when :message
        rep.send_message(msg)
      end
end