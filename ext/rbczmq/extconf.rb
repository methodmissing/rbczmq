# encoding: utf-8

require 'mkmf'
require 'fileutils'
require 'pathname'
include FileUtils

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

# XXX fallbacks specific to Darwin for JRuby (does not set these values in RbConfig::CONFIG)
LIBEXT = RbConfig::CONFIG['LIBEXT'] || 'a'
DLEXT = RbConfig::CONFIG['DLEXT'] || 'bundle'

libs_path = Pathname(File.expand_path(File.dirname(__FILE__)))
vendor_path = libs_path + '..'
zmq_path = vendor_path + 'zeromq'
czmq_path = vendor_path + 'czmq'
zmq_include_path = zmq_path + 'include'
czmq_include_path = czmq_path + 'include'

# extract dependencies
unless File.directory?(zmq_path) && File.directory?(czmq_path)
  if `which tar`.strip.empty?
    puts "The 'tar' (creates and manipulates streaming archive files) utility is required to extract dependencies"
    exit(1)
  end
  Dir.chdir(vendor_path) do
    system "tar xvzf zeromq.tar.gz"
    system "tar xvzf czmq.tar.gz"
  end
end

# build libzmq
lib = File.join(zmq_path, 'src', '.libs', "libzmq.#{LIBEXT}")
Dir.chdir zmq_path do
  system "./autogen.sh" unless File.exist?(zmq_path + 'configure')
  system "./configure && make"
end unless File.exist?(lib)
cp lib, libs_path

# build libczmq
lib = File.join(czmq_path, 'src', '.libs', "libczmq.#{LIBEXT}")
Dir.chdir czmq_path do
  system "./autogen.sh" unless File.exist?(czmq_path + 'configure')
  system "./configure --with-libzmq=#{zmq_path} && make all"
end unless File.exist?(lib)
cp lib, libs_path

dir_config('rbczmq')

have_func('rb_thread_blocking_region')

$INCFLAGS << " -I#{zmq_include_path}" if find_header("zmq.h", zmq_include_path)
$INCFLAGS << " -I#{czmq_include_path}" if find_header("czmq.h", czmq_include_path)

find_library("zmq", "zmq_init", libs_path.to_s)
find_library("czmq", "zctx_new", libs_path.to_s)
$LDFLAGS = "-L. -L#{libs_path}"

$defs << "-pedantic"

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

create_makefile('rbczmq_ext')