#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vsfs.h"


// globals  =======================================
int vs_fd; // file descriptor of the Linux file that acts as virtual disk.
              // this is not visible to an application.

// ========================================================
// Globals for superblock, FAT, and root directory
struct superblock sb;
unsigned int *fat;
struct dir_entry *root_dir;

open_file_table_entry open_file_table[MAX_FILES];  // The open file table
// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vs_fd, (off_t) offset, SEEK_SET);
    n = read (vs_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vs_fd, (off_t) offset, SEEK_SET);
    n = write (vs_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("write error\n");
	return (-1);
    }
    return 0; 
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/
void initialize_open_file_table() {
    for (int i = 0; i < MAX_FILES; i++) {
        open_file_table[i].file_index = 0;
        open_file_table[i].mode = 0;
        open_file_table[i].current_position = 0;
        open_file_table[i].is_free = 1;  // Initially, all entries are free
    }
}

// this function is partially implemented.
int vsformat (char *vdiskname, unsigned int m)
{
    char command[1000];
    int size;
    int num = 1;
    int count;
    size  = num << m; // 2^m
    count = size / BLOCKSIZE;
        printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
             vdiskname, BLOCKSIZE, count);
    //printf ("executing command = %s\n", command);
    system (command);
    // now write the code to format the disk.
    // Open the virtual disk file for writing
    int fd = open(vdiskname, O_WRONLY);
    if (fd == -1) {
        return -1; // File open error
    }
    // ------------------------------------ Initialize and write the superblock
   
        sb.fs_size = count;
        sb.fat_start = 1; // Assuming superblock is in block 0
        sb.root_dir_start = 1 + FAT_SIZE ;// FAT follows superblock

    if (write(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        close(fd);
        return -1; // Write error
    }
    // ------------------------------------ Initialize and write the FAT
    unsigned int fat_entries[FAT_SIZE];//(FAT_SIZE * BLOCKSIZE) / sizeof(unsigned int);
    for (int i = 0; i < FAT_SIZE; ++i) {
        fat_entries[i] = 0;
    }
    if (write(fd, fat_entries, sizeof(fat_entries)) != sizeof(fat_entries)) {
        close(fd);
        return -1; // Write error
    }
    // ------------------------------------ Initialize and write the root directory
	    struct dir_entry *root_dir = malloc(MAX_FILES * sizeof(struct dir_entry)); // Allocating memory for an array of struct dir_entry
	if (root_dir == NULL) {
	    // Handle memory allocation error
	    return -1;
	}

	for (int i = 0; i < MAX_FILES; i++) {
	    root_dir[i].filename[0] = '\0'; // Mark as empty
	    root_dir[i].file_size = 0;
	    root_dir[i].start_block = -1;
	}
    if (write(fd, root_dir, sizeof(root_dir)) != sizeof(root_dir)) {
        close(fd);
        return -1; // Write error
    }

    close(fd);
    return (0); 
}


// this function is partially implemented.
int  vsmount (char *vdiskname)
{
    // open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vs_fd is global; hence other function can use it. 
    vs_fd = open(vdiskname, O_RDWR);
    // load (chache) the superblock info from disk (Linux file) into memory
    // load the FAT table from disk into memory:
    // load root directory from disk into memory
    if (vs_fd == -1) {
        printf("error 1");
        return -1; // Failed to open the file
    }

    // Read the superblock from the virtual disk
    if (pread(vs_fd, &sb, sizeof(sb), 0) != sizeof(sb)) {
        printf("error 2");
        close(vs_fd);
        return -1; // Error reading superblock
    }

    // Allocate memory and read the FAT table
    //fat = (unsigned int *)malloc(FAT_SIZE * BLOCKSIZE);
    fat = (unsigned int *)malloc(FAT_SIZE);
    for (int i = 0; i < FAT_SIZE; ++i) {
        fat[i] = 0;
    }
    if (fat == NULL) {
        printf("error 3");
        close(vs_fd);
        return -1; // Memory allocation failed
    }
    if (pread(vs_fd, fat, FAT_SIZE * BLOCKSIZE, sb.fat_start * BLOCKSIZE) != FAT_SIZE * BLOCKSIZE) {
        free(fat);
        close(vs_fd);
        printf("error 4");
        return -1; // Error reading FAT
    }

    // Allocate memory and read the root directory
    //root_dir = (struct dir_entry *)malloc(ROOT_DIR_BLOCKS * BLOCKSIZE);
    if (root_dir == NULL) {
        free(fat);
        close(vs_fd);
        printf("error 5");
        return -1; // Memory allocation failed
    }
    if (pread(vs_fd, root_dir, ROOT_DIR_BLOCKS * BLOCKSIZE, sb.root_dir_start * BLOCKSIZE) != ROOT_DIR_BLOCKS * BLOCKSIZE) {
        free(root_dir);
        free(fat);
        close(vs_fd);
        printf("error 6");
        return -1; // Error reading root directory
    }

    // Initialize the open file table
    initialize_open_file_table();
    return(0);
}


