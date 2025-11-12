#include <iostream>
#include <limits>
#include <vector>
#include <string>
#include <cstdio> 
#include <cstdlib> 
#include <algorithm>
#include <ncurses/ncurses.h>
#include "mintextlib.h"

// Global variables
std::vector<std::string> lines = {""};
int cursor_y = 0;
int cursor_x = 0;
std::string clipboard = "";
bool running = true;
std::string filename = "untitled.txt";
std::vector<std::vector<std::string>> undo_history;
std::vector<std::vector<std::string>> redo_history;
const size_t MAX_HISTORY = 50;

void clear_screen() {
    clear();
}

void set_raw_mode() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);    
    curs_set(1);
}

void unset_raw_mode() {
    endwin();
}
void statusbar() { 
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    attron(A_REVERSE);
    move(max_y - 1,0);
    printw("'");
    printw(filename.c_str());
    printw("' X: '%d' Y: '%d' Ln: '%zu'", cursor_x, cursor_y, lines.size());

    for(int i = 0; i < max_x - (filename.length() + 18); ++i) {
        printw(" ");    
    }
    attroff(A_REVERSE);
    refresh();
} 

void set_window_name(const std::string& new_title) {
    std::cout << "\033]0;" << new_title << "\007" << std::flush;
}

std::string get_key() {
    int key = getch();
    
    if (key == KEY_UP) return "UP";
    else if (key == KEY_DOWN) return "DOWN";
    else if (key == KEY_LEFT) return "LEFT";
    else if (key == KEY_RIGHT) return "RIGHT";
    else if (key == KEY_DC) return "DEL";
    else if (key == KEY_NPAGE) return "PGDN";
    else if (key == KEY_END) return "END";
    
    else if (key == 3) return "CTRL_C"; 
    else if (key == 22) return "CTRL_V"; 
    else if (key == 17) return "CTRL_Q"; 
    else if (key == 8) return "BACKSPACE";
    else if (key == 13) return "ENTER"; 
    else if (key== 19) return "CTRL_S"; 
    else if (key == 26) return "CTRL_Z";    
    else if (key == 25) return "CTRL_Y";  
    else if (key == 15) return "CTRL_O";

    else if (key >= 32 && key <= 126) return std::string(1, static_cast<char>(key));

    return "UNKNOWN";
}

void render_editor() {
    clear_screen();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    for (int i = 0; i < lines.size() && i < max_y - 1; ++i) {
        mvprintw(i, 0, "%4d ", i + 1);
        printw("%s", lines[i].c_str());
    }  
    statusbar();
    move(cursor_y, cursor_x + 5);    
    refresh();

}

void record_state() {
    redo_history.clear();
    undo_history.push_back(lines);
    if (undo_history.size() > MAX_HISTORY) {
        undo_history.erase(undo_history.begin());
    }
};

void undo() {
    if (!undo_history.empty()) {      
        redo_history.push_back(lines);
        if (redo_history.size() > MAX_HISTORY) {
            redo_history.erase(redo_history.begin());
        }
        lines = undo_history.back();
        undo_history.pop_back();
        cursor_y = std::min(cursor_y, (int)lines.size() - 1);
        if (cursor_y < 0) cursor_y = 0; 
        cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
    }
}

void redo() {
    if (!redo_history.empty()) { 
        undo_history.push_back(lines);
        if (undo_history.size() > MAX_HISTORY) {
            undo_history.erase(undo_history.begin());
        }
        lines = redo_history.back();
        redo_history.pop_back();
        cursor_y = std::min(cursor_y, (int)lines.size() - 1);
        if (cursor_y < 0) cursor_y = 0;
        cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
    }
}

void save_file() {
    unset_raw_mode();

    std::cout << "\033[H";
    std::cout << "Save: " << std::flush;
    
    std::string new_filename_input;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
    std::getline(std::cin, new_filename_input);

    if (!new_filename_input.empty()) {
        filename = new_filename_input;
    }

    FILE* file = fopen(filename.c_str(), "w");
    if (file) {
        for (const auto& line : lines) {
            fprintf(file, "%s\n", line.c_str());
        }
        fclose(file);
        std::cout << "\nFile '" << filename << "'Created";
    } else {
        std::cout << "\n\033[31mError\033[0m";
    }
    std::cout.flush();    
    get_key(); 
    set_raw_mode();
    clear_screen();
    render_editor();
}

