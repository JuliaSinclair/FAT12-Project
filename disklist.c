#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "diskfunctions.h"

// Node structure for storing subdirectories in linked list
typedef struct Node Node;

struct Node{
    int location;
    char* name;
    Node* next;
};

// Initialize linked list
Node * head = NULL;

/*
	Function: add_newNode
	Arguments:
		new_location: subdirectory location for new node
		new_name: subdirectory name for new node
	Purpose: Add new node to linked list for subdirectories
	Reference: Modified from CSC 360 Assignment 1
*/
void add_newNode(int new_location, char * new_name){
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->location = new_location;
	new_node->name = new_name;
	new_node->next = NULL;
	
	if (head == NULL){
		head = new_node;
	}
	else{
		Node* current = head;

		while (current->next != NULL){
			current = current->next;
		}

		current->next = new_node;
	}
}

/*
	Function: deleteNode
	Arguments:
		d_location: subdirectory location for node to be deleted 
	Purpose: Delete specific node from linked list for subdirectories
	Reference: Modified from CSC 360 Assignment 1
*/
void deleteNode(int d_location){
	Node* current = head;
	Node* previous = NULL;

	while (current != NULL) {
		if (current->location == d_location) {
			if (current == head) {
				head = head->next;
				free(current);
				current = head;
			}
			else {
				previous->next = current->next;
				free(current);
				current = previous->next;
			}
		}
		else {
			previous = current;
			current = current->next;
		}
	}
}

/*
	Function: select_bit_range
	Arguments:
		value: value to be masked
		minimum: smallest bit to be read
		maximum: largest bit to be read
	Purpose: Returns value of certain bit range of value
*/
int select_bit_range(int value, int minimum, int maximum){
	int mask = 0;
	for (int i = minimum; i <= maximum; i++){
		mask |= 1 << i;
	}
	int result = (value & mask) >> minimum;
	return result;
}

/*
	Function: print_directory
	Arguments:
		p: pointer to mapped memory
		directory: location in mapped memory of directory to be printed 
		directory_name: name of directory to be printed 
	Purpose: Print contents from root directory and all subdirectories 
*/
void print_directory(char* p, int directory, char * directory_name){
    int FAT_entry = directory/SECTOR_SIZE - 31;
    int sector = 512;
    int start = 1;
    // If root directory
    if (directory == SECTOR_SIZE*19){
        sector *= 14;
    }
	// Print Directory Name
	printf("%s\n", directory_name);
	printf("==================\n");

    while(start == 1 || (FAT_entry != 0x00 && 0xff0 > FAT_entry)){
        for(int i = 0; i < sector; i += 32){
            int first_logical_cluster = ((p[i + directory + 27]&0xFF)<<8) + (p[i + directory + 26]&0xFF);
            uint8_t attributes = p[i + 11 + directory];
            int volume_label_bit = (attributes & ( 1 << 3 )) >> 3;
            int subdirectory_bit = (attributes & (1 << 4)) >> 4;
            if (p[i + directory] == 0x00){
                break;
            }
            if(p[i + directory] != 0x2E && attributes != 0x0F && first_logical_cluster != 0 && first_logical_cluster != 1 && volume_label_bit != 1 && p[i + directory] != 0xE5){
                	char* name = malloc(sizeof(char));
					int k;
					for (k = 0; k < 8; k++){
						name[k] = p[k + i + directory];
					}
					name[k] = '\0';

					trim_white_space(name);

					int file_size = get_file_size(p, i + directory);


				if (subdirectory_bit == 1){
                    int subdirectory = (first_logical_cluster + 31) * SECTOR_SIZE;
					char* subdirectory_name = strcat(strcat(strdup(directory_name), "/"), strdup(name));
					add_newNode(subdirectory, subdirectory_name);

					int last_write_date = ((p[i + directory + 25]&0xFF)<<8) + (p[i + directory + 24]&0xFF);
					int year = select_bit_range(last_write_date, 9, 15) + 1980;
					int month = select_bit_range(last_write_date, 5, 8);
					int day = select_bit_range(last_write_date, 0, 4);
					int last_write_time = ((p[i + directory + 23]&0xFF)<<8) + (p[i + directory + 22]&0xFF);
					int hour = select_bit_range(last_write_time, 11, 15);
					int minute = select_bit_range(last_write_time, 5, 10);

					printf("D %10d %20s %02d/%02d/%04d %02d:%02d\n", file_size, name, day, month, year, hour, minute);

                }

                else {
					char* extension = malloc(sizeof(char));
					int l;
					for (l = 8; l < 11; l++){
						extension[l-8] = p[l + i + directory];
					}
					extension[l] = '\0';

					trim_white_space(extension);

					strcat (name, ".");
					strncat(name, extension, 3);

					int creation_date = ((p[i + directory + 17]&0xFF)<<8) + (p[i + directory + 16]&0xFF);
					int year = select_bit_range(creation_date, 9, 15) + 1980;
					int month = select_bit_range(creation_date, 5, 8);
					int day = select_bit_range(creation_date, 0, 4);
					int creation_time = ((p[i + directory + 15]&0xFF)<<8) + (p[i + directory + 14]&0xFF);
					int hour = select_bit_range(creation_time, 11, 15);
					int minute = select_bit_range(creation_time, 5, 10);

					printf("F %10d %20s %02d/%02d/%04d %02d:%02d\n", file_size, name, day, month, year, hour, minute);
					free(extension);
                }

				free(name);
            }
        }
        start = 0;
        if (FAT_entry > 1){
            FAT_entry = read_FAT_entry(p, FAT_entry);
			directory = (FAT_entry + 31) * SECTOR_SIZE;
        }
        else if (FAT_entry < 1){
            FAT_entry = 0x00;
        }
    }
}

int main(int argc, char *argv[]){
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

	char * p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

	//Print all files in root directory and subdirectories
	print_directory(p, SECTOR_SIZE*19, "ROOT");
	while (head != NULL){
		print_directory(p, head->location, head->name);
		Node* next_node = head -> next;
		deleteNode(head->location);
		head = next_node;

	}
	
	// Free memory space
	munmap(p, sb.st_size);
	close(fd);
	return 0;
}