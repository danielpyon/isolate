#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ncurses.h>

using namespace std;

void print_usage(char* prog_name) {
  cerr << "Usage: " << prog_name << " --binary /path/to/binary --function-address address\n";
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
    /**
     * first, ask for the type of current argument (multiple choice)
     * then, there are two cases:
     * 1) primitives: directly input the char, string, int, float, etc
     * 2) pointers to structs/classes: hexedit arbitrary offsets, nestably
     *   - each offset might contain a primitive, or another pointer to struct/class/vtable
     * TODO: since this is annoying to do, maybe allow user to input a file with this information
     */

    initscr();
    clear();
    noecho();
    cbreak();  // disable line buffering
    curs_set(0); // hide the cursor

    // Enable arrow keys
    keypad(stdscr, TRUE);

    // Define options
    const vector<string> options = {
      "Primitive",
      "Composite"
    };

    int choice = 0;
    while (true) {
      clear();
      mvprintw(0, 1, "Argument Type #1:");

      int highlight = 0;
      for (int i = 0; i < options.size(); ++i) {
        if (i == highlight) {
          attron(A_REVERSE);
          mvprintw(i + 1, 1, options[i].c_str());
          attroff(A_REVERSE);
        } else {
          mvprintw(i + 1, 1, options[i].c_str());
        }
      }

      // Get user input
      int c = getch();
      switch (c) {
        case KEY_UP:
          highlight--;
          if (highlight < 0) {
            highlight = options.size() - 1; // wrap around to bottom
          }
          break;
        case KEY_DOWN:
          highlight++;
          if (highlight >= options.size()) {
            highlight = 0; // wrap around to top
          }
          break;
        case '\n': // enter key
          choice = highlight;
          break;
        default:
          break;
      }

      // If the user presses Enter, exit the loop
      if (c == '\n')
        break;
    }

    // clear the screen and print the user's choice
    clear();
    mvprintw(0, 0, "You chose: %s", options[choice].c_str());
    refresh();

    getch();  // wait for user input

    // end curses mode
    endwin();
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