// this function is partially implemented.
int vsumount ()
{
    // write superblock to virtual disk file
    // write FAT to virtual disk file
    // write root directory to virtual disk file
    // Write back the superblock
    if (pwrite(vs_fd, &sb, sizeof(sb), 0) != sizeof(sb)) {
        return -1; // Error writing superblock
    }

    // Write back the FAT table
    if (pwrite(vs_fd, fat, FAT_SIZE * BLOCKSIZE, sb.fat_start * BLOCKSIZE) != FAT_SIZE * BLOCKSIZE) {
        return -1; // Error writing FAT
    }

    // Write back the root directory
    if (pwrite(vs_fd, root_dir, ROOT_DIR_BLOCKS * BLOCKSIZE, sb.root_dir_start * BLOCKSIZE) != ROOT_DIR_BLOCKS * BLOCKSIZE) {
        return -1; // Error writing root directory
    }

    fsync (vs_fd); // synchronize kernel file cache with the disk
    close (vs_fd);
    // Free allocated memory
    free(fat);
    free(root_dir);
    return (0); 
}


int vscreate(char *filename) {
    // Check if filename is too long or null
    if (!filename || strlen(filename) > FILENAME_MAX_CHAR) {
        printf("Invalid file name %s\n", filename);
        return -1;
    }

    // Check if file already exists
    for (int i = 0; i < MAX_FILES; ++i) {
        if (strcmp(root_dir[i].filename, filename) == 0) {
            printf("There is already a file with the name of %s\n", filename);
            return -1; // File already exists
        }
    }

    // Find a free directory entry
    int freeIndex = -1;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (root_dir[i].filename[0] == '\0') { // Indicates a free entry
            freeIndex = i;
            break;
        }
    }
    if (freeIndex == -1) {
        printf("No free index found in the directory.\n");
        return -1; // No free directory entry found
    }

    // Initialize the directory entry
    strcpy(root_dir[freeIndex].filename, filename);
    root_dir[freeIndex].file_size = 0;
    root_dir[freeIndex].start_block = 0; // Indicates no data blocks are allocated yet

    // Update the root directory on the disk
    int root_dir_start_block = sb.root_dir_start;
    for (int i = 0; i < ROOT_DIR_BLOCKS; ++i) {
        if (write_block(&root_dir[i * (BLOCKSIZE / sizeof(struct dir_entry))], root_dir_start_block + i) != 0) {
            printf("Cannot write root directoy blocks.");
            return -1; // Error writing root directory block
        }
    }

    return 0; // Success
}


// control edilmedi:
int vsopen(char *filename, int mode) {
    printf("vsopen\n");
    // Check if filename is null or empty
    if (!filename || strlen(filename) == 0) {
        return -1;
    }

    // Validate mode
    if (mode != MODE_READ && mode != MODE_APPEND) {
        return -1;
    }

    // Search for the file in the root directory
    int fileIndex = -1;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (strcmp(root_dir[i].filename, filename) == 0) {
            fileIndex = i;
            break;
        }
    }
    if (fileIndex == -1) {
        return -1; // File not found
    }

    // Update open file table
    // Assuming open_file_table is an array of structures that store file state
    int fd = -1;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (open_file_table[i].is_free) {
            open_file_table[i].is_free = 0;
            open_file_table[i].file_index = fileIndex;
            open_file_table[i].mode = mode;
            open_file_table[i].current_position = 0; // Assuming we track read/write position
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        return -1; // No free file descriptor available
    }

    return fd; // Return file descriptor
}


int vsclose(int fd) {
    // Check if fd is within the valid range
    if (fd < 0 || fd >= MAX_FILES) {
    	printf("Invaild fd value fo vsclose %d", fd);
        return -1; // Invalid file descriptor
    }

    // Check if the file descriptor is referring to an open file
    if (open_file_table[fd].is_free) {
    	printf("Invaild is free value fo vsclose %d", open_file_table[fd].is_free);
        return -1; // File descriptor is not in use
    }

    // Mark the file descriptor as free
    open_file_table[fd].file_index = 0;
    open_file_table[fd].mode = 0;
    open_file_table[fd].current_position = 0;
    open_file_table[fd].is_free = 1;

    return 0; // Success
}


