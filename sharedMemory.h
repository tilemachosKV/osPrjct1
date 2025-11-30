#include <semaphore.h>

#define SHM_SIZE 4096
#define DLG_NUMBER 5
#define MSG_SIZE 128

typedef struct
{
    char payload[MSG_SIZE];
    pid_t sender_pid;
    sem_t access;
    int times_read;
} Message;

typedef struct
{
    int dialogue_id;
    int num_active_participants;
    Message message;
} Dialogue;

typedef struct
{
    Dialogue dialogues[DLG_NUMBER];
    int active_dialogues_count;
    sem_t shm_sem;
} SharedMemory;