#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <algorithm> // For std::max and std::min

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// Global variables
std::vector<std::string> lines = {""};
int cursor_y = 0;
int cursor_x = 0;
std::string clipboard = "";
bool running = true;
std::string filename = "untitled.txt";

#ifndef _WIN32
struct termios old_settings;
#endif

// Function prototypes
void clear_screen();
void set_raw_mode();
void unset_raw_mode();
std::string get_key();
void render_editor();
void save_file();
void load_file(); // Added prototype
void handle_key(const std::string& key);

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    std::cout << "\033[H\033[J"; // ANSI escape codes to clear screen and move cursor to home
}

void set_raw_mode() {
#ifndef _WIN32
    tcgetattr(STDIN_FILENO, &old_settings);
    struct termios new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
#endif
}

void unset_raw_mode() {
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSADRAIN, &old_settings);
#endif
}

std::string get_key() {
#ifdef _WIN32
    int key = _getch();
    if (key == 0 || key == 0xE0) { // Special keys
        key = _getch();
        if (key == 'H') return "UP";
        else if (key == 'P') return "DOWN";
        else if (key == 'K') return "LEFT";
        else if (key == 'M') return "RIGHT";
        else if (key == 'S') return "DEL";
        else if (key == 'I') return "PGDN";
        else if (key == 'O') return "END";
        else return "UNKNOWN_SPECIAL_WIN"; // Fallback for unhandled special keys
    } else if (key == 3) return "CTRL_C";   // Ctrl+C
    else if (key == 22) return "CTRL_V";  // Ctrl+V
    else if (key == 17) return "CTRL_Q";  // Ctrl+Q (Exit without saving)
    else if (key == 19) return "CTRL_S";  // Ctrl+S (Load file)
    else if (key == 8) return "BACKSPACE"; // Backspace
    else if (key == 13) return "ENTER";     // Enter
    else return std::string(1, static_cast<char>(key));
#else
    char c;
    read(STDIN_FILENO, &c, 1);
    if (c == '\x1b') { // Escape sequence
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
                read(STDIN_FILENO, &char_code, 1); // Read the ~
                return "DEL";
            }
            else if (char_code == '6') {
                read(STDIN_FILENO, &char_code, 1); // Read the ~
                return "PGDN";
            }
            else if (char_code == 'F') return "END";
            else return "UNKNOWN_SPECIAL_UNIX";
        } else {
            return "UNKNOWN_ESC_SEQ_UNIX";
        }
    } else if (c == '\x03') return "CTRL_C";   // Ctrl+C
    else if (c == '\x16') return "CTRL_V";  // Ctrl+V
    else if (c == '\x11') return "CTRL_Q";  // Ctrl+Q (Exit without saving)
    else if (c == '\x13') return "CTRL_S";  // Ctrl+S (Load file) - Standard key for S in control sequence
    else if (c == '\x7f') return "BACKSPACE"; // Backspace
    else if (c == '\r' || c == '\n') return "ENTER"; // Enter
    return std::string(1, c);
#endif
}

void render_editor() {
    clear_screen();

    // Display line numbers and content
    for (int i = 0; i < lines.size(); ++i) {
        printf("%4d %s\r\n", i + 1, lines[i].c_str());
    }
    
    // Display status/help bar at the bottom
    // Move cursor to the last line of the screen (assuming typical 24-25 line terminal)
    printf("\033[25;1H\033[K"); // Move to line 25, column 1, and clear line
    printf("Ctrl+Q: Quit | End: Save & Exit | Ctrl+S: Load File | Ctrl+C: Copy Line | Ctrl+V: Paste Line");

    // Position cursor at current editing location (accounting for line number + space)
    printf("\033[%d;%dH", cursor_y + 1, cursor_x + 1 + 5); // Set cursor position (row;column)
    fflush(stdout); // Flush the output buffer
}

void save_file() {
    clear_screen();
    std::cout << "\033[H";
    std::cout << "Enter filename to save (e.g., my_document.txt): ";
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
        std::cout << "\nFile '" << filename << "' saved successfully!\nPress any key to continue...";
    } else {
        std::cout << "\nError saving file: Could not open file.\nPress any key to continue...";
    }
    std::cout.flush();
    get_key(); // Wait for user to press a key
    set_raw_mode();
    clear_screen();
    render_editor();
}

void load_file() {
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

    FILE* file = fopen(filename.c_str(), "r");
    if (file) {
        lines.clear();
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file)) {
            std::string line(buffer);
            if (!line.empty() && line.back() == '\n') {
                line.pop_back(); // Remove newline character
            }
            lines.push_back(line);
        }
        fclose(file);
        if (lines.empty()) { // Ensure there is at least one line if file was empty
            lines.emplace_back("");
        }
        cursor_y = 0;
        cursor_x = 0;
        std::cout << "\nFile '" << filename << "' loaded successfully!\nPress any key to continue...";
    } else {
        std::cout << "\nError loading file: Could not open file.\nPress any key to continue...";
    }
    std::cout.flush();
    get_key(); // Wait for user to press a key
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

    // Boundary checks for cursor_y and cursor_x
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
            if (lines.empty()) { // Should not happen if cursor_y > 0 but as a safety
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
            if (lines.empty()) { // Should not happen if lines.size() - 1 >= 1 but as a safety
                lines.emplace_back("");
                cursor_y = 0;
                cursor_x = 0;
            }
        }
    } else if (key == "PGDN") { // Insert line below
        lines.insert(lines.begin() + cursor_y + 1, "");
        cursor_y++;
        cursor_x = 0;
    } else if (key == "CTRL_C") {
        clipboard = current_line_content;
    } else if (key == "CTRL_V") {
        current_line_content.insert(cursor_x, clipboard);
        cursor_x += clipboard.length();
    } else if (key == "END") { // Save and Exit
        save_file();
        running = false;
    } else if (key == "CTRL_S") { // Load file
        load_file();
    } else if (key == "CTRL_Q") { // Exit without saving
        running = false;
    } else if (key.length() == 1) { // Regular character input
        current_line_content.insert(cursor_x, key);
        cursor_x++;
    }

    // Final boundary check for cursor_x
    cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
}

int main() {
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
    std::cout << "\n---Final Content---" << std::endl;
    for (const auto& line : lines) {
        std::cout << line << std::endl;
    }
    std::cout << "-------------------\n" << std::endl;

    return 0;
}