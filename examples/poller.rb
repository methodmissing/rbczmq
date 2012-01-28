# encoding: utf-8

$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'

ctx = ZMQ::Context.new
pub = ctx.bind(:PUB, 'inproc://example.poller')

subscribers = []

poller = ZMQ::Poller.new

5.times do
  sub = ctx.connect(:SUB, 'inproc://example.poller')
  subscribers << sub
  poller.register_readable(sub)
end

puts "[#{subscribers.size}] subscribers registered with the poller"
p subscribers

puts "publisher sends 'test'"
pub.send("test")

puts "poll, timeout 1s"
poller.poll(1)
puts "readable sockets ..."
p poller.readables
puts "writable sockets ..."
p poller.writables

puts "Subscriber sockets can receive without blocking ..."
p subscribers.map{|s| s.recv_nonblock }

ctx.destroy