#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

#include <vector>
#include <string>

extern std::vector<std::string> lines;
extern int cursor_y;
extern int cursor_x;
extern std::string clipboard;
extern bool running;
extern std::string filename;
extern std::vector<std::vector<std::string>> undo_history;
extern std::vector<std::vector<std::string>> redo_history;
extern const size_t MAX_HISTORY;

void clear_screen();
void set_raw_mode();
void unset_raw_mode();
void statusbar();
void set_window_name(const std::string& new_title);
std::string get_key();
void render_editor();
void record_state();
void undo();
void redo();
void save_file();
void load_file(const std::string& file_to_load);
void overwrite_file();
void handle_key(const std::string& key);
void print(const std::string& str);

#endif