#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "diskfunctions.h"

/*
	Function: total_disk_files
	Arguments:
		p: A pointer to mapped memory
		directory: location of directory in mapped memory
	Purpose: Returns number of files in file system
*/
int total_disk_files(char * p, int directory) {
    int total_files = 0;
    int FAT_entry = directory/SECTOR_SIZE - 31;
    int sector = 512;
    int start = 1;
    // If root directory 
    if (directory == SECTOR_SIZE*19){
        sector *= 14;
    }
    while(start == 1 || (FAT_entry != 0x00 && 0xFF0 > FAT_entry)){
        for(int i = 0; i < sector; i += 32){
            int first_logical_cluster = ((p[i + directory + 27]&0xFF)<<8) + (p[i + directory + 26]&0xFF);
            uint8_t attributes = p[i + 11 + directory];
            int volume_label_bit = (attributes & ( 1 << 3 )) >> 3;
            int subdirectory_bit = (attributes & (1 << 4)) >> 4;
            if (p[i + directory] == 0x00){
                break;
            }
            if(p[i + directory] != 0x2E && attributes != 0x0F && first_logical_cluster != 0 && first_logical_cluster != 1 && volume_label_bit != 1 && p[i + directory] != 0xE5){
                if (subdirectory_bit == 1){
                    int subdirectory = (first_logical_cluster + 31) * SECTOR_SIZE;
                    total_files += total_disk_files(p , subdirectory);
                }

                else {
                    total_files++;
                }
            }
        }
        start = 0;
        if (FAT_entry > 1){
            FAT_entry = read_FAT_entry(p, FAT_entry);
            directory = (FAT_entry+31) * SECTOR_SIZE;
        }
        else if (FAT_entry < 1){
            FAT_entry = 0x00;
        }
    }
    return total_files;
}


int main(int argc, char *argv[]) {
    // Check for input file
    if (argc != 2) {
        fprintf(stderr, "Please input a file system image \n");
        exit(1);
    }

    // Opening File System Image 
    // Modified code from CSC 360 Fall 2022 Tutorial 8
	int fd;
	struct stat sb;

	fd = open(argv[1], O_RDWR);
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
    
    // Find OS Name in Boot Sector
    char* os_name = malloc(sizeof(char));
    int j = 0;
    for (int i = 3; i < 8 + 3; i++){
        os_name[j] = p[i];
        j++;
    }

    // Find Disk Label (Volume Label)
    char* disk_label = malloc(sizeof(char));
    int k = 0;
    // Volume label in Boot Sector
    for (int i = 43; i < 43 + 11; i ++){
        disk_label[k] = p[i];
           k++;
    }
    // If empty, then Volume Label in Root Directory
    if (disk_label[0] == 32){ 
        int disk_location = SECTOR_SIZE * 19;
        while (disk_location < SECTOR_SIZE * 32) {   
            uint8_t attributes = p[11 + disk_location];
            if (attributes == 0x08) {
                for(int i = 0; i < 8; i++){
                    disk_label[i] = p[i + disk_location];
                }
            }
            disk_location += 32;
        }
    }

    // Find total size of the disk
    int disk_size = total_disk_size(p);

    //Find free size of the disk
    int free_size = free_disk_space(p);

    // Find number of files in the disk
    int total_files = total_disk_files(p, SECTOR_SIZE*19);

    // Find number of FAT copies in Boot Sector
    int FAT_copies = p[16];

    // Find number of sectors per FAT in Boot Sector
    int sectors = p[22] + (p[23] << 8);


    // Print information 
    printf("OS Name: %s\n", os_name);
    printf("Label of the disk: %s\n", disk_label);
    printf("Total size of the disk: %d bytes \n", disk_size);
    printf("Free size of the disk: %d bytes \n\n", free_size);
    printf("==============\n");
    printf("The number of files in the disk: %d \n\n", total_files);
    printf("=============\n");
    printf("Number of FAT copies: %d\n", FAT_copies);
    printf("Sectors per FAT: %d \n", sectors);


    // Free memory space
    free(os_name);
    free(disk_label);
	munmap(p, sb.st_size); 
	close(fd);
	return 0;
}