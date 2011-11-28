# encoding: utf-8

require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

libs_path = File.expand_path(File.dirname(__FILE__))
vendor_path = File.join(File.dirname(__FILE__), '..')
zmq_include_path = File.join(vendor_path, 'zeromq', 'include')
czmq_include_path = File.join(vendor_path, 'czmq', 'include')

dir_config('rbczmq')

have_func('rb_thread_blocking_region')

$INCFLAGS << " -I#{zmq_include_path}" if find_header("zmq.h", zmq_include_path)
$INCFLAGS << " -I#{czmq_include_path}" if find_header("czmq.h", czmq_include_path)

find_library("zmq", "zmq_init", libs_path)
find_library("czmq", "zctx_new", libs_path)
$LDFLAGS << " -L#{libs_path}"

$defs << "-pedantic"

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

create_makefile('rbczmq_ext')