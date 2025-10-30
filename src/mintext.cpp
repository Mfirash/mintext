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
            print("Minimal text editor commands");
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
            return 0;
        } else if (filename == "") // Do nothing if not loading file
        {
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
    std::cout << "Exited text editor." << std::endl;
    std::cout << "\n---Content---" << std::endl;
    for (const auto& line : lines) {
        std::cout << line << std::endl;
    }
    std::cout << "-------------\n" << std::endl;

    return 0;
}


