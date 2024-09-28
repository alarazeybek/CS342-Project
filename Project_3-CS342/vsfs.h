

// Do not change this file //

#define MODE_READ 0
#define MODE_APPEND 1
#define BLOCKSIZE 2048 // bytes
// I added:
#define FAT_SIZE 32
#define FAT_ENTRIES 16384
#define FAT_ENTRY_SIZE 4

#define ROOT_DIR_BLOCKS 8
#define DIRECTORY_ENTRY_SIZE 128
#define DIR_ENTRY_NUM_EACH_BLOCK 16
#define MAX_FILES 128

#define FILENAME_MAX_CHAR 30
// Superblock structure
struct superblock {
    unsigned int fs_size; // size of the file system in blocks
    unsigned int fat_start; // starting block of the FAT
    unsigned int root_dir_start; // starting block of the root directory
    // ... other fields as needed
};

// Directory entry structure
struct dir_entry {
    char filename[30];
    unsigned int file_size;
    unsigned int start_block;
    // ... other attributes as needed
};

typedef struct {
    int file_index;         // Index of the file in the root directory
    int mode;               // Mode in which the file is opened (read or append)
    int current_position;   // Current position in the file
    int is_free;            // Flag to indicate if this entry is free (1 if free, 0 if in use) for finding a place in the table.
} open_file_table_entry;

int vsformat (char *vdiskname, unsigned int m);

int vsmount (char *vdiskname);

int vsumount ();

int vscreate(char *filename);

int vsopen(char *filename, int mode);

int vsclose(int fd);

int vssize (int fd);

int vsread(int fd, void *buf, int n);

int vsappend(int fd, void *buf, int n);

int vsdelete(char *filename);


