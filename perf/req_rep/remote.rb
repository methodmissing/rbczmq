# encoding: utf-8

ctx = ZMQ::Context.new
rep = ctx.socket(:REP);
rep.bind($runner.endpoint);

start_time = Time.now
$runner.msg_count.times do
  msg = case $runner.encoding
    when :string
      rep.recv
    when :frame
      rep.recv_frame
    when :message
      rep.recv_message
    end

    case $runner.encoding
      when :string
        rep.send(msg)
      when :frame
        rep.send_frame(msg)
      when :message
        rep.send_message(msg)
      end
end

puts "Sent #{$runner.msg_count} messages in %ss ..." % (Time.now - start_time)