int vssize(int fd) {
    // Check if fd is within the valid range
    if (fd < 0 || fd >= MAX_FILES) {
        return -1; // Invalid file descriptor
    }

    // Check if the file descriptor is referring to an open file
    if (open_file_table[fd].is_free) {
        return -1; // File descriptor is not in use
    }

    // Retrieve the file index from the open file table
    int fileIndex = open_file_table[fd].file_index;
    if (fileIndex < 0 || fileIndex >= MAX_FILES) {
        return -1; // Invalid file index
    }

    // Return the size of the file from the root directory entry
    return root_dir[fileIndex].file_size;
}

int read_data(int fileIndex, void *buf, int start_pos, int size) {
    int current_block = root_dir[fileIndex].start_block;
    int offset_in_block = start_pos % BLOCKSIZE;
    int remaining_size = size;
    int total_bytes_read = 0;
    char block[BLOCKSIZE];
    char *buffer_ptr = (char *)buf;

    // Navigate to the starting block
    int blocks_to_skip = start_pos / BLOCKSIZE;
    for (int i = 0; i < blocks_to_skip; ++i) {
        if (current_block == 0) {
            // End of file reached unexpectedly
            return total_bytes_read;
        }
        current_block = fat[current_block];  // Move to the next block
    }

    while (remaining_size > 0 && current_block != 0) {
        // Read the current block
        if (read_block(block, current_block) != 0) {
            return total_bytes_read; // Read error
        }

        // Calculate how much data to copy from this block
        int bytes_in_block = BLOCKSIZE - offset_in_block;
        int bytes_to_copy = (remaining_size < bytes_in_block) ? remaining_size : bytes_in_block;

        // Copy data from the block to the buffer
        memcpy(buffer_ptr, block + offset_in_block, bytes_to_copy);

        // Update pointers and counters
        buffer_ptr += bytes_to_copy;
        total_bytes_read += bytes_to_copy;
        remaining_size -= bytes_to_copy;
        offset_in_block = 0;  // Only needed for the first block

        // Move to the next block
        current_block = fat[current_block];
    }

    return total_bytes_read;
}
int vsread(int fd, void *buf, int n) {
    // Validate the file descriptor and parameters
    if (fd < 0 || fd >= MAX_FILES || open_file_table[fd].is_free || buf == NULL || n <= 0) {
        return -1; // Invalid parameters or file not open
    }

    // Check if the file is opened in read mode
    if (open_file_table[fd].mode != MODE_READ) {
        return -1; // File not opened in read mode
    }

    // Retrieve the file index and information
    int fileIndex = open_file_table[fd].file_index;
    int fileSize = root_dir[fileIndex].file_size;
    int current_position = open_file_table[fd].current_position;

    // Calculate the amount of data to read
    int bytesToRead = n;
    if (current_position + n > fileSize) {
        bytesToRead = fileSize - current_position; // Adjust for EOF
    }

    if (bytesToRead <= 0) {
        return 0; // Nothing to read (EOF)
    }

    // Read data from the file's blocks into the buffer
    int bytesRead = 0;
    // Assuming function read_data() that reads data from the file's blocks
    bytesRead = read_data(fileIndex, buf, current_position, bytesToRead);

    // Update the current position
    open_file_table[fd].current_position += bytesRead;

    return bytesRead; // Return the number of bytes read
}

