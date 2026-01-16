#include <iostream>
#include <limits>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <curses.h>
#include "mintextlib.h"
#ifdef _WIN32
#include <windows.h>

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
        return TRUE;
    default:
        return FALSE;
    }
}

#endif

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
int last_max_y = 0;
int last_max_x = 0;
int max_y, max_x;
int scroll_y = 0;
int scroll_x = 0;
int sel_anchor_y = -1, sel_anchor_x = -1;
bool is_selecting = false;

void clear_screen()
{
    getmaxyx(stdscr, max_y, max_x);
    clear();
    erase();
    refresh();
}

void set_raw_mode()
{
    initscr();
#ifdef _WIN32
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    getmaxyx(stdscr, last_max_y, last_max_x);
}

void unset_raw_mode()
{
    endwin();
}

void check_resize()
{
    int new_max_y, new_max_x;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        new_max_x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        new_max_y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    else
    {
        return;
    }    
#else
    resizeterm(new_y, new_x);
    render_editor();
#endif
    if (new_max_y != last_max_y || new_max_x != last_max_x)
    {
        resize_term(new_max_y, new_max_x);
        last_max_y = new_max_y;
        last_max_x = new_max_x;
        max_y = new_max_y;
        max_x = new_max_x;
        if (cursor_y >= new_max_y + scroll_y - 1)
        {
            scroll_y = std::max(0, cursor_y - new_max_y + 2);
        }
        clear();
    }
}

void statusbar()
{
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    attron(A_REVERSE);
    move(max_y - 1, 0);
    printw("'");
    printw(filename.c_str());
    printw("' X: '%d' Y: '%d' Ln: '%zu' Selection: '%d'", cursor_x, cursor_y, lines.size(), is_selecting);

    for (int i = 0; i < max_x - (filename.length() + 18); ++i)
    {
        printw(" ");
        getmaxyx(stdscr, max_y, max_x);
    }
    attroff(A_REVERSE);
    refresh();
}

void cmdbar()
{
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    move(max_y - 1, 0);
    clrtoeol();
    printw("> ");
    echo();
    curs_set(1);
    char buffer[256];
    getnstr(buffer, sizeof(buffer) - 1);
    std::string full_input(buffer);
    noecho();
    if (full_input.empty())
        return;
    std::string cmd;
    std::string args;
    size_t space_pos = full_input.find(' ');
    if (space_pos != std::string::npos)
    {
        cmd = full_input.substr(0, space_pos);
        args = full_input.substr(space_pos + 1);
    }
    else
    {
        cmd = full_input;
    }
    if (cmd == "quit" || cmd == "q")
    {
        running = false;
    }
    else if (cmd == "save" || cmd == "sv")
    {
        if (!args.empty()) 
        {
            changefilename(args);
            overwrite_file();
            overwrite_file();
        } else 
        {
            overwrite_file();
        }
        
    }
    else if (cmd == "load" || cmd == "ld")
    {        
        load_file(args);
    }
    else if (cmd == "name")
    {
        if (!args.empty())
            changefilename(args);
    }
    else if (cmd == "overwrite" || cmd == "ow" || cmd == "w")
    {
        overwrite_file();
    }
    else if (cmd == "end" || cmd == "e")
    {
        cursor_y = (lines.empty()) ? 0 : (int)lines.size() - 1;
        cursor_x = (lines.empty()) ? 0 : (int)lines[cursor_y].length();
    }
    else if (cmd == "gotoy" || cmd == "y" || cmd == "line")
    {
        if (!args.empty())
        {
            try
            {
                int target_y = std::stoi(args) - 1;
                cursor_y = std::max(0, std::min(target_y, (int)lines.size() - 1));
                cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
            }
            catch (...)
            {
            }
        }
    }
    else if (cmd == "gotox" || cmd == "x")
    {
        if (!args.empty())
        {
            try
            {
                int target_x = std::stoi(args);
                cursor_x = std::max(0, std::min(target_x, (int)lines[cursor_y].length()));
            }
            catch (...)
            {
            }
        }
    }
    render_editor();
}

void set_window_name(const std::string &new_title)
{
    std::cout << "\033]0;" << new_title << "\007" << std::flush;
}

