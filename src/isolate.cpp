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

#if defined(__APPLE__)
#include <mach/mach.h>
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
 */
void print_usage(char* prog_name) {
  cerr << "Usage: " << prog_name << " --binary /path/to/binary --function-address address\n";
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
  mach_port_t task = 0;
  long int pid = 0;
  cin >> pid;

  kern_return_t ret = task_for_pid(mach_task_self(), pid, &task);
  if (ret != KERN_SUCCESS) {
    printf("task_for_pid failed: %s\n", mach_error_string(ret));
    return 0;
  }
  printf("%u\n", task);
  return 0;

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

    bool done = false;
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

      {
        vector<string> choices { "No", "Yes" };
        done = get_choice(choices, "Done? ");
      }

      delete[] curr_arg_type_prompt;
      delete[] curr_arg_value_prompt;
    }

    endwin();

    for (const auto& x : arguments) {
      cout << g_argument_type_tags[x.type_tag_idx] << endl;
    }
  }

  // launch the debugger
  // TODO: remove conditional
  if (1 == 2) {
    // allocate space for structs/classes, and set regs to correct values
    const char* argv[] = { "lldb", NULL };
    const char* envp[] = { term_str, NULL };

    // TODO: multiarch support
    if (execve("/usr/bin/lldb", const_cast<char* const*>(argv), const_cast<char* const*>(envp)) < 0) {
      perror("execve");
    }
  }

  return 1;
}
