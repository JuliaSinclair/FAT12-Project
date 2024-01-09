#ifndef _DISK_FUNCTIONS_H_
#define _DISK_FUNCTIONS_H_

// Global Variables 
#define SECTOR_SIZE 512 
#define MAX_DATA_AREA 2848


// Functions
int read_FAT_entry(char* p, int location);
void update_FAT_entry(char* p, int location, int update);
int total_disk_size(char* p);
int free_disk_space(char* p);
int get_file_size(char* p, int location);
int find_free_cluster(char* p);
void trim_white_space(char* string);

#endif