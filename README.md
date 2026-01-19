# Minimal Text
A TUI text editor in C++
# Commands:
* ^Q = Quit
* ^S = Load
* ^C = Copy
* ^V = Paste 
* PgDn and Enter = New line
* End: Save
* Backspace and Delete: Delete a character
* Arrow keys = Move cursor
# Syntax
To use syntax highlighting
make syntax/[FILETYPE].syntax
and put in
[ATTRON_TYPE] { [WORDS] }
