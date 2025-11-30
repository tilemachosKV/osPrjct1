#include <stdio.h>        // Standard I/O: Used for printf, scanf, and perror
#include <stdlib.h>       // Standard Library: Used for exit()
#include <string.h>       // String Operations: Used for strcpy, strncmp
#include <unistd.h>       // Unix Standard: ftruncate, close, STDOUT_FILENO
#include <fcntl.h>        // File Control: Defines O_CREAT, O_RDWR constants
#include <sys/stat.h>     // File Status: Defines modes/permissions (for 0666)
#include <sys/mman.h>     // Memory Management: shm_open, mmap, munmap, MAP_SHARED
#include <sys/types.h>    // Required for pid_t definition
#include <sys/ioctl.h>    // Required to get terminal width
#include "sharedMemory.h" // Shared Memory struct
#include <pthread.h>      // For thread use

#define SCREEN_WIDTH 80

// checks whether the user input is correct
int checkInput(int argc, char *argv[], const char *params[])
{
  if (argc != 4)
    return 0;
  if (strcmp(argv[1], params[0]) != 0 && strcmp(argv[1], params[1]) != 0)
    return 0;
  if (strcmp(argv[2], params[2]) != 0)
    return 0;

  return 1;
}

void printMessage(char *name, char *msg, int is_me)
{
  struct winsize w;
  // Get current terminal width
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  int width = w.ws_col;

  if (is_me)
  {
    // RIGHT ALIGN
    // %*s takes a width argument and pads the string with spaces to the left
    printf("%*s\n", width, name);
    printf("%*s\n", width, msg);
  }
  else
  {
    // LEFT ALIGN
    printf("%s\n", name);
    printf("%s\n", msg);
  }

  printf("\n");
}

void terminate(SharedMemory *sharedMemory, char *name, int shm_fd, int dialogueId)
{
  int lastProcess = 0;
  pid_t mypid = getpid();

  if (sharedMemory->active_dialogues_count == 1 && mypid == sharedMemory->dialogues[dialogueId].message.sender_pid)
    lastProcess++;

  munmap(sharedMemory, SHM_SIZE);
  close(shm_fd);
  if (lastProcess)
  {
    printf("shared memory final cleanup done\n");
    shm_unlink(name);
  }
}

SharedMemory *init(int argc, char *argv[], int *shm_fd)
{
  const char *name = argv[3];
  void *ptr; // Pointer to the shared memory

  SharedMemory *someMemory;

  // 1. Create the shared memory object
  *shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  if (*shm_fd == -1)
  {
    perror("shm_open");
    exit(1);
  }

  // 2. Set the SHM_SIZE of the shared memory
  ftruncate(*shm_fd, SHM_SIZE);

  // 3. Map the shared memory object into our address space
  ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
  if (ptr == MAP_FAILED)
  {
    perror("mmap");
    exit(1);
  }

  someMemory = (SharedMemory *)ptr;
  
  // Initialize shared memory to zero
  memset(someMemory, 0, SHM_SIZE);
  sem_init(&someMemory->shm_sem, 1, 1);
  someMemory->active_dialogues_count = 1;

  // Init dialoge
  someMemory->dialogues[0].dialogue_id = someMemory->active_dialogues_count;
  someMemory->dialogues[0].num_active_participants = 1;

  // Init message
  someMemory->dialogues[0].message.sender_pid = getpid();
  sem_init(&someMemory->dialogues[0].message.access, 1, 0);
  someMemory->dialogues[0].message.times_read = 0;

  return someMemory;

  // // 4. Write to the shared memory
  // printf("Enter messages to write (type 'DONE' to quit):\n");
  // while (strncmp(userInput, "DONE", 5) != 0)
  // { // Use 5 to check "DONE\0"
  //   printf("NEW MESSAGE: ");
  //   fflush(stdout);

  //   int scan_result = scanf("%127s", userInput);
  //   if (scan_result != 1)
  //   {
  //     // Handle End-of-File (Ctrl+D) or read error
  //     break;
  //   }

  //   strcpy(someMemory->dialogues[0].message.payload, userInput);
  //   someMemory->dialogues[0].message.times_read = 0;
  //   sem_post(&someMemory->dialogues[0].message.access);
  // }

  // // 5. Unmap the shared memory
  // munmap(ptr, SHM_SIZE);

  // // 6. Close the file descriptor
  // close(shm_fd);
}

