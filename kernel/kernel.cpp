#include "../include/common.h"
#include "../libc/stdio.h"

bool in_editor = false;
int editing_file_id = -1;
char* editor_buffer = nullptr; 
int editor_len = 0;
int cursor_pos = 0;   

bool expect_e0 = false; 
bool shift_pressed = false; 

void draw_editor() {
    for (int i = 0; i < 80 * 25; i++) vga[i] = (unsigned short)' ' | (color << 8);

    const char* header = "--- ogyrec edit --- FILE: ";
    int h_i = 0; while(header[h_i]) { vga[h_i] = header[h_i] | (0x0E << 8); h_i++; }
    int f_i = 0; while(fs[editing_file_id].name[f_i]) { vga[h_i] = fs[editing_file_id].name[f_i] | (0x0F << 8); h_i++; f_i++; }
    const char* h_end = " --- [ESC: AUTO-SAVE & EXIT]";
    int he_i = 0; while(h_end[he_i]) { vga[h_i] = h_end[he_i] | (0x0E << 8); h_i++; he_i++; }
    for(int i = 0; i < 80; i++) vga[80 + i] = '-' | (0x08 << 8);

    int cx = 0, cy = 0;
    for(int i = 0; i < cursor_pos; i++) {
        if(editor_buffer[i] == '\n') { cx = 0; cy++; }
        else { cx++; if(cx >= 80) { cx = 0; cy++; } }
    }
    int scroll_y = 0; if (cy > 22) scroll_y = cy - 22;

    int draw_x = 0, draw_y = 0;
    for(int i = 0; i < editor_len; i++) {
        if (editor_buffer[i] == '\n') { draw_x = 0; draw_y++; continue; }

        if (draw_y >= scroll_y && draw_y < scroll_y + 23) {
            int screen_y = draw_y - scroll_y + 2;
            vga[screen_y * 80 + draw_x] = editor_buffer[i] | (color << 8);
        }
        draw_x++; if(draw_x >= 80) { draw_x = 0; draw_y++; }
    }

    if (cy >= scroll_y && cy < scroll_y + 23) {
        int screen_y = cy - scroll_y + 2;
        unsigned short current = vga[screen_y * 80 + cx];
        vga[screen_y * 80 + cx] = (current & 0x00FF) | 0x7000; 
    }
}

int get_cursor_x(int target_pos) {
    int cx = 0;
    for(int i = 0; i < target_pos; i++) {
        if(editor_buffer[i] == '\n') cx = 0;
        else { cx++; if(cx >= 80) cx = 0; }
    }
    return cx;
}

int get_cursor_y(int target_pos) {
    int cy = 0, cx = 0;
    for(int i = 0; i < target_pos; i++) {
        if(editor_buffer[i] == '\n') { cx = 0; cy++; }
        else { cx++; if(cx >= 80) { cx = 0; cy++; } }
    }
    return cy;
}

int get_pos_from_xy(int target_x, int target_y) {
    int cx = 0, cy = 0;
    int last_valid_pos = 0;
    for(int i = 0; i <= editor_len; i++) {
        if (cy == target_y) {
            if (cx == target_x || i == editor_len || editor_buffer[i] == '\n') return i;
        }
        if (cy > target_y) return last_valid_pos; 
        
        last_valid_pos = i;
        if(i < editor_len) {
            if(editor_buffer[i] == '\n') { cx = 0; cy++; }
            else { cx++; if(cx >= 80) { cx = 0; cy++; } }
        }
    }
    return editor_len;
}

void int_to_string(int num, char* str) {
    if (num == 0) { str[0] = '0'; str[1] = '\0'; return; }
    int temp = num, len = 0;
    while(temp > 0) { len++; temp /= 10; }
    str[len] = '\0';
    while(num > 0) { str[--len] = (num % 10) + '0'; num /= 10; }
}

void make_unique_name(char* name, int parent_dir, bool is_folder) {
    char base_name[16];
    copy_string(base_name, name, 16);
    int counter = 1;
    
    while(true) {
        bool exists = false;
        for(int i = 1; i < MAX_NODES; i++) {
            if(fs[i].is_used && fs[i].parent_id == parent_dir && fs[i].is_dir == is_folder && strings_equal(fs[i].name, name)) {
                exists = true; break;
            }
        }
        if(!exists) break; 
        
        char num_str[8];
        int_to_string(counter, num_str);
        
        int base_len = string_length(base_name);
        int num_len = string_length(num_str);
        
        int copy_len = base_len;
        if(copy_len + num_len > 15) copy_len = 15 - num_len;
        
        for(int i = 0; i < copy_len; i++) name[i] = base_name[i];
        for(int i = 0; i < num_len; i++) name[copy_len + i] = num_str[i];
        name[copy_len + num_len] = '\0';
        
        counter++;
    }
}

