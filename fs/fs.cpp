#include "../include/common.h"

FSNode fs[MAX_NODES];
unsigned short fat[2048]; 
int current_dir = 0;

bool strings_equal(const char* s1, const char* s2) {
    int i = 0; while (s1[i] != '\0' && s2[i] != '\0') { if (s1[i] != s2[i]) return false; i++; } return s1[i] == s2[i];
}
void copy_string(char* dest, const char* src, int max_len) {
    int i = 0; while (src[i] != '\0' && i < max_len - 1) { dest[i] = src[i]; i++; } dest[i] = '\0';
}
bool starts_with(const char* str, const char* prefix) {
    int i = 0; while (prefix[i] != '\0') { if (str[i] != prefix[i]) return false; i++; } return true;
}
int string_length(const char* str) {
    int len = 0; while (str[len] != '\0') len++; return len;
}

void init_fs() {
    for (int i = 0; i < MAX_NODES; i++) fs[i].is_used = false;
    fs[0].is_used = true; fs[0].is_dir = true; fs[0].parent_id = 0; fs[0].size = 0; fs[0].first_sector = 0;
    copy_string(fs[0].name, "/", 2);
    for(int i = 0; i < 2048; i++) fat[i] = 0; 
}

int find_free_node() {
    for (int i = 1; i < MAX_NODES; i++) { if (!fs[i].is_used) return i; } return -1;
}

int find_free_sector() {
    for(int i = 11; i < 2048; i++) { if(fat[i] == 0) return i; } return -1;
}

void ata_wait() {
    while ((inb(0x1F7) & 0x80) == 0x80);
    while ((inb(0x1F7) & 0x20) == 0x20);
}
void ata_read_sector(int lba, unsigned short* buffer) {
    ata_wait(); outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba); outb(0x1F4, (unsigned char)(lba >> 8)); outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20); ata_wait();
    for (int i = 0; i < 256; i++) buffer[i] = inw(0x1F0);
}
void ata_write_sector(int lba, unsigned short* buffer) {
    ata_wait(); outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba); outb(0x1F4, (unsigned char)(lba >> 8)); outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x30); ata_wait();
    for (int i = 0; i < 256; i++) outw(0x1F0, buffer[i]);
}

void load_disk() {
    unsigned short* fs_ptr = (unsigned short*)fs;
    ata_read_sector(1, fs_ptr); ata_read_sector(2, fs_ptr + 256);
    unsigned short* fat_ptr = fat;
    for(int i = 0; i < 8; i++) ata_read_sector(3 + i, fat_ptr + (i * 256));
    if (!fs[0].is_used || !fs[0].is_dir) init_fs();
}

void save_disk() {
    unsigned short* fs_ptr = (unsigned short*)fs;
    ata_write_sector(1, fs_ptr); ata_write_sector(2, fs_ptr + 256);
    unsigned short* fat_ptr = fat;
    for(int i = 0; i < 8; i++) ata_write_sector(3 + i, fat_ptr + (i * 256));
    ata_wait(); outb(0x1F7, 0xE7); ata_wait(); 
}

void write_ram_to_file(int node_id, const char* buffer, int length) {
    int current = fs[node_id].first_sector;
    while(current != 0 && current != 0xFFFF) { int next = fat[current]; fat[current] = 0; current = next; }
    fs[node_id].size = length;
    if (length == 0) { fs[node_id].first_sector = 0; return; }
    int bytes_written = 0; int prev_sector = 0;
    while(bytes_written < length) {
        int sector = find_free_sector(); if(sector == -1) break;
        if(prev_sector == 0) fs[node_id].first_sector = sector; else fat[prev_sector] = sector;
        fat[sector] = 0xFFFF;
        unsigned short block[256]; char* char_block = (char*)block;
        for(int i=0; i<512; i++) char_block[i] = 0;
        int to_write = length - bytes_written; if(to_write > 512) to_write = 512;
        for(int i=0; i<to_write; i++) char_block[i] = buffer[bytes_written + i];
        ata_write_sector(sector, block);
        bytes_written += to_write; prev_sector = sector;
    }
}

char* read_file_to_ram(int node_id) {
    int length = fs[node_id].size; if (length == 0) return nullptr;
    char* buffer = (char*)malloc(length + 1); if(!buffer) return nullptr;
    int current_sector = fs[node_id].first_sector; int bytes_read = 0;
    while(current_sector != 0 && current_sector != 0xFFFF && bytes_read < length) {
        unsigned short block[256]; ata_read_sector(current_sector, block);
        char* char_block = (char*)block;
        int to_read = length - bytes_read; if(to_read > 512) to_read = 512;
        for(int i=0; i<to_read; i++) buffer[bytes_read + i] = char_block[i];
        bytes_read += to_read; current_sector = fat[current_sector];
    }
    buffer[length] = '\0'; return buffer;
}