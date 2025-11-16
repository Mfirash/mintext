#include <iostream>
#include <vector>
#include <string>
#include <cstdio> 
#include <cstdlib> 
#include <ncurses/ncurses.h>
#include "mintextlib.h"

void print(const std::string& str) { // im not dealing with std::cout spams in help
    std::cout << str << std::endl;
};

int main(int argc, char *argv[]) {    
    if (argc > 1) {
        filename = argv[1];
        if (filename == "-help" || filename == "--h") {
            print("\033[1m mintext commands \033[0m");
            print("Ctrl+Q: Quit");
            print("Ctrl+S: Load");
            print("Ctrl+C: Copy");
            print("Ctrl+V: Paste");
            print("PgDn and Enter: New Line");            
            print("End: Save");
            print("Backspace and Delete: Delete a key");
            print("Arrow Keys: Move cursor");
            print("Ctrl+Z: Undo");
            print("Ctrl+Y: Redo");
            print("Ctrl+O: Overwrite");
            print("");
            print("\033[1m Arguments \033[0m"); 
            print("-help or -h: Show command list"); 
            print("-create or --c: Create new file"); 
            return 0;
        } else if (filename == "") // Do nothing if not loading file
        {
        } else if (filename == "-create" || filename == "--c") {
            changefilename(argv[2]);
            createfile();
        } else {
        load_file(filename);
        };
    }
    clear_screen();
    set_raw_mode();
    try {
        render_editor();

        while (running) {
            std::string key = get_key();
            handle_key(key);
            if (running) {
                render_editor();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
    }
    unset_raw_mode();
    clear_screen();
    std::cout << "Exit" << std::endl;
    return 0;
}


