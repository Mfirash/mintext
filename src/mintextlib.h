#pragma once 

#include <string>
#include <vector>

extern std::vector<std::string> lines;
extern int cursor_y;
extern int cursor_x;
extern std::string clipboard;
extern bool running;
extern std::string filename;
extern int scroll_x;
extern int last_max_y;
extern int last_max_x;
extern int max_y, max_x;
extern int scroll_y; 
extern int sel_anchor_y, sel_anchor_x;
extern bool is_selecting;

void clear_screen();
void set_raw_mode();
void unset_raw_mode();
std::string get_key();
void render_editor();
void statusbar();
void set_window_name(const std::string& new_title);
void record_state();
void undo();
void redo();
void save_file();
void load_file(const std::string& file_to_load);
void overwrite_file();
void handle_key(const std::string& key);
void changefilename(const std::string& new_name); 
bool createfile();
void check_resize();