int allocate_new_block() {
    // Search for a free block in the FAT
    for (int i = 0; i < FAT_ENTRIES; i++) {
        if (fat[i] == 0) { // 0 indicates a free block
            fat[i] = 1;   // Mark the block as used and end of file
            return i;      // Return the block index
        }
    }
    return -1; // No free block found
}
int append_data(int fileIndex, void *buf, int start_pos, int size) {
    	printf("begining:\n");
    char block[BLOCKSIZE];
    char *buffer_ptr = (char *)buf;
    int remaining_size = size;
    int total_bytes_written = 0;
    int current_block = root_dir[fileIndex].start_block;
    int last_block = -1; //-1 ile dennnne
    int offset_in_block = start_pos % BLOCKSIZE;

    // Find the last block of the file and the offset within it
    	//printf(current_block);
    	printf("aaa:\n");
    while (current_block != -1) { //0 ?
    	printf("debug 1\n");
        last_block = current_block;
        current_block = fat[current_block];
    }

    // Start appending data
    while (remaining_size > 0) {
        int bytes_in_block = (offset_in_block + remaining_size > BLOCKSIZE) ? BLOCKSIZE - offset_in_block : remaining_size;

        // Read the block if we need to modify existing data in it
        if (last_block != 0 && offset_in_block > 0) {
            if (read_block(block, last_block) != 0) {
                return total_bytes_written; // Read error
            }
        } else {
            memset(block, 0, BLOCKSIZE); // Start with a clean block
        }

        // Copy data to the block
        memcpy(block + offset_in_block, buffer_ptr, bytes_in_block);

        // Allocate a new block if needed
        if (last_block == 0 || offset_in_block > 0) {
            last_block = allocate_new_block(); // Function to allocate a new block and update FAT
            if (last_block == 0) {
                return total_bytes_written; // Block allocation failed
            }
            if (root_dir[fileIndex].start_block == 0) {
                root_dir[fileIndex].start_block = last_block; // Update start block for empty files
            }
        }

        // Write the block back to disk
        if (write_block(block, last_block) != 0) {
            return total_bytes_written; // Write error
        }

        // Update pointers and counters
        buffer_ptr += bytes_in_block;
        total_bytes_written += bytes_in_block;
        remaining_size -= bytes_in_block;
        offset_in_block = 0; // Only needed for the first iteration
    }

    return total_bytes_written;
}

void printFAT(){
	for( int i = 0; i < FAT_SIZE; i++){
		printf("FAT[%d]:%d \n", i, fat[i]);
	}
}

int vsappend(int fd, void *buf, int n) {
    printf("111:\n");
    // Validate the file descriptor and parameters
    printf("111:\n");
    if (fd < 0 || fd >= MAX_FILES || open_file_table[fd].is_free || buf == NULL || n <= 0) {
        return -1; // Invalid parameters or file not open
    }

    	printf("222:\n");
    // Check if the file is opened in append mode
    if (open_file_table[fd].mode != MODE_APPEND) {
        return -1; // File not opened in append mode
    }

    	printf("333:\n");
    // Retrieve the file index and information
    int fileIndex = open_file_table[fd].file_index;
    int current_size = root_dir[fileIndex].file_size;

    // Find free blocks and append data
    int bytesAppended = 0;
    // Assuming function append_data() that appends data to the file's blocks
    bytesAppended = append_data(fileIndex, buf, current_size, n);

    // Update the file size
    root_dir[fileIndex].file_size += bytesAppended;

    return bytesAppended; // Return the number of bytes appended
}

int vsdelete(char *filename) {
    // Check if filename is null or empty
    if (!filename || strlen(filename) == 0) {
        return -1;
    }

    // Locate the file in the root directory
    int fileIndex = -1;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (strcmp(root_dir[i].filename, filename) == 0) {
            fileIndex = i;
            break;
        }
    }
    if (fileIndex == -1) {
        return -1; // File not found
    }

    // Free the allocated blocks (if any)
    int start_block = root_dir[fileIndex].start_block;
    if (start_block != 0) { // Check if file has allocated blocks
        // Iterate through the FAT and free the blocks
        int current_block = start_block;
        while (current_block != 0) {
            int next_block = fat[current_block]; // Get next block
            fat[current_block] = 0; // Free current block
            current_block = next_block;
        }
    }

    // Update the root directory
    root_dir[fileIndex].filename[0] = '\0'; // Mark as deleted
    root_dir[fileIndex].file_size = 0;
    root_dir[fileIndex].start_block = 0;

    // Write changes to disk (FAT and root directory)
    // Assuming write_block function writes a single block to disk
    // Write updated FAT
    for (int i = 0; i < FAT_SIZE; ++i) {
        if (write_block(&fat[i * (BLOCKSIZE / sizeof(unsigned int))], sb.fat_start + i) != 0) {
            printf("Error writing FAT block");
            return -1; // Error writing FAT block
        }
    }
    // Write updated root directory
    int root_dir_start_block = sb.root_dir_start;
    for (int i = 0; i < ROOT_DIR_BLOCKS; ++i) {
        if (write_block(&root_dir[i * (BLOCKSIZE / sizeof(struct dir_entry))], root_dir_start_block + i) != 0) {
            return -1; // Error writing root directory block
        }
    }

    return 0; // Success
}