std::string get_key()
{
    int key = getch();

    if (key == KEY_UP)
        return "UP";
    else if (key == KEY_DOWN)
        return "DOWN";
    else if (key == KEY_LEFT)
        return "LEFT";
    else if (key == KEY_RIGHT)
        return "RIGHT";
    else if (key == KEY_DC)
        return "DEL";
    else if (key == KEY_NPAGE)
        return "PGDN";
    else if (key == KEY_END)
        return "END";
    else if (key == KEY_RESIZE)
        return "RESIZE";
    else if (key == 3)
        return "CTRL_C";
    else if (key == 22)
        return "CTRL_V";
    else if (key == 17)
        return "CTRL_Q";
    else if (key == 8 || key == 127 || key == KEY_BACKSPACE)
        return "BACKSPACE";
    else if (key == 10 || key == 13 || key == KEY_ENTER)
        return "ENTER";
    else if (key == 19)
        return "CTRL_S";
    else if (key == 26)
        return "CTRL_Z";
    else if (key == 25)
        return "CTRL_Y";
    else if (key == 15)
        return "CTRL_O";
    else if (key == 1)
        return "CTRL_A";
    else if (key == 24)
        return "CTRL_X";

    else if (key >= 32 && key <= 126)
        return std::string(1, static_cast<char>(key));

    return "UNKNOWN";
}

void render_editor()
{
    check_resize();
    erase();
    clear_screen();
    getmaxyx(stdscr, max_y, max_x);
    const int LINE_NUM_WIDTH = 5;
    const int EDIT_HEIGHT = max_y - 1;

    if (cursor_y < scroll_y)
        scroll_y = cursor_y;
    if (cursor_y >= scroll_y + EDIT_HEIGHT)
        scroll_y = cursor_y - EDIT_HEIGHT + 1;
    if (cursor_x < scroll_x)
        scroll_x = cursor_x;
    if (cursor_x >= scroll_x + (max_x - LINE_NUM_WIDTH))
    {
        scroll_x = cursor_x - (max_x - LINE_NUM_WIDTH) + 1;
    }

    int start_y = sel_anchor_y, start_x = sel_anchor_x;
    int end_y = cursor_y, end_x = cursor_x;

    if (start_y > end_y || (start_y == end_y && start_x > end_x))
    {
        std::swap(start_y, end_y);
        std::swap(start_x, end_x);
    }

    for (int i = 0; i < EDIT_HEIGHT; ++i)
    {
        int line_index = scroll_y + i;
        if (line_index < lines.size())
        {
            mvprintw(i, 0, "%4d ", line_index + 1);
            const std::string &line = lines[line_index];
            for (int x_idx = 0; x_idx < (max_x - LINE_NUM_WIDTH); ++x_idx)
            {
                int real_x = x_idx + scroll_x;
                if (real_x < line.length())
                {
                    bool in_selection = false;
                    if (is_selecting)
                    {
                        if (line_index > start_y && line_index < end_y)
                        {
                            in_selection = true;
                        }
                        else if (start_y == end_y && line_index == start_y)
                        {
                            in_selection = (real_x >= start_x && real_x < end_x);
                        }
                        else if (line_index == start_y)
                        {
                            in_selection = (real_x >= start_x);
                        }
                        else if (line_index == end_y)
                        {
                            in_selection = (real_x < end_x);
                        }
                    }

                    if (in_selection)
                        attron(A_REVERSE);
                    mvaddch(i, x_idx + LINE_NUM_WIDTH, line[real_x]);
                    if (in_selection)
                        attroff(A_REVERSE);
                }
            }
        }
        else
        {
            mvprintw(i, 0, ".");
        }
    }

    statusbar();
    move(cursor_y - scroll_y, cursor_x - scroll_x + LINE_NUM_WIDTH);
    refresh();
}

void record_state()
{
    redo_history.clear();
    undo_history.push_back(lines);
    if (undo_history.size() > MAX_HISTORY)
    {
        undo_history.erase(undo_history.begin());
    }
};

void undo()
{
    if (!undo_history.empty())
    {
        redo_history.push_back(lines);
        if (redo_history.size() > MAX_HISTORY)
        {
            redo_history.erase(redo_history.begin());
        }
        lines = undo_history.back();
        undo_history.pop_back();
        cursor_y = std::min(cursor_y, (int)lines.size() - 1);
        if (cursor_y < 0)
            cursor_y = 0;
        cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
    }
}

void redo()
{
    if (!redo_history.empty())
    {
        undo_history.push_back(lines);
        if (undo_history.size() > MAX_HISTORY)
        {
            undo_history.erase(undo_history.begin());
        }
        lines = redo_history.back();
        redo_history.pop_back();
        cursor_y = std::min(cursor_y, (int)lines.size() - 1);
        if (cursor_y < 0)
            cursor_y = 0;
        cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
    }
}

