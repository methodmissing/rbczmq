# encoding: utf-8

ctx = ZMQ::Context.new
pair = ctx.socket(:PAIR)
pair.connect(Runner::ENDPOINT)

messages, start_time = 0, nil
while (case Runner.encoding
  when :string
    pair.recv
  when :frame
    pair.recv_frame
  when :message
    pair.recv_message
  end) do
  start_time ||= Time.now
  messages += 1
  break if messages == Runner.msg_count
end

Runner.stats(start_time)