#include <iostream>
#include "mach_exc_handlers.h"

using namespace std;

extern "C" kern_return_t catch_mach_exception_raise(
  mach_port_t exception_port,
  mach_port_t thread_port,
  mach_port_t task_port,
  exception_type_t exception_type,
  mach_exception_data_t codes,
  mach_msg_type_number_t num_codes)
{
  cout << "i am in the exception handler" << endl;
  if (exception_type == EXC_SOFTWARE && codes[0] == EXC_SOFT_SIGNAL) {
    if (codes[2] == SIGSTOP)
      codes[2] = 0;

    ptrace(PT_THUPDATE, 0, (caddr_t)(uintptr_t)thread_port, codes[2]);
  }
  return KERN_SUCCESS;
}

extern "C" kern_return_t catch_mach_exception_raise_state(
  mach_port_t exception_port,
  exception_type_t exception,
  const mach_exception_data_t code,
  mach_msg_type_number_t codeCnt,
  int* flavor,
  const thread_state_t old_state,
  mach_msg_type_number_t old_stateCnt,
  thread_state_t new_state,
  mach_msg_type_number_t* new_stateCnt)
{
  return MACH_RCV_INVALID_TYPE;
}

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
  mach_msg_type_number_t* new_stateCnt)
{
  return MACH_RCV_INVALID_TYPE;
}