extern "C" void kernel_main() {
    init_memory(); clear_screen(); load_disk(); 
    
    // --- ТЕСТИРУЕМ НАШ НОВЫЙ PRINTF! ---
    int free_ram_kb = 1024; 
    const char* os_name = "OgyrecOS";
    
    printf("Welcome to %s!\n", os_name);
    printf("System Booted successfully. %d KB of RAM available.\n", free_ram_kb);
    printf("Type '%s' to see available commands.\n", "help");
    // -----------------------------------
    
    print_prompt();

    char command_buffer[256]; int cmd_index = 0;

    while (true) {
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);
            if (scancode == 0xE0) { expect_e0 = true; continue; }

            if (scancode == 0x2A || scancode == 0x36) { shift_pressed = true; continue; }
            if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = false; continue; }

            if (scancode < 0x80) {
                char c = scancode_to_char(scancode);

                if (shift_pressed && c != 0) {
                    if (c >= 'a' && c <= 'z') c -= 32; 
                    else if (c == '-') c = '_';
                    else if (c == '=') c = '+';
                    else if (c == '1') c = '!';
                    else if (c == '2') c = '@';
                    else if (c == '3') c = '#';
                    else if (c == '4') c = '$';
                    else if (c == '5') c = '%';
                    else if (c == '6') c = '^';
                    else if (c == '7') c = '&';
                    else if (c == '8') c = '*';
                    else if (c == '9') c = '(';
                    else if (c == '0') c = ')';
                    else if (c == '[') c = '{';
                    else if (c == ']') c = '}';
                    else if (c == ';') c = ':';
                    else if (c == '\'') c = '"';
                    else if (c == ',') c = '<';
                    else if (c == '.') c = '>';
                    else if (c == '/') c = '?';
                }

                if (in_editor) {
                    if (expect_e0) {
                        if (scancode == 0x4B && cursor_pos > 0) cursor_pos--;
                        else if (scancode == 0x4D && cursor_pos < editor_len) cursor_pos++;
                        else if (scancode == 0x48) { 
                            int cx = get_cursor_x(cursor_pos); int cy = get_cursor_y(cursor_pos);
                            if (cy > 0) cursor_pos = get_pos_from_xy(cx, cy - 1);
                        } 
                        else if (scancode == 0x50) { 
                            int cx = get_cursor_x(cursor_pos); int cy = get_cursor_y(cursor_pos);
                            cursor_pos = get_pos_from_xy(cx, cy + 1);
                        }
                        expect_e0 = false; draw_editor();
                    }
                    else if (c == 27) { 
                        write_ram_to_file(editing_file_id, editor_buffer, editor_len);
                        free(editor_buffer); in_editor = false; clear_screen();
                        save_disk(); 
                        printf("Auto-saved to disk!\n"); print_prompt();
                    } 
                    else if (c == '\b' && cursor_pos > 0) {
                        for(int i = cursor_pos-1; i < editor_len; i++) editor_buffer[i] = editor_buffer[i+1];
                        editor_len--; cursor_pos--; draw_editor();
                    } 
                    else if (c >= 32 || c == '\n') {
                        if (editor_len < 8000) {
                            for(int i = editor_len; i > cursor_pos; i--) editor_buffer[i] = editor_buffer[i-1];
                            editor_buffer[cursor_pos] = c; editor_len++; cursor_pos++; draw_editor();
                        }
                    }
                } 
                else {
                    expect_e0 = false;
                    if (c == '\n') {
                        command_buffer[cmd_index] = '\0'; print_char('\n');
                        
                        if (cmd_index == 0) { }
                        
                        else if (strings_equal(command_buffer, "help")) {
                            printf("=== OgyrecOS Commands ===\n");
                            printf(" help       - Show this list\n");
                            printf(" list       - List files and folders\n");
                            printf(" cd <dir>   - Open folder (.. to go back)\n");
                            printf(" mkdir <nm> - Create a new folder\n");
                            printf(" create <nm>- Create an empty file\n");
                            printf(" edit <nm>  - Open ogyrec edit\n");
                            printf(" rename / rendir <old> <new> - Rename \n");
                            printf(" move / movedir <item> <dest> - Move\n");
                            printf(" delete / rmdir <name>       - Delete\n");
                            printf(" clear      - Clear screen\n");
                            printf("=========================\n");
                        }
                        
                        else if (starts_with(command_buffer, "mkdir ")) {
                            char temp_name[16]; copy_string(temp_name, &command_buffer[6], 16);
                            make_unique_name(temp_name, current_dir, true); 
                            
                            int free_slot = find_free_node();
                            if (free_slot != -1) {
                                fs[free_slot].is_used = true; fs[free_slot].is_dir = true; 
                                fs[free_slot].parent_id = current_dir; fs[free_slot].size = 0; fs[free_slot].first_sector = 0;  
                                copy_string(fs[free_slot].name, temp_name, 16);
                                save_disk(); 
                            } else printf("Error: No free nodes!\n");
                        }
                        
                        else if (starts_with(command_buffer, "create ")) {
                            char temp_name[16]; copy_string(temp_name, &command_buffer[7], 16);
                            make_unique_name(temp_name, current_dir, false); 
                            
                            int free_slot = find_free_node();
                            if (free_slot != -1) {
                                fs[free_slot].is_used = true; fs[free_slot].is_dir = false;     
                                fs[free_slot].parent_id = current_dir; fs[free_slot].size = 0; fs[free_slot].first_sector = 0;  
                                copy_string(fs[free_slot].name, temp_name, 16);
                                save_disk(); 
                                printf("Created: %s\n", temp_name);
                            } else printf("Error: No free nodes!\n");
                        }
                        
                        else if (starts_with(command_buffer, "delete ") || starts_with(command_buffer, "rmdir ")) {
                            bool is_dir_cmd = starts_with(command_buffer, "rmdir ");
                            int offset = is_dir_cmd ? 6 : 7;
                            char target[16]; copy_string(target, &command_buffer[offset], 16);
                            
                            int id = -1;
                            for(int i=1; i < MAX_NODES; i++) {
                                if(fs[i].is_used && fs[i].parent_id == current_dir && fs[i].is_dir == is_dir_cmd && strings_equal(fs[i].name, target)) { id = i; break; }
                            }
                            if(id != -1) {
                                fs[id].is_used = false; 
                                int current_sector = fs[id].first_sector;
                                while(current_sector != 0 && current_sector != 0xFFFF) {
                                    int next = fat[current_sector];
                                    fat[current_sector] = 0; 
                                    current_sector = next;
                                }
                                save_disk();
                                printf("Deleted successfully!\n");
                            } else printf("Error: Not found!\n");
                        }
                        
                        else if (starts_with(command_buffer, "rename ") || starts_with(command_buffer, "rendir ")) {
                            bool is_dir_cmd = starts_with(command_buffer, "rendir ");
                            int offset = 7; 
                            
                            char old_name[16]; char new_name[16];
                            old_name[0] = '\0'; new_name[0] = '\0';
                            int space_idx = -1;
                            for(int i=offset; command_buffer[i] != '\0'; i++) {
                                if(command_buffer[i] == ' ') { space_idx = i; break; }
                            }
                            if(space_idx != -1) {
                                int i=0; for(; i < space_idx - offset && i < 15; i++) old_name[i] = command_buffer[offset + i]; old_name[i] = '\0';
                                int j=0; for(; command_buffer[space_idx + 1 + j] != '\0' && j < 15; j++) new_name[j] = command_buffer[space_idx + 1 + j]; new_name[j] = '\0';
                                
                                int id = -1;
                                for(int k=1; k < MAX_NODES; k++) {
                                    if(fs[k].is_used && fs[k].parent_id == current_dir && fs[k].is_dir == is_dir_cmd && strings_equal(fs[k].name, old_name)) { id = k; break; }
                                }
                                if(id != -1) {
                                    bool conflict = false;
                                    for(int k=1; k < MAX_NODES; k++) {
                                        if(fs[k].is_used && fs[k].parent_id == current_dir && fs[k].is_dir == is_dir_cmd && strings_equal(fs[k].name, new_name)) { conflict = true; break; }
                                    }
                                    if(conflict) {
                                        printf("Error: Name already exists for this type!\n");
                                    } else {
                                        copy_string(fs[id].name, new_name, 16);
                                        save_disk();
                                        printf("Renamed successfully!\n");
                                    }
                                } else printf("Error: Item not found!\n");
                            } else printf("Usage: rename/rendir <old> <new>\n");
                        }

                        else if (starts_with(command_buffer, "move ") || starts_with(command_buffer, "movedir ")) {
                            bool is_dir_cmd = starts_with(command_buffer, "movedir ");
                            int offset = is_dir_cmd ? 8 : 5;
                            
                            char item_name[16]; char dest_name[16];
                            item_name[0] = '\0'; dest_name[0] = '\0';
                            int space_idx = -1;
                            for(int i = offset; command_buffer[i] != '\0'; i++) {
                                if(command_buffer[i] == ' ') { space_idx = i; break; }
                            }
                            if(space_idx != -1) {
                                int i=0; for(; i < space_idx - offset && i < 15; i++) item_name[i] = command_buffer[offset + i]; item_name[i] = '\0';
                                int j=0; for(; command_buffer[space_idx + 1 + j] != '\0' && j < 15; j++) dest_name[j] = command_buffer[space_idx + 1 + j]; dest_name[j] = '\0';
                                
                                int item_id = -1;
                                for(int k=1; k < MAX_NODES; k++) {
                                    if(fs[k].is_used && fs[k].parent_id == current_dir && fs[k].is_dir == is_dir_cmd && strings_equal(fs[k].name, item_name)) { item_id = k; break; }
                                }
                                
                                if(item_id != -1) {
                                    int dest_id = -1;
                                    if(strings_equal(dest_name, "..")) dest_id = fs[current_dir].parent_id;
                                    else if(strings_equal(dest_name, "/")) dest_id = 0; 
                                    else {
                                        for(int k=1; k < MAX_NODES; k++) {
                                            if(fs[k].is_used && fs[k].is_dir && fs[k].parent_id == current_dir && strings_equal(fs[k].name, dest_name)) {
                                                dest_id = k; break;
                                            }
                                        }
                                    }
                                    
                                    if(dest_id != -1) {
                                        bool conflict = false;
                                        for(int k=1; k < MAX_NODES; k++) {
                                            if(fs[k].is_used && fs[k].parent_id == dest_id && fs[k].is_dir == is_dir_cmd && strings_equal(fs[k].name, item_name)) { conflict = true; break; }
                                        }
                                        if(conflict) {
                                            printf("Error: Name exists in destination!\n");
                                        } else {
                                            fs[item_id].parent_id = dest_id;
                                            save_disk();
                                            printf("Moved successfully!\n");
                                        }
                                    } else printf("Error: Destination folder not found!\n");
                                } else printf("Error: Item not found!\n");
                            } else printf("Usage: move/movedir <item> <dest>\n");
                        }

                        else if (starts_with(command_buffer, "edit ")) {
                            char filename[16]; copy_string(filename, &command_buffer[5], 16);
                            int id = -1;
                            for (int i = 1; i < MAX_NODES; i++) { 
                                if (fs[i].is_used && !fs[i].is_dir && fs[i].parent_id == current_dir && strings_equal(fs[i].name, filename)) { id = i; break; } 
                            }
                            if (id == -1) { 
                                id = find_free_node(); 
                                if (id != -1) { 
                                    fs[id].is_used = true; fs[id].is_dir = false;         
                                    fs[id].parent_id = current_dir; fs[id].size = 0; fs[id].first_sector = 0;       
                                    copy_string(fs[id].name, filename, 16); save_disk(); 
                                } else printf("Error: No free nodes!\n");
                            }
                            if (id != -1) {
                                editing_file_id = id; editor_buffer = (char*)malloc(8192);
                                for(int i=0; i<8192; i++) editor_buffer[i] = 0;
                                if (fs[id].size > 0) { char* d = read_file_to_ram(id); if(d) { copy_string(editor_buffer, d, 8192); free(d); } }
                                editor_len = string_length(editor_buffer); cursor_pos = editor_len;
                                in_editor = true; draw_editor();
                            }
                        }
                        else if (strings_equal(command_buffer, "list")) {
                             for (int i = 1; i < MAX_NODES; i++) { 
                                 if (fs[i].is_used && fs[i].parent_id == current_dir) { 
                                     printf("%s %s\n", fs[i].is_dir ? "[DIR] " : "[FILE]", fs[i].name);
                                 } 
                             }
                        }
                        else if (starts_with(command_buffer, "cd ")) {
                            const char* target = &command_buffer[3];
                            if (strings_equal(target, "..")) current_dir = fs[current_dir].parent_id;
                            else if (strings_equal(target, "/")) current_dir = 0;
                            else {
                                bool found = false;
                                for (int i = 1; i < MAX_NODES; i++) {
                                    if (fs[i].is_used && fs[i].is_dir && fs[i].parent_id == current_dir && strings_equal(fs[i].name, target)) { 
                                        current_dir = i; found = true; break; 
                                    }
                                }
                                if(!found) printf("Directory not found!\n");
                            }
                        } 
                        else if (strings_equal(command_buffer, "cd..")) current_dir = fs[current_dir].parent_id;
                        else if (strings_equal(command_buffer, "cd/")) current_dir = 0;
                        else if (strings_equal(command_buffer, "clear")) clear_screen();
                        else {
                            printf("Unknown command!\n"); 
                        }
                        
                        cmd_index = 0; if(!in_editor) print_prompt();
                    } 
                    else if (c == '\b' && cmd_index > 0) { cmd_index--; delete_last_char(); }
                    else if (c >= 32 && cmd_index < 255) { print_char(c); command_buffer[cmd_index++] = c; }
                }
            }
        }
    }
}