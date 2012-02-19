# encoding: utf-8

ctx = ZMQ.context
pair = ctx.socket(:PAIR);
pair.bind($runner.endpoint);

msg = $runner.payload

start_time = Time.now
$runner.msg_count.times do
  case $runner.encoding
  when :string
    pair.send(msg)
  when :frame
    pair.send_frame(ZMQ::Frame(msg))
  when :message
    m =  ZMQ::Message.new
    m.pushstr "header"
    m.pushstr msg
    m.pushstr "body"
    pair.send_message(m)
  end
end

puts "Sent #{$runner.msg_count} messages in %ss ..." % (Time.now - start_time)