SharedMemory *attach(int argc, char *argv[], int *shm_fd)
{
  const char *name = argv[3];
  void *ptr;

  // 1. Open the shared memory object
  *shm_fd = shm_open(name, O_RDWR, 0666);
  if (*shm_fd == -1)
  {
    perror("shm_open");
    exit(1);
  }

  // 2. Map the shared memory object
  ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
  if (ptr == MAP_FAILED)
  {
    perror("mmap");
    exit(1);
  }

  return (SharedMemory *)ptr;

  // // 3. Read from the shared memory
  // while (strncmp(someMemory->dialogues[0].message.payload, "DONE", 4) != 0)
  // {
  //   sem_wait(&someMemory->dialogues[0].message.access);
  //   printf("MESSAGE: '%s'\n", someMemory->dialogues[0].message.payload);
  // }
  // printf("Last read from shared memory: '%s'\n", someMemory->dialogues[0].message.payload);

  // // 4. Unmap the shared memory
  // munmap(ptr, SHM_SIZE);

  // // 5. Close the file descriptor
  // close(shm_fd);

  // // 6. Remove the shared memory object
  // // Only the reader (or a dedicated cleanup process) should do this
  // shm_unlink(name);
}

void *threadReceive(void *arg)
{
  SharedMemory *sharedMemory = (SharedMemory *)arg;

  while (strncmp(sharedMemory->dialogues[0].message.payload, "DONE", 5) != 0)
  {
    sem_wait(&sharedMemory->dialogues[0].message.access);
    printf("MESSAGE: '%s'\n", sharedMemory->dialogues[0].message.payload);
  }
  printf("Last read from shared memory: '%s'\n", sharedMemory->dialogues[0].message.payload);

  return NULL;
}

void *threadSend(void *arg)
{
  SharedMemory *sharedMemory = (SharedMemory *)arg;
  char userInput[128];

  printf("Enter messages to write (type 'DONE' to quit):\n");
  while (strncmp(userInput, "DONE", 5) != 0)
  { // Use 5 to check "DONE\0"
    printf("NEW MESSAGE: ");
    fflush(stdout);

    int scan_result = scanf("%127s", userInput);
    if (scan_result != 1)
    {
      // Handle End-of-File (Ctrl+D) or read error
      break;
    }

    strcpy(sharedMemory->dialogues[0].message.payload, userInput);
    sharedMemory->dialogues[0].message.times_read = 0;
    sem_post(&sharedMemory->dialogues[0].message.access);
    sem_post(&sharedMemory->dialogues[0].message.access);
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  int shm_fd, dialogue = 0;
  const char *params[] = {"-i", "-a", "-k"};
  SharedMemory *sharedMemory;
  pthread_t receiver, sender;

  if (!checkInput(argc, argv, params))
  {
    printf("WRONG INPUT\nTry the bellow:\n./chat -i -k your_key   OR   ./chat -a -k your_key\n");
    printf("Exiting...\n");
    return -1;
  }

  sharedMemory = strcmp(argv[1], params[0]) == 0 ? init(argc, argv, &shm_fd) : attach(argc, argv, &shm_fd);

  pthread_create(&receiver, NULL, threadReceive, sharedMemory);
  pthread_create(&sender, NULL, threadSend, sharedMemory);
  pthread_join(sender, NULL);
  pthread_join(receiver, NULL);

  terminate(sharedMemory, argv[3], shm_fd, dialogue);
  printf("Chat closed, byeee!\n");
  return 0;
}
