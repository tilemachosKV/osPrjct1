#include <stdio.h>           // Standard I/O: Used for printf, scanf, and perror
#include <stdlib.h>          // Standard Library: Used for exit()
#include <string.h>          // String Operations: Used for strcpy, strncmp
#include <fcntl.h>           // File Control: Defines O_CREAT, O_RDWR constants
#include <sys/stat.h>        // File Status: Defines modes/permissions (for 0666)
#include <sys/mman.h>        // Memory Management: shm_open, mmap, munmap, MAP_SHARED
#include <unistd.h>          // Unix Standard: ftruncate, close
#include "sharedMemory.h"

// checks whether the user input is correct
int check_input(int argc, char* argv[], const char *params[]) {
  if (argc != 4) return 0;
  if (strcmp(argv[1], params[0]) != 0 && strcmp(argv[1], params[1]) != 0) return 0;
  if (strcmp(argv[2], params[2]) != 0) return 0;

  return 1;
}

void init(int argc, char* argv[]) {
  const char* name = "my_shm"; // Name of the shared memory object
  const int SIZE = 4096;      // Size of the shared memory
  char message[128];

  int shm_fd;   // Shared memory file descriptor
  void* ptr;    // Pointer to the shared memory

  SharedData* someMemory;

  // 1. Create the shared memory object
  shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(1);
  }

  // 2. Set the size of the shared memory
  ftruncate(shm_fd, SIZE);

  // 3. Map the shared memory object into our address space
  ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  someMemory = (SharedData*)ptr;
  sem_init(&someMemory->newMessage, 1, 0);

  // 4. Write to the shared memory
  // printf("Writing to shared memory: '%s'\n", message);
  printf("Enter messages to write (type 'DONE' to quit):\n");
  while (strncmp(message, "DONE", 5) != 0) { // Use 5 to check "DONE\0"
    printf("NEW MESSAGE: ");
    fflush(stdout);

    int scan_result = scanf("%127s", message);
    if (scan_result != 1) {
      // Handle End-of-File (Ctrl+D) or read error
      strcpy(someMemory->data, "DONE"); // Write DONE as last message
      sem_post(&someMemory->newMessage);
      break;
    }

    strcpy(someMemory->data, message);
    sem_post(&someMemory->newMessage);
  } 

  printf("Exiting.\n");

  // 5. Unmap the shared memory
  munmap(ptr, SIZE);
  
  // 6. Close the file descriptor
  close(shm_fd);
}

void attach(int argc, char* argv[]) {
  const char* name = "my_shm";
  const int SIZE = 4096;
  int shm_fd;
  void* ptr;

  SharedData* someMemory;

  // 1. Open the shared memory object
  shm_fd = shm_open(name, O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open"); 
    exit(1);
  }

  // 2. Map the shared memory object
  ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  someMemory = (SharedData*)ptr;

  // 3. Read from the shared memory
  while (strncmp(someMemory->data, "DONE", 4) != 0) {
    sem_wait(&someMemory->newMessage);
    printf("MESSAGE: '%s'\n", someMemory->data);
  }
  printf("Last read from shared memory: '%s'\n", someMemory->data);

  // 4. Unmap the shared memory
  munmap(ptr, SIZE);

  // 5. Close the file descriptor
  close(shm_fd);
  
  // 6. Remove the shared memory object
  // Only the reader (or a dedicated cleanup process) should do this
  shm_unlink(name);
}

int main(int argc, char* argv[]) {
  const char *params[] = {"-i", "-a", "-k"};

  if (!check_input(argc, argv, params)) {
    printf("WRONG INPUT\nTry the bellow:\n./chat -i -k your_key   OR   ./chat -a -k your_key\n");
    printf("Exiting...\n");
    return -1;
  }

  if (strcmp(argv[1], params[0]) == 0) {
    init(argc, argv);
  }else {
    attach(argc, argv);
  }

  return 0;
}
