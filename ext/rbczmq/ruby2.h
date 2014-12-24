#ifndef RBCZMQ_RUBY2_H
#define RBCZMQ_RUBY2_H

#ifndef HAVE_RB_THREAD_CALL_WITHOUT_GVL
#define rb_thread_call_without_gvl rb_thread_blocking_region
#endif

#endif
