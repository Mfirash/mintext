#include <iostream>
#include <vector>
#include <string>
#include <cstdio> 
#include <cstdlib> 
#include <algorithm>
#include <conio.h> 
#include <windows.h>
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
    system("cls");
    std::cout << "\033[H\033[J"; 
}

void set_raw_mode() {
}

void unset_raw_mode() {

}
void statusbar() { 
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD originalPos = csbi.dwCursorPosition;
    COORD bottomLeft;
    bottomLeft.X = 0;
    bottomLeft.Y = csbi.srWindow.Bottom;
    SetConsoleCursorPosition(hConsole, bottomLeft);      
    std::cout << "\033[7m '" << filename << "' X: '" << cursor_x << "' Y: '" << cursor_y << "' Ln: '" << lines.size() << "' ";    
    std::cout << "\033[0m";
    SetConsoleCursorPosition(hConsole, originalPos);
} 

void set_window_name(const std::string& new_title) {
    SetConsoleTitleA(new_title.c_str());
}

std::string get_key() {
    int key = _getch();
    if (key == 0 || key == 0xE0) { 
        key = _getch();
        if (key == 'H') return "UP";
        else if (key == 'P') return "DOWN";
        else if (key == 'K') return "LEFT";
        else if (key == 'M') return "RIGHT";
        else if (key == 'S') return "DEL";
        else if (key == 'I') return "PGDN";
        else if (key == 'O') return "END";
        else return "UNKNOWN_SPECIAL_WIN";
    } else if (key == 3) return "CTRL_C"; 
    else if (key == 22) return "CTRL_V"; 
    else if (key == 17) return "CTRL_Q"; 
    else if (key == 8) return "BACKSPACE";
    else if (key == 13) return "ENTER"; 
    else if (key== 19) return "CTRL_S"; 
    else if (key == 26) return "CTRL_Z";    
    else if (key == 25) return "CTRL_Y";  
    else if (key == 15) return "CTRL_O";
    else return std::string(1, static_cast<char>(key));
}

void render_editor() {
    clear_screen();

    for (int i = 0; i < lines.size(); ++i) {
        printf("%4d %s\r\n", i + 1, lines[i].c_str());
    }  
    statusbar();
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos;
    pos.X = cursor_x + 5;
    pos.Y = cursor_y;  
    SetConsoleCursorPosition(hConsole, pos);  
    statusbar();
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
    clear_screen();
    std::cout << "\033[H";
    std::cout << "Enter filename to save: ";
    std::cout.flush();

    unset_raw_mode();
    std::string new_filename_input;
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
        filename = file_to_load; 
    } else {
    clear_screen();
    std::cout << "\033[H";
    std::cout << "Enter Path to load: ";
    std::cout.flush();

    unset_raw_mode();
    std::string load_filename_input;
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
        fclose(file);
        std::cout << "\033[30;1H\033[K\033[7mSAVED: " << filename << "\033[0m" << std::endl;
        fflush(stdout);        
    } else {
        std::cout << "\033[30;1H\033[K\033[7mERROR SAVING: " << filename << "\033[0m" << std::endl;
        fflush(stdout);
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
        std::string part_after_cursor = current_line_content.substr(cursor_x);
        current_line_content = current_line_content.substr(0, cursor_x);
        lines.insert(lines.begin() + cursor_y + 1, part_after_cursor);
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

