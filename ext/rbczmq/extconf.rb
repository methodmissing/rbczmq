# encoding: utf-8

require 'mkmf'
require 'pathname'

def sys(cmd, err_msg)
  system(cmd) || fail(err_msg)
end

def fail(msg)
  STDERR.puts msg
  exit(1)
end

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

# XXX fallbacks specific to Darwin for JRuby (does not set these values in RbConfig::CONFIG)
LIBEXT = RbConfig::CONFIG['LIBEXT'] || 'a'
DLEXT = RbConfig::CONFIG['DLEXT'] || 'bundle'

cwd = Pathname(File.expand_path(File.dirname(__FILE__)))
dst_path = cwd + 'dst'
libs_path = dst_path + 'lib'
vendor_path = cwd + '..'
zmq_path = vendor_path + 'zeromq'
czmq_path = vendor_path + 'czmq'
zmq_include_path = zmq_path + 'include'
czmq_include_path = czmq_path + 'include'

# extract dependencies
unless File.directory?(zmq_path) && File.directory?(czmq_path)
  fail "The 'tar' (creates and manipulates streaming archive files) utility is required to extract dependencies" if `which tar`.strip.empty?
  Dir.chdir(vendor_path) do
    sys "tar xvzf zeromq.tar.gz", "Could not extract the ZeroMQ archive!"
    sys "tar xvzf czmq.tar.gz", "Could not extract the CZMQ archive!"
  end
end

# build libzmq
lib = libs_path + "libzmq.#{LIBEXT}"
Dir.chdir zmq_path do
  sys "./autogen.sh", "ZeroMQ autogen failed!" unless File.exist?(zmq_path + 'configure')
  sys "./configure --prefix=#{dst_path} --enable-shared && make && make install", "ZeroMQ compile error!"
end unless File.exist?(lib)

# build libczmq
lib = libs_path + "libczmq.#{LIBEXT}"
Dir.chdir czmq_path do
  sys "./autogen.sh", "CZMQ autogen failed!" unless File.exist?(czmq_path + 'configure')
  sys "./configure --prefix=#{dst_path} --with-libzmq=#{dst_path} --disable-shared && make all && make install", "CZMQ compile error!"
end unless File.exist?(lib)

dir_config('rbczmq')

have_func('rb_thread_blocking_region')

$INCFLAGS << " -I#{zmq_include_path}" if find_header("zmq.h", zmq_include_path)
$INCFLAGS << " -I#{czmq_include_path}" if find_header("czmq.h", czmq_include_path)

if RUBY_VERSION =~ /1.9.\d/
  $LDFLAGS = "-L. -L#{libs_path}"
else
  $LDFLAGS << " -L#{libs_path}"
end

fail "Error compiling and linking libzmq" unless have_library("zmq")
fail "Error compiling and linking libczmq" unless have_library("czmq")

$defs << "-pedantic"

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

create_makefile('rbczmq_ext')