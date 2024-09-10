#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ncurses.h>

using namespace std;

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

    // clear the screen and print the user's choice
    clear();
    mvprintw(0, 0, "You chose: %s", choices[choice].c_str());
    refresh();
    getch();

    if (choice == 0) {
      const vector<string> choices = { "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "float", "double" };
      switch (get_choice(choices, "Argument #1 Type:")) {
        case 0:
          int8_t val;
          cin >> val;
          break;
        case 1:
          break;
        case 2:
          break;
        case 3:
          break;
        case 4:
          break;
        case 5:
          break;
        case 6:
          break;
        case 7:
          break;
        case 8:
          break;
        case 9:
          break;
        default:
          break;
      }

      string value;
      while (true) {
        cin >> value;

        if (cin.fail()) {
          cin.clear();
          cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
          cout << "Invalid input. Please enter a valid unsigned integer: ";
        } else {
          cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear any remaining input
          break;
        }
      }

      clear();
      mvprintw(0, 0, "You chose: %s", choices[choice].c_str());
      refresh();
    } else {

    }

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
