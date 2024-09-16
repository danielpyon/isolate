#include <stdlib.h>
#include <sys/ptrace.h>

extern "C" {
  #include "mach_exc.h"
}

extern "C" boolean_t mach_exc_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

extern "C" kern_return_t catch_mach_exception_raise(
  mach_port_t exception_port,
  mach_port_t thread_port,
  mach_port_t task_port,
  exception_type_t exception_type,
  mach_exception_data_t codes,
  mach_msg_type_number_t num_codes);

extern "C" kern_return_t catch_mach_exception_raise_state(
  mach_port_t exception_port,
  exception_type_t exception,
  const mach_exception_data_t code,
  mach_msg_type_number_t codeCnt,
  int* flavor,
  const thread_state_t old_state,
  mach_msg_type_number_t old_stateCnt,
  thread_state_t new_state,
  mach_msg_type_number_t* new_stateCnt);

extern "C" kern_return_t catch_mach_exception_raise_state_identity(
  mach_port_t exception_port,
  mach_port_t thread,
  mach_port_t task,
  exception_type_t exception,
  mach_exception_data_t code,
  mach_msg_type_number_t codeCnt,
  int* flavor,
  thread_state_t old_state,
  mach_msg_type_number_t old_stateCnt,
  thread_state_t new_state,
  mach_msg_type_number_t* new_stateCnt);
