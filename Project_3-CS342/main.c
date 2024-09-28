#include <stdio.h>
#include "vsfs.h" // Assuming this is the header file where your functions are declared
#include <string.h>

// Constants for testing
#define VDISK_NAME "test_vdisk.bin"
#define DISK_SIZE_M 18 // Example disk size parameter Should be more than 11!


int main() {
    // Format and mount the file system
    char vdiskname[] = "vsfs_disk.img";
    unsigned int m = 20; // Example size parameter
    if (vsformat(vdiskname, m) != 0) {
        printf("Formatting failed\n");
        return 1;
    }
    if (vsmount(vdiskname) != 0) {
        printf("Mounting failed\n");
        return 1;
    }

    // Create a new file
    char filename[] = "testfile.txt";
    if (vscreate(filename) != 0) {
        printf("File creation failed\n");
    } else {
        printf("File created: %s\n", filename);
    }

    // Open the file for appending
    int fd = vsopen(filename, MODE_APPEND);
    if (fd < 0) {
        printf("Opening file for appending failed\n");
    } else {
        printf("File opened for appending. FD: %d\n", fd);
    }

    // Append data to the file
    char data[] = "Hello, VSFS!";
    int bytes_appended = vsappend(fd, data, strlen(data));
    if (bytes_appended < 0) {
        printf("Appending to file failed\n");
    } else {
        printf("Appended %d bytes to the file\n", bytes_appended);
    }
     // Append data to the file
    char data2[] = "Hello, ALY!";
    int bytes_appended2 = vsappend(fd, data2, strlen(data2));
    if (bytes_appended2 < 0) {
        printf("Appending to file failed\n");
    } else {
        printf("Appended %d bytes to the file\n", bytes_appended2);
    }
     // Append data to the file
    char data3[] = "BYE, VSFS!";
    int bytes_appended3 = vsappend(fd, data3, strlen(data3));
    if (bytes_appended3 < 0) {
        printf("Appending to file failed\n");
    } else {
        printf("Appended %d bytes to the file\n", bytes_appended3);
    }
     // Append data to the file
    char data4[] = "BYE, ALY!";
    int bytes_appended4 = vsappend(fd, data4, strlen(data4));
    if (bytes_appended4 < 0) {
        printf("Appending to file failed\n");
    } else {
        printf("Appended %d bytes to the file\n", bytes_appended4);
    }
    //FAT print
    printf("Printing FAT\n");
    printFAT();

    // Close the file
    if (vsclose(fd) != 0) {
        printf("Closing file failed\n");
    } else {
        printf("File closed\n");
    }

    // Open the file for reading
    fd = vsopen(filename, MODE_READ);
    if (fd < 0) {
        printf("Opening file for reading failed\n");
    } else {
        printf("File opened for reading. FD: %d\n", fd);
    }

    // Get file size
    int size = vssize(fd);
    printf("File size: %d bytes\n", size);

    // Read from the file
    char buffer[100];
    int bytes_read = vsread(fd, buffer, sizeof(buffer));
    if (bytes_read < 0) {
        printf("Reading from file failed\n");
    } else {
        printf("Read %d bytes from the file: %s\n", bytes_read, buffer);
    }
    
    //FAT print
    printf("Printing FAT\n");
    printFAT();

    // Close the file
    if (vsclose(fd) != 0) {
        printf("Closing file failed\n");
    } else {
        printf("File closed\n");
    }

    // Delete the file
    if (vsdelete(filename) != 0) {
        printf("Deleting file failed\n");
    } else {
        printf("File deleted: %s\n", filename);
    }
    
    
    //FAT print
    printf("Printing FAT\n");
    printFAT();

    // Unmount the file system
    if (vsumount() != 0) {
        printf("Unmounting failed\n");
        return 1;
    }

    return 0;
}


/*
int main() {
    int result;

    // Test vsformat
    printf("Testing vsformat...\n");
    result = vsformat(VDISK_NAME, DISK_SIZE_M);
    if (result != 0) {
        printf("vsformat failed\n");
        return -1;
    }
    printf("vsformat succeeded\n");

    // Test vsmount
    printf("Testing vsmount...\n");
    result = vsmount(VDISK_NAME);
    if (result != 0) {
        printf("vsmount failed\n");
        return -1;
    }
    printf("vsmount succeeded\n");

    // Test vsumount
    printf("Testing vsumount...\n");
    result = vsumount();
    if (result != 0) {
        printf("vsumount failed\n");
        return -1;
    }
    printf("vsumount succeeded\n");

    printf("All tests passed!\n");
    return 0;
}

}*/
