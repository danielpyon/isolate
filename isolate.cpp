#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <sstream>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ncurses.h>

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

    const vector<string> choices = {
      "Primitive",
      "Complex", // string, struct/class/raw memory
    };

    int choice = get_choice(choices, "Argument #1 Type:");

    vector<ArgumentType> arguments;
    if (choice == 0) {
      ArgumentType arg(get_choice(g_argument_type_tags, "Argument #1 Type:"));

      const char* arg_value_dialog = "Argument #1 Value: ";
      string arg_value;

get_arg_value:
      while (true) {
        clear();

        mvprintw(0, 1, arg_value_dialog);
        attron(A_REVERSE);
        mvprintw(0, 1 + strlen(arg_value_dialog), "%s", arg_value.c_str());
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
          case 0:
            int tmp = stoi(arg_value);
            if (tmp >= -128 && tmp <= 127)
              arg.data.i8_data = tmp;
            else
              throw invalid_argument("value outside of int8_t bounds");
            break;
          case 1:
            int tmp = stoi(arg_value);
            if (tmp >= -65536 && tmp <= 65535)
              arg.data.i8_data = tmp;
            else
              throw invalid_argument("value outside of int16_t bounds");
            break;
          case 2:
            arg.data.i32_data = stoi(arg_value);
            break;
          case 3:
            ss >> arg.data.i64_data;
            break;
          case 4:
            ss >> arg.data.u8_data;
            break;
          case 5:
            ss >> arg.data.u16_data;
            break;
          case 6:
            ss >> arg.data.u32_data;
            break;
          case 7:
            ss >> arg.data.u64_data;
            break;
          case 8:
            ss >> arg.data.float_data;
            break;
          case 9:
            ss >> arg.data.double_data;
            break;
          default:
            cerr << "Invalid argument type\n";
            fflush(stderr);
            exit(EXIT_FAILURE);
            break;
        }
      } catch (const invalid_argument& ia) {

      } catch (const out_of_range& oor) {
        
      }

      if (ss.fail()) {
        clear();
        printw("Invalid format! Press Enter to try again");
        refresh();
        getch();
        goto get_arg_value;
      } else {
        arguments.push_back(arg);
      }
    } else {

    }

    clear();
    refresh();

    getch(); // wait for user input
    endwin(); // end curses mode
  }

  /*
  // launch the debugger
  {
    // allocate space for structs/classes, and set regs to correct values

    char* argv[] = { "lldb", NULL };
    char* envp[] = { term_cstr, NULL };

    // TODO: multiarch support
    if (execve("/usr/bin/lldb", argv, envp) < 0) {
      perror("execve");
    }
  }
  */

  return 1;
}