void load_file(const std::string& file_to_load) {
    if (!file_to_load.empty()) {
      
    } else {    
        unset_raw_mode();
        std::cout << "\033[H";
        std::cout << "Enter Path to load: " << std::flush;
        std::string load_filename_input;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, load_filename_input);
        
        if (!load_filename_input.empty()) {
            filename = load_filename_input;
        }   
        
    }     

    FILE* file = fopen(filename.c_str(), "r");
    if (file) {
        lines.clear();
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file)) {
            std::string line(buffer);
            if (!line.empty() && line.back() == '\n') {
                line.pop_back(); 
            }
            lines.push_back(line);
        }
        fclose(file);
        if (lines.empty()) {
            lines.emplace_back("");
        }
        cursor_y = 0;
        cursor_x = 0;
        std::cout << "\nFile '" << filename << "' loaded";
    } else {
        std::cout << "\n\033[31mUnknown File path\033[0m\n";
    }
    std::cout.flush();
    get_key(); 
    set_raw_mode();
    clear_screen();
    set_window_name(filename);
    render_editor();    
}

void overwrite_file() {
    if (filename == "untitled.txt") {
        save_file();
        return;
    }
    FILE* file = fopen(filename.c_str(), "w"); 
    if (file) {  
        for (const auto& line : lines) {
            fprintf(file, "%s\n", line.c_str());
        }
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        mvprintw(max_y - 1, 0, "SAVED: %s", filename.c_str());
        clrtoeol(); 
        refresh(); 
    } else {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        mvprintw(max_y - 1, 0, "ERROR SAVING: %s", filename.c_str());
        clrtoeol(); 
        refresh();
    }
}



void handle_key(const std::string& key) {  
    if (key != "UP" && key != "DOWN" && key != "LEFT" && key != "RIGHT" && key != "CTRL_Z" && key != "CTRL_Y" && key != "END" && key != "CTRL_Q" && key != "CTRL_O") {
        record_state();
    }  
    if (lines.empty()) {
        lines.emplace_back("");
        cursor_y = 0;
        cursor_x = 0;
        return;
    }

    cursor_y = std::max(0, std::min(cursor_y, (int)lines.size() - 1));
    std::string& current_line_content = lines[cursor_y];
    cursor_x = std::max(0, std::min(cursor_x, (int)current_line_content.length()));

    if (key == "UP") {        
        if (cursor_y > 0) {
            cursor_y--;
            cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
        }        
    } else if (key == "DOWN") {        
        if (cursor_y < lines.size() - 1) {            
            cursor_y++;
            cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
        }        
    } else if (key == "LEFT") {        
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = lines[cursor_y].length();
        }       
    } else if (key == "RIGHT") {        
        if (cursor_x < current_line_content.length()) {
            cursor_x++;
        } else if (cursor_y < lines.size() - 1) {
            cursor_y++;
            cursor_x = 0;
        }        
    } else if (key == "ENTER") {
        lines.insert(lines.begin() + cursor_y + 1, "");
        cursor_y++;
        cursor_x = 0;
    } else if (key == "BACKSPACE") {
        if (cursor_x > 0) {
            current_line_content.erase(cursor_x - 1, 1);
            cursor_x--;
        } else if (cursor_y > 0) {
            int prev_line_len = lines[cursor_y - 1].length();
            lines[cursor_y - 1] += current_line_content;
            lines.erase(lines.begin() + cursor_y);
            cursor_y--;
            cursor_x = prev_line_len;
            if (lines.empty()) {
                lines.emplace_back("");
                cursor_y = 0;
                cursor_x = 0;
            }
        }
    } else if (key == "DEL") {
        if (cursor_x < current_line_content.length()) {
            current_line_content.erase(cursor_x, 1);
        } else if (cursor_y < lines.size() - 1) {
            current_line_content += lines[cursor_y + 1];
            lines.erase(lines.begin() + cursor_y + 1);
            if (lines.empty()) {
                lines.emplace_back("");
                cursor_y = 0;
                cursor_x = 0;
            }
        }
    } else if (key == "PGDN") {         
        lines.insert(lines.begin() + cursor_y + 1, "");
        cursor_y++;
        cursor_x = 0;
    } else if (key == "CTRL_C") {
        clipboard = current_line_content;
    } else if (key == "CTRL_V") {
        current_line_content.insert(cursor_x, clipboard);
        cursor_x += clipboard.length();
    } else if (key == "END") {         
        save_file();
        running = false;
    } else if (key == "CTRL_Q") {         
        running = false;
    } else if (key.length() == 1) {         
        current_line_content.insert(cursor_x, key);
        cursor_x++;
    } else if (key == "CTRL_S") 
    {
        load_file("");
    } else if (key == "CTRL_Z") 
    {
        undo();
    } else if (key == "CTRL_Y") 
    {
        redo();
    } else if (key == "CTRL_O") 
    {
        overwrite_file();
    } 

    cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
}

