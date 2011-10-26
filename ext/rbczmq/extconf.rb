# encoding: utf-8

require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

dir_config('rbczmq')

$defs << "-DRUBY_VM" if have_func('rb_thread_blocking_region')
find_library("zmq", "zmq_init")
find_library("czmq", "zctx_new")
$defs << "-pedantic"

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

create_makefile('rbczmq_ext')