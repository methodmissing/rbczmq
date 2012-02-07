# encoding: utf-8

ctx = ZMQ::Context.new
sub = ctx.socket(:SUB)
sub.subscribe("")
sub.connect(Runner::ENDPOINT)

messages, start_time = 0, nil
while (case Runner.encoding
  when :string
    sub.recv
  when :frame
    sub.recv_frame
  when :message
    sub.recv_message
  end) do
  start_time ||= Time.now
  messages += 1
  break if messages == Runner.msg_count
end

Runner.stats(start_time)