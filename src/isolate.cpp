#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <sstream>
#include <cstdint>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#endif

using namespace std;

const vector<string> g_argument_type_tags = { "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "float", "double" };

struct ArgumentType {
  int type_tag_idx = 0;

  union {
    int8_t i8_data;
    int16_t i16_data;
    int32_t i32_data;
    int64_t i64_data;
    uint8_t u8_data;
    uint16_t u16_data;
    uint32_t u32_data;
    uint64_t u64_data;
    float float_data;
    double double_data;
  } data;

  ArgumentType(int idx) : type_tag_idx(idx) {}
  ArgumentType() = delete;
  ~ArgumentType() {}
};

/**
 * @brief prints program usage
 * @param prog_name the name of the program
 */
void print_usage(char* prog_name) {
  cerr << "Usage: " << prog_name << " --binary /path/to/binary --function-address address\n";
}

/**
 * @brief prints registers of provided thread via mach ports
 * @param thread the thread whose registers to print
 * @return Mach error code (or KERN_SUCCESS)
 */
int print_registers(mach_port_t thread) {
  arm_thread_state64_t state;
  mach_msg_type_number_t state_count = ARM_THREAD_STATE64_COUNT;
  kern_return_t ret = thread_get_state(thread, ARM_THREAD_STATE64, (thread_state_t)&state, &state_count);
  if (ret != KERN_SUCCESS)
    return ret;

  cout << "pc: " << hex << state.__pc << endl;
  cout << "sp: " << hex << state.__sp << endl;
  for (int i = 0; i < 29; i++)
    cout << "x" << dec << i << ": " << hex << state.__x[i] << endl;
  return KERN_SUCCESS;
}

/**
 * @brief prompts the user for a choice on the terminal
 * @param choices a vector of choices
 * @param prompt the prompt to display to the user
 * @return an index into choices @p choices representing the user's choice
 */
int get_choice(const vector<string>& choices, const string& prompt) {
  int choice = 0;
  int highlight = 0;

  while (true) {
    clear();
    mvprintw(0, 1, prompt.c_str());
    for (int i = 0; i < choices.size(); i++) {
      if (i == highlight) {
        attron(A_REVERSE);
        mvprintw(i + 1, 1, choices[i].c_str());
        attroff(A_REVERSE);
      } else {
        mvprintw(i + 1, 1, choices[i].c_str());
      }
    }

    // Get user input
    int c = getch();
    switch (c) {
      case KEY_UP:
        highlight--;
        if (highlight < 0) {
          highlight = choices.size() - 1;
        }
        break;
      case KEY_DOWN:
        highlight++;
        if (highlight >= choices.size()) {
          highlight = 0;
        }
        break;
      case '\n':
        choice = highlight;
        break;
      default:
        break;
    }

    if (c == '\n')
      break;
  }

  return choice;
}

int main(int argc, char* argv[]) {
  // get the TERM env var
  char* term_str = nullptr;
  {
    string t("TERM=");
    t += getenv("TERM");
    int len = strlen(t.c_str()) + 1;

    term_str = (char*)malloc(len);
    if (term_str == nullptr) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }

    strncpy(term_str, t.c_str(), len - 1);
    term_str[len - 1] = '\0';
  }

  // parse command line args
  char* binary_path = nullptr;
  uint64_t function_addr = 0;
  {
    static struct option long_options[] = {
        {"binary", required_argument, 0, 'b'},
        {"function-address", required_argument, 0, 'f'},
        {"", optional_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "b:f:", long_options, NULL)) != -1) {
      switch (opt) {
        case 'b':
          binary_path = optarg;
          break;
        case 'f':
          function_addr = strtoul(optarg, nullptr, 0);
          if (!function_addr) {
            cout << "Address must be a number\n";
            exit(EXIT_FAILURE);
          } else if (function_addr == ULONG_MAX && errno == ERANGE) {
            cout << "Address doesn't fit in 8 bytes\n";
            exit(EXIT_FAILURE);
          }
          break;
        case '?':
          print_usage(argv[0]);
          exit(EXIT_FAILURE);
      }
    }

    if (binary_path == nullptr || !function_addr) {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // ask user for arguments
  {
    list<uint64_t> args;

    initscr();
    clear();
    noecho();
    cbreak();  // disable line buffering
    curs_set(0); // hide the cursor
    keypad(stdscr, TRUE); // enable arrow keys

    bool done = 1 - get_choice({"No", "Yes"}, "Any arguments? ");
    int current_arg_num = 0;
    vector<ArgumentType> arguments;

    while (!done) {
      current_arg_num++;

      const vector<string> choices = {
        "Primitive",
        "Complex", // string, struct/class/raw memory
      };

      // Compute the prompt string for current argument type
      char* curr_arg_type_prompt;
      {
        int sz = snprintf(nullptr, 0, "Argument #%d Type:  ", current_arg_num);
        curr_arg_type_prompt = new char[sz + 1];
        snprintf(curr_arg_type_prompt, sz, "Argument #%d Type:  ", current_arg_num);
      }

      // Compute the prompt string for current argument value
      char* curr_arg_value_prompt;
      {
        int sz = snprintf(nullptr, 0, "Argument #%d Value:  ", current_arg_num);
        curr_arg_value_prompt = new char[sz + 1];
        snprintf(curr_arg_value_prompt, sz, "Argument #%d Value:  ", current_arg_num);
      }

      int choice = get_choice(choices, curr_arg_type_prompt);
      if (choice == 0) {
        // Primitive arg type
        ArgumentType arg(get_choice(g_argument_type_tags, curr_arg_type_prompt));

        string arg_value;
        while (true) { // while arg value not parsed
          while (true) { // user input loop
            clear();

            mvprintw(0, 1, curr_arg_value_prompt);
            attron(A_REVERSE);
            mvprintw(0, 1 + strlen(curr_arg_value_prompt), "%s", arg_value.c_str());
            attroff(A_REVERSE);

            int c = getch();
            if (c == '\n')
              break;

            if ((c == KEY_BACKSPACE || c == 127 || c == 8) && arg_value.length() > 0) {
              arg_value.pop_back();
            } else if (isnumber(c) || c == '.') {
              arg_value.push_back(c);
            }

            refresh();
          }

          try {
            switch (arg.type_tag_idx) {
              case 0: {
                int tmp = stoi(arg_value);
                if (tmp >= INT8_MIN && tmp <= INT8_MAX)
                  arg.data.i8_data = tmp;
                else
                  throw invalid_argument("Value outside of int8_t bounds. Try again.");
                break;
              }
              case 1: {
                int tmp = stoi(arg_value);
                if (tmp >= INT16_MIN && tmp <= INT16_MAX)
                  arg.data.i16_data = tmp;
                else
                  throw invalid_argument("Value outside of int16_t bounds. Try again.");
                break;
              }
              case 2: {
                int tmp = stoi(arg_value);
                if (tmp >= INT32_MIN && tmp <= INT32_MAX)
                  arg.data.i32_data = tmp;
                else
                  throw invalid_argument("Value outside of int32_t bounds. Try again.");
                break;
              }
              case 3: {
                long tmp = stoll(arg_value);
                if (tmp >= INT64_MIN && tmp <= INT64_MAX)
                  arg.data.i64_data = tmp;
                else
                  throw invalid_argument("Value outside of int64_t bounds. Try again.");
                break;
              }
              case 4: {
                unsigned long tmp = stoul(arg_value);
                if (tmp >= 0 && tmp <= UINT8_MAX)
                  arg.data.u8_data = tmp;
                else
                  throw invalid_argument("Value outside of uint8_t bounds. Try again.");
                break;
              }
              case 5: {
                unsigned long tmp = stoul(arg_value);
                if (tmp >= 0 && tmp <= UINT16_MAX)
                  arg.data.u16_data = tmp;
                else
                  throw invalid_argument("Value outside of uint16_t bounds. Try again.");
                break;
              }
              case 6: {
                unsigned long tmp = stoull(arg_value);
                if (tmp >= 0 && tmp <= UINT32_MAX)
                  arg.data.u32_data = tmp;
                else
                  throw invalid_argument("Value outside of uint32_t bounds. Try again.");
                break;
              }
              case 7:
                arg.data.u64_data = stoull(arg_value);
                break;
              case 8:
                arg.data.float_data = stof(arg_value);
                break;
              case 9:
                arg.data.double_data = stod(arg_value);
                break;
              default:
                endwin();
                cerr << "Invalid argument type" << endl;
                exit(EXIT_FAILURE);
                break;
            }
          } catch (const exception& e) {
            clear();
            printw(e.what());
            refresh();
            getch();
            continue;
          }

          arguments.push_back(arg);
          break;
        }
      } else {

      }

      clear();
      refresh();

      done = get_choice({"No", "Yes"}, "Done? ");

      delete[] curr_arg_type_prompt;
      delete[] curr_arg_value_prompt;
    }

    endwin();
  }

  // launch the debugger
  pid_t target_pid = fork();
  if (target_pid == 0) {
    ptrace(PT_TRACE_ME, 0, NULL, 0);
    raise(SIGSTOP);
    cout << "[child]: after SIGSTOP, before execve" << endl;

    // allocate space for structs/classes, and set regs to correct values
    const char* argv[] = { binary_path, NULL };
    const char* envp[] = { term_str, NULL };
    if (execve(binary_path, const_cast<char* const*>(argv), const_cast<char* const*>(envp)) < 0) {
      perror("execve");
      exit(EXIT_FAILURE);
    }
  } else {
    // get the task port
    mach_port_t target_task_port = 0;
    {
      kern_return_t ret = task_for_pid(mach_task_self(), target_pid, &target_task_port);
      if (ret != KERN_SUCCESS) {
        printf("task_for_pid failed: %s\n", mach_error_string(ret));
        return 0;
      } else {
        printf("target_pid = %u\n", target_task_port);
      }
    }
        
    // save exception ports
    exception_mask_t saved_masks[EXC_TYPES_COUNT];
    mach_port_t saved_ports[EXC_TYPES_COUNT];
    exception_behavior_t saved_behaviors[EXC_TYPES_COUNT];
    thread_state_flavor_t saved_flavors[EXC_TYPES_COUNT];
    mach_msg_type_number_t saved_exception_types_count;
    task_get_exception_ports(target_task_port, EXC_MASK_ALL, saved_masks, &saved_exception_types_count, saved_ports, saved_behaviors, saved_flavors);

    // attach to child after SIGSTOP
    int status;
    waitpid(target_pid, &status, 0);
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
      cout << "[parent]: child stopped, attaching now" << endl;
    }

    mach_port_name_t target_exception_port;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &target_exception_port);
    mach_port_insert_right(mach_task_self(), target_exception_port, target_exception_port, MACH_MSG_TYPE_MAKE_SEND);
    task_set_exception_ports(target_task_port, EXC_MASK_ALL, target_exception_port, EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, THREAD_STATE_NONE);

    ptrace(PT_ATTACHEXC, target_pid, 0, 0);
    ptrace(PT_CONTINUE, target_pid, (caddr_t)1, 0);

    waitpid(target_pid, &status, 0);
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
      cout << "[parent]: SIGTRAP received" << endl;
    }

    if (kill(target_pid, SIGSTOP) == -1)
      perror("kill");
    ptrace(PT_DETACH, target_pid, 0, 0);

    thread_act_array_t thread_list;
    {
      mach_msg_type_number_t thread_count;
      kern_return_t ret = task_threads(target_task_port, &thread_list, &thread_count);
      if (ret != KERN_SUCCESS) {
        cerr << "failed to get task threads: " << mach_error_string(ret) << endl;
        exit(EXIT_FAILURE);
      }

      if (thread_count == 0) {
        cerr << "no threads in task!" << endl;
        exit(EXIT_FAILURE);
      }

      arm_thread_state64_t state;
      mach_msg_type_number_t state_count = ARM_THREAD_STATE64_COUNT;
      ret = thread_get_state(thread_list[0], ARM_THREAD_STATE64, (thread_state_t)&state, &state_count);
      if (ret != KERN_SUCCESS) {
        cerr << "failed to get thread state" << endl;
        exit(EXIT_FAILURE);
      }

      // state.__pc = (ino_t)function_addr;
      ret = thread_set_state(thread_list[0], ARM_THREAD_STATE64, (thread_state_t)&state, state_count);
      if (ret != KERN_SUCCESS) {
        cerr << mach_error_string(ret) << endl;
        exit(EXIT_FAILURE);
      }

      ret = print_registers(thread_list[0]);
      if (ret != KERN_SUCCESS) {
        cerr << mach_error_string(ret) << endl;
        exit(EXIT_FAILURE);
      }

      // TODO: handle ASLR
      // set the first byte of the function to brk #0x1
      {
        // save old instruction
        void* orig_inst;
        mach_msg_type_number_t data_count = 4;
        kern_return_t kr = vm_read(target_task_port, (vm_address_t)function_addr, (vm_size_t)4, (vm_offset_t*)&orig_inst, &data_count);
        if (kr != KERN_SUCCESS) {
          cerr << "vm_read failed: " << mach_error_string(kr) << endl;
          exit(EXIT_FAILURE);
        }
        cout << "original instruction:" << endl;
        for (int i = 0; i < 4; i++) {
          cout << hex << ((char*)orig_inst)[i] << endl;
        }

        mach_vm_size_t page_size = getpagesize();
        vm_address_t brk_addr = 0;
        kr = vm_allocate(mach_task_self(), &brk_addr, page_size, VM_FLAGS_ANYWHERE);

        if (kr != KERN_SUCCESS) {
          cerr << "vm_allocate failed: " << mach_error_string(kr) << endl;
          exit(EXIT_FAILURE);
        }

        *(char*)brk_addr = 0x20;
        *(((char*)brk_addr) + 1) = 0x00;
        *(((char*)brk_addr) + 2) = 0x20;
        *(((char*)brk_addr) + 3) = 0xd4;
        vm_write(target_task_port, (vm_address_t)function_addr, brk_addr, 4);

        kr = vm_deallocate(mach_task_self(), brk_addr, page_size);
        if (kr != KERN_SUCCESS) {
          cerr << "vm_deallocate failed: " << mach_error_string(kr) << endl;
          exit(EXIT_FAILURE);
        }
      }
    }

    // restore original exception ports
    for (uint32_t i = 0; i < saved_exception_types_count; ++i)
      task_set_exception_ports(target_task_port, saved_masks[i], saved_ports[i], saved_behaviors[i], saved_flavors[i]);
    mach_port_deallocate(mach_task_self(), target_exception_port);

    kill(target_pid, SIGKILL);
  }

  return 0;
}