void save_file()
{
    unset_raw_mode();

    std::cout << "\033[H";
    std::cout << "Save: " << std::flush;

    std::string new_filename_input;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, new_filename_input);

    if (!new_filename_input.empty())
    {
        filename = new_filename_input;
    }

    FILE *file = fopen(filename.c_str(), "w");
    if (file)
    {
        for (const auto &line : lines)
        {
            fprintf(file, "%s\n", line.c_str());
        }
        fclose(file);
        std::cout << "\nFile '" << filename << "'Created";
    }
    else
    {
        std::cout << "\n\033[31mError\033[0m";
    }
    std::cout.flush();
    get_key();
    set_raw_mode();
    clear_screen();
    render_editor();
}

void changefilename(const std::string &new_name)
{ // API for apps that embed mintext
    if (!new_name.empty())
    {
        filename = new_name;
        set_window_name(filename);
    }
}

bool createfile()
{ // API for apps that embed mintext
    FILE *file = fopen(filename.c_str(), "w");
    if (file)
    {
        fclose(file);
        lines.clear();
        lines.emplace_back("");
        cursor_y = 0;
        cursor_x = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void load_file(const std::string &file_to_load)
{
    if (!file_to_load.empty())
    {
    }
    else
    {
        unset_raw_mode();
        std::cout << "\033[H";
        std::cout << "Enter Path to load: " << std::flush;
        std::string load_filename_input;
        std::getline(std::cin, load_filename_input);

        if (!load_filename_input.empty())
        {
            filename = load_filename_input;
        }
    }

    FILE *file = fopen(filename.c_str(), "r");
    if (file)
    {
        std::vector<std::string> new_lines;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file)) {
            std::string line(buffer);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            new_lines.push_back(line);
        }
        fclose(file);        
        if (new_lines.empty()) new_lines.push_back("");        
        lines = new_lines;
        cursor_y = 0;
        cursor_x = 0;
    }
    else
    {
        std::cout << "\n\033[31mUnknown File path\033[0m\n";
    }
    std::cout.flush();
    get_key();
    set_raw_mode();
    clear_screen();
    set_window_name(filename);
    render_editor();
}

void overwrite_file()
{
    if (filename == "untitled.txt")
    {
        save_file();
        return;
    }
    if (lines.empty()) return;
    FILE *file = fopen(filename.c_str(), "w");
    if (file)
    {
        for (size_t i = 0; i < lines.size(); ++i)
        {
            fputs(lines[i].c_str(), file);
            if (i < lines.size() - 1) fputc('\n', file);
        }
        fclose(file);
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        mvprintw(max_y - 1, 0, "SAVED: %s", filename.c_str());
        clrtoeol();
        refresh();
    }
    else
    {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        mvprintw(max_y - 1, 0, "ERROR SAVING: %s", filename.c_str());
        clrtoeol();
        refresh();
    }
}

struct Range
{
    int start_y, start_x, end_y, end_x;
};

Range get_ordered_range()
{
    Range r = {sel_anchor_y, sel_anchor_x, cursor_y, cursor_x};
    if (r.start_y > r.end_y || (r.start_y == r.end_y && r.start_x > r.end_x))
    {
        std::swap(r.start_y, r.end_y);
        std::swap(r.start_x, r.end_x);
    }
    return r;
}

void delete_selection()
{
    Range r = get_ordered_range();
    if (r.start_y == r.end_y)
    {
        lines[r.start_y].erase(r.start_x, r.end_x - r.start_x);
    }
    else
    {
        std::string suffix = lines[r.end_y].substr(r.end_x);
        lines[r.start_y].erase(r.start_x);
        lines[r.start_y] += suffix;
        lines.erase(lines.begin() + r.start_y + 1, lines.begin() + r.end_y + 1);
    }
    cursor_y = r.start_y;
    cursor_x = r.start_x;
}

