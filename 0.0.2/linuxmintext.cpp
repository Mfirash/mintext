#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <termios.h> 
#include <unistd.h>  

std::vector<std::string> lines = {""};
int cursor_y = 0;
int cursor_x = 0;
std::string clipboard = "";
bool running = true;
std::string filename = "untitled.txt";

struct termios old_settings;

void clear_screen();
void set_raw_mode();
void unset_raw_mode();
std::string get_key();
void render_editor();
void save_file();
void handle_key(const std::string& key);

void clear_screen() {
    system("clear");
    std::cout << "\033[H\033[J"; 
}

void set_raw_mode() {
    tcgetattr(STDIN_FILENO, &old_settings);
    struct termios new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
}

void unset_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSADRAIN, &old_settings);
}

std::string get_key() {
    char c;
    read(STDIN_FILENO, &c, 1);
    
    if (c == '\x1b') { 
        char next_char;
        read(STDIN_FILENO, &next_char, 1);
        if (next_char == '[') {
            char char_code;
            read(STDIN_FILENO, &char_code, 1);
            
            if (char_code == 'A') return "UP";
            else if (char_code == 'B') return "DOWN";
            else if (char_code == 'C') return "RIGHT";
            else if (char_code == 'D') return "LEFT";
            else if (char_code == '3') {
                read(STDIN_FILENO, &char_code, 1); 
                return "DEL";
            }
            else if (char_code == '6') {
                read(STDIN_FILENO, &char_code, 1); 
                return "PGDN"; 
            }
            else if (char_code == 'F') return "END";
            else return "UNKNOWN_SPECIAL_UNIX";
        } else {
            return "UNKNOWN_ESC_SEQ_UNIX";
        }
    } else if (c == '\x03') return "CTRL_C"; 
    else if (c == '\x16') return "CTRL_V"; 
    else if (c == '\x11') return "CTRL_Q"; 
    else if (c == '\x7f') return "BACKSPACE"; 
    else if (c == '\x13') return "CTRL_S";    
    else if (c == '\r' || c == '\n') return "ENTER"; 
    
    return std::string(1, c); 
}

void render_editor() {
    clear_screen();

    for (int i = 0; i < lines.size(); ++i) {
        printf("%4d %s\r\n", i + 1, lines[i].c_str());
    }

   
    printf("\033[%d;%dH", cursor_y + 1, cursor_x + 1 + 5); 
    fflush(stdout); 
}

void save_file() {
    clear_screen();
    std::cout << "\033[H";
    std::cout << "Enter filename to save: ";
    std::cout.flush();

    unset_raw_mode(); // Need to unset raw mode for standard console input (getline)
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
        std::cout << "\nFile '" << filename << "' saved successfully!";
    } else {
        std::cout << "\nError saving file: Could not open file.";
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
    std::cout << "Enter filename to load (e.g., /path/to/file.txt): ";
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
        std::cout << "\nFile '" << filename << "' loaded successfully!\nPress any key to continue...";
    } else {
        std::cout << "\nError loading file: Could not open file.\nPress any key to continue...";
    }
    std::cout.flush();
    get_key(); 
    set_raw_mode();
    clear_screen();
    render_editor();
}

void handle_key(const std::string& key) {
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
    }

    cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
}

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
            return 0;
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