#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "diskfunctions.h"


int read_FAT_entry(char* p, int location){
    int lower;
    int higher;
    int entry;
    if(location % 2 == 0){
        higher = p[((3* location)/2) + SECTOR_SIZE] & 0xFF;
        lower = p[((3* location)/2 + 1) + SECTOR_SIZE] & 0x0F;
        entry = (lower << 8) + higher; 
    }
    else {
        higher = p[((3* location)/2 + 1) + SECTOR_SIZE] & 0x0F;
        lower = p[((3* location)/2) + SECTOR_SIZE] & 0xFF;
        entry = (lower >> 4) + (higher << 4);
    }
    return entry;
}

void update_FAT_entry(char* p, int location, int update){
    if(location % 2 == 0){
        p[((3* location)/2) + SECTOR_SIZE] = update & 0xFF;
        p[((3* location)/2 + 1) + SECTOR_SIZE] = (update >>8) & 0x0F;
    }
    else{
        p[((3* location)/2 + 1) + SECTOR_SIZE] = (update >> 4) & 0x0F;
        p[((3* location)/2) + SECTOR_SIZE] = (update << 4) & 0xFF;
    }
}

int total_disk_size(char* p){
    int sector_count = p[19] + (p[20]<<8);
    int disk_size = sector_count * SECTOR_SIZE;
    return disk_size;
}

int free_disk_space(char* p){
    int num_free_sector = 0;
    int entry;
    for (int i = 2; i <= MAX_DATA_AREA; i++){
        entry = read_FAT_entry(p, i);
        if(entry == 0x000){
            num_free_sector++;
        }
    }
    num_free_sector *= SECTOR_SIZE;
    return num_free_sector;    
}

int get_file_size(char* p, int location){
    int size = ((p[location + 31] & 0xFF) << 24) + ((p[location + 30] & 0xFF) << 16) + ((p[location + 29] &0xFF) << 8) + (p[location + 28]& 0xFF);
    return size;
}

int find_free_cluster(char* p){
    int free_cluster = -1;
    for (int i = 2; i <= MAX_DATA_AREA; i++){
        if(read_FAT_entry(p, i) == 0x000){
            free_cluster = i;
            break;
        }
    }
    return free_cluster;
}

void trim_white_space(char* string){
	int last_value = 0;
	int i = 0;
	while (string[i] != '\0'){
		if (string[i] != ' ' && string[i] != '\t'){
			last_value = i;
		}
		i++;
	}

	string[last_value + 1] = '\0';
}