void handle_key(const std::string &key)
{
    if (key != "UP" && key != "DOWN" && key != "LEFT" && key != "RIGHT" &&
        key != "CTRL_Z" && key != "CTRL_Y" && key != "END" &&
        key != "CTRL_Q" && key != "CTRL_O")
    {
        if (is_selecting && (key.length() == 1 || key == "BACKSPACE" || key == "ENTER"))
        {
            delete_selection();
            is_selecting = false;
        }
        record_state();
    }
    if (!is_selecting)
    {
        sel_anchor_y = cursor_y;
        sel_anchor_x = cursor_x;
    }

    if (lines.empty())
    {
        lines.emplace_back("");
        cursor_y = 0;
        cursor_x = 0;
        return;
    }

    cursor_y = std::max(0, std::min(cursor_y, (int)lines.size() - 1));
    std::string &current_line_content = lines[cursor_y];
    cursor_x = std::max(0, std::min(cursor_x, (int)current_line_content.length()));

    if (key == "UP")
    {
        if (cursor_y > 0)
        {
            cursor_y--;
            cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
        }
    }
    else if (key == "DOWN")
    {
        if (cursor_y < (int)lines.size() - 1)
        {
            cursor_y++;
            cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
        }
    }
    else if (key == "LEFT")
    {
        if (cursor_x > 0)
        {
            cursor_x--;
        }
        else if (cursor_y > 0)
        {
            cursor_y--;
            cursor_x = lines[cursor_y].length();
        }
    }
    else if (key == "RIGHT")
    {
        if (cursor_x < (int)current_line_content.length())
        {
            cursor_x++;
        }
        else if (cursor_y < (int)lines.size() - 1)
        {
            cursor_y++;
            cursor_x = 0;
        }
    }
    else if (key == "ENTER")
    {
        std::string text_after_cursor = current_line_content.substr(cursor_x);
        current_line_content.erase(cursor_x);
        lines.insert(lines.begin() + cursor_y + 1, text_after_cursor);
        cursor_y++;
        cursor_x = 0;
    }
    else if (key == "BACKSPACE")
    {
        if (cursor_x > 0)
        {
            current_line_content.erase(cursor_x - 1, 1);
            cursor_x--;
        }
        else if (cursor_y > 0)
        {
            int prev_line_len = lines[cursor_y - 1].length();
            lines[cursor_y - 1] += current_line_content;
            lines.erase(lines.begin() + cursor_y);
            cursor_y--;
            cursor_x = prev_line_len;
        }
    }
    else if (key == "DEL")
    {
        if (cursor_x < (int)current_line_content.length())
        {
            current_line_content.erase(cursor_x, 1);
        }
        else if (cursor_y < (int)lines.size() - 1)
        {
            current_line_content += lines[cursor_y + 1];
            lines.erase(lines.begin() + cursor_y + 1);
        }
    }
    else if (key == "PGDN")
    {
        lines.insert(lines.begin() + cursor_y + 1, "");
        cursor_y++;
        cursor_x = 0;
    }
    else if (key == "CTRL_C")
    {
        if (is_selecting)
        {
            Range r = get_ordered_range();
            clipboard = "";
            for (int i = r.start_y; i <= r.end_y; ++i)
            {
                int x1 = (i == r.start_y) ? r.start_x : 0;
                int x2 = (i == r.end_y) ? r.end_x : lines[i].length();
                clipboard += lines[i].substr(x1, x2 - x1);
                if (i < r.end_y)
                    clipboard += "\n";
            }
            is_selecting = false;
        }
        else
        {
            clipboard = current_line_content;
        }
    }
    else if (key == "CTRL_V")
    {
        current_line_content.insert(cursor_x, clipboard);
        cursor_x += clipboard.length();
    }
    else if (key == "END")
    {
        save_file();
        running = false;
    }
    else if (key == "CTRL_Q")
    {
        running = false;
    }
    else if (key == "CTRL_S")
    {
        load_file("");
    }
    else if (key == "CTRL_Z")
    {
        undo();
    }
    else if (key == "CTRL_Y")
    {
        redo();
    }
    else if (key == "CTRL_O")
    {
        overwrite_file();
    }
    else if (key == "CTRL_A")
    {
        is_selecting = !is_selecting;
        if (is_selecting)
        {
            sel_anchor_y = cursor_y;
            sel_anchor_x = cursor_x;
        }
    }
    else if (key == "CTRL_X")
    {
        cmdbar();
    }
    else if (key.length() == 1)
    {
        current_line_content.insert(cursor_x, key);
        cursor_x++;
    }

    if (!lines.empty())
    {
        cursor_y = std::max(0, std::min(cursor_y, (int)lines.size() - 1));
        cursor_x = std::min(cursor_x, (int)lines[cursor_y].length());
    }
}