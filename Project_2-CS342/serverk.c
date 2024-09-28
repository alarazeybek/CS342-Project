#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 
#include <pthread.h>

#define QUITE_MESSAGE "/nser"
#define QUEUE_PERMISSIONS   0660
#define MAX_MESSAGES        10
#define MAX_MSG_SIZE        256
#define MSG_BUFFER_SIZE     (MAX_MSG_SIZE + 10)


struct ThreadData {
    //char** inputFileNames;
    int threadNumber;
    int d_count;
    int t_count;
    char filename[MAX_MSG_SIZE];
};

struct item {
    int clientThreadID;
    int key;
    char command[4];
    char value[20];
};

struct answer_item {
    int clientThreadID;
    int is_put;
    int is_del;
    char get_val[20];
};

struct buffer {
    int count;
    int in;
    int out;
    struct item data[MSG_BUFFER_SIZE];
    pthread_mutex_t mutex_q;
    pthread_cond_t xp;
    pthread_cond_t xc;
};

pthread_mutex_t mutex;
pthread_cond_t condVar;
struct buffer buf;
int workerKey;
int order_count;
int program_executes;
mqd_t qd_server, qd_client, quit_message;    // queue descriptors


static void *workerThreadJob(void *arg_ptr);
void buf_add(struct buffer *bp, char *srcBuffer) ;
int buf_rem(struct buffer *bp, struct item *itemp);
int hashFunc(int k, int D);
static void *readMessageQueue(void *arg_ptr);

int main(int argc, char **argv) {
    program_executes = 1;
    workerKey = -1;
    order_count = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condVar, NULL);
    buf.count = 0;
    buf.in = 0;
    buf.out = 0;
    pthread_mutex_init(&buf.mutex_q, NULL);
    pthread_cond_init(&buf.xp, NULL);
    pthread_cond_init(&buf.xc, NULL);

    int dcount = 0, tcount = 0, vsize = 0;
    char *fname = NULL;
    char *mqname1 = NULL;
    char *mqname2 = NULL;
    // parsing
        int option;
        while ((option = getopt(argc, argv, "d:f:t:s:m:")) != -1) {
            switch (option) {
                case 'd':
                    dcount = atoi(optarg);
                    break;
                case 'f':
                    fname = optarg;
                    break;
                case 't':
                    tcount = atoi(optarg);
                    break;
                case 's':
                    vsize = atoi(optarg);
                    break;
                case 'm':
                    static char mqname_buffer[sizeof(optarg) + 3];
		            snprintf(mqname_buffer, sizeof(mqname_buffer), "/%s1", optarg);
		            mqname1 = mqname_buffer;
                    static char mqname_buffer2[sizeof(optarg) + 3];
		            snprintf(mqname_buffer2, sizeof(mqname_buffer2), "/%s2", optarg);
		            mqname2 = mqname_buffer2;
                    break;
                default:
                    fprintf(stderr, "Usage: %s -d dcount -f fname -t tcount -s vsize -m mqname\n", argv[0]);
                    exit(EXIT_FAILURE);
            }
        }

    if (dcount <= 0 || tcount <= 0 || vsize <= 0 || !fname || !mqname1|| !mqname2) {
        fprintf(stderr, "All options must be specified and positive integers where required.\n");
        exit(EXIT_FAILURE);
    }
    //**********Data Files***********
        int size = vsize;
        char **inputFiles = malloc(dcount * sizeof(char *));
        for (int i = 0; i < dcount; i++) {
            static char file_name[MAX_MSG_SIZE];
            snprintf(file_name, sizeof(file_name), "%s%d.txt", fname, i+1);
            FILE *file = fopen(file_name, "ab+");
            if (file == NULL) {
                perror("Error opening file");
                exit(1);
            }
            fclose(file);
            int f_size = size * sizeof(char);
            inputFiles[i] = malloc(f_size);
            strcpy(inputFiles[i], file_name);
        }
    char MQ1_QUEUE_NAME[MSG_BUFFER_SIZE]; // MAX_MSG_SIZE yapilabilri
    char MQ2_QUEUE_NAME[MSG_BUFFER_SIZE];
    strcpy(MQ1_QUEUE_NAME, mqname1);
    strcpy(MQ2_QUEUE_NAME, mqname2);
    // Create the server queue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_server = mq_open(MQ1_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Server: mq_open (server)");
        exit(1);
    }
	if ((quit_message = mq_open(MQ2_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1){
        perror("Server: mq_open (server)");
        exit(1);
    }
    if ((qd_client = mq_open(MQ2_QUEUE_NAME, O_WRONLY| O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1){
        perror("Server: mq_open (server)");
        exit(1);
    }

    char out_buffer[MSG_BUFFER_SIZE];
    int thread_num = tcount;
    pthread_t tids[thread_num + 1];
    struct ThreadData t_args[thread_num + 1];
    int ret;
    
    t_args[thread_num].threadNumber = thread_num;
    t_args[thread_num].d_count = dcount;
    t_args[thread_num].t_count = tcount;
    strcpy(t_args[thread_num].filename , fname);
    ret = pthread_create(&(tids[thread_num]), NULL, readMessageQueue, (void *)&t_args[thread_num]);
    if (ret != 0) {
        printf("thread create failed \n");
        exit(1);
    }
    for (int i = 0; i < thread_num; i++) {
        t_args[i].threadNumber = i;
        t_args[i].d_count = dcount;
        t_args[i].t_count = tcount;
        strcpy(t_args[i].filename , fname);
        ret = pthread_create(&(tids[i]), NULL, workerThreadJob, (void *)&t_args[i]);
        if (ret != 0) {
            printf("thread create failed \n");
            exit(1);
        }
    }


    // Wait for worker threads to finish
    for (int i = 0; i < thread_num; i++) {
        pthread_join(tids[i], NULL);
        if (ret != 0) {
            printf("thread join failed\n");
            exit(1);
        }
    }

    // Clean up resources
    for (int i = 0; i < dcount; i++) {
        free(inputFiles[i]);
    }
    free(inputFiles);
    mq_close(qd_server);
    mq_close(qd_client);
    mq_close(quit_message);
    mq_unlink(QUITE_MESSAGE);
    mq_unlink(MQ1_QUEUE_NAME);
    mq_unlink(MQ2_QUEUE_NAME);

    return 0;
}

static void *readMessageQueue(void *arg_ptr){
    
    struct ThreadData *t_args = (struct ThreadData *)arg_ptr;
    while (1) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    
    char qm[MSG_BUFFER_SIZE];
        int mq_status = mq_getattr(quit_message, &attr);
        if (mq_status == -1) {
            perror("Error getting message queue attributes");
            exit(EXIT_FAILURE);
        }
        if (attr.mq_curmsgs != 0) {
            program_executes = 0;
            break;
        }
	/*if (mq_receive(quit_message, qm, MSG_BUFFER_SIZE, NULL) == -1) {
            perror("Error receiving quit message");
            continue;
        }

        if (strcmp(qm, "QUITSERVER") == 0 || strcmp(qm, "quitserver") == 0) {
            printf("Quit signal received. Shutting down server.\n");
            program_executes = 0;
            break;
        }*/

        // Get the next message from the queue
        char in_buffer[MSG_BUFFER_SIZE];
        if (mq_receive(qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror("Server: mq_receive");
            continue;
        }
        printf("Server: message received. message: %s\n",in_buffer);
        // Parse the client queue name from the message
        char temp_buf[MAX_MSG_SIZE];
        sscanf(in_buffer, "%s", temp_buf);
char va[MAX_MSG_SIZE];char com[MAX_MSG_SIZE];
int ct = 0;
int kk = 0;
sscanf(temp_buf, "%d %s %d %s", &ct,com, &kk, va);
        workerKey = hashFunc(kk, t_args->d_count) ;
        order_count % t_args->t_count;
        order_count++;
        buf_add(&buf, in_buffer);
        pthread_cond_broadcast(&condVar);
        printf("A worker thread is assigned.\n");
    }
}

static void *workerThreadJob(void *arg_ptr) {
    struct ThreadData *t_args = (struct ThreadData *)arg_ptr;

    int id = t_args->threadNumber;
    int d_count = t_args->d_count;

    while (program_executes == 1) {
        pthread_mutex_lock(&mutex);

        while (id != workerKey) {
            pthread_cond_wait(&condVar , &mutex);
        }

        struct item temp_item;
        int res = buf_rem(&buf, &temp_item);

        if (res != 0) {
            perror("Error in reading from buffer");
            exit(1);
        }


        int file_idx = id + 1;
        
        char f_name_buf[MAX_MSG_SIZE];
        snprintf(f_name_buf, sizeof(f_name_buf)+4, "%s%d.txt", t_args->filename, file_idx);


        //printf("file name %s  hash apres index %d\n", f_name_buf, file_idx);
        if (1) {
        static char serialized_answer[MAX_MSG_SIZE];
        printf("command %s\n", (temp_item.command));
            if (strcmp(temp_item.command, "PUT") == 0) {
                FILE *file = fopen(
                    f_name_buf
                    , "ab+");
                if (file == NULL) {
                    perror("Error opening file");
                    exit(1);
                }
                fprintf(file, "%d %s\n", temp_item.key, temp_item.value);
                fclose(file);
                snprintf(serialized_answer, sizeof(f_name_buf), "%d %s", id, "");
            } else if (strcmp(temp_item.command, "DEL") == 0) {
                FILE *file = fopen(f_name_buf, "r+");
                if (file == NULL) {
                    perror("Error opening file");
                    exit(1);
                }
                FILE *tempFile = fopen("temp.txt", "w+");
                if (tempFile == NULL) {
                    perror("Error opening file");
                    exit(1);
                }
                int key_to_delete = temp_item.key;
                char line[50];
                while (fgets(line, sizeof(line), file)) {
                    int key;
                    char value[20];
                    sscanf(line, "%d %s", &key, value);
                    if (key != key_to_delete) {
                        fprintf(tempFile, "%d %s\n", key, value);
                    }
                }
                fclose(file);
                fclose(tempFile);
                remove(f_name_buf);
                rename("temp.txt", f_name_buf);
                snprintf(serialized_answer, sizeof(f_name_buf), "%d %s", id, "");
            } else if (strcmp(temp_item.command, "GET") == 0) {
                FILE *file = fopen(f_name_buf, "r");
                if (file == NULL) {
                    perror("Error opening file");
                    exit(1);
                }
                int key_to_find = temp_item.key;
                char value_to_send[20];
                strcpy(value_to_send, "NOT_FOUND");
                char line[50];
                while (fgets(line, sizeof(line), file)) {
                    int key;
                    char value[20];
                    sscanf(line, "%d %s", &key, value);
                    if (key == key_to_find) {
                        strcpy(value_to_send, value);
                        snprintf(serialized_answer, sizeof(f_name_buf), "%d %s", id, value);
                        break;
                    }
                }
                fclose(file);
            }
                // Send the response back to the client
                /*struct answer_item answer;
                answer.clientThreadID = id;
                answer.is_put = 0;
                answer.is_del = 0;
                strcpy(answer.get_val, value_to_send); */
                //serialize_item(&answer, serialized_answer);
                
		if (qd_client == (mqd_t)-1) {
            perror("Error: client message queue not open");
            exit(EXIT_FAILURE);
        }
	char out_buffer[MSG_BUFFER_SIZE];
	strcpy(out_buffer, "answer");
        int mq_status = mq_send(qd_client, out_buffer, strlen(out_buffer) + 1, 0);
        if (mq_status == -1) {
            perror("Error sending response to worker MQ");
            exit(1);
        }

        pthread_mutex_unlock(&mutex);}
    }
    pthread_exit(NULL);
}

 void buf_add(struct buffer *bp, char *srcBuffer) {
     pthread_mutex_lock(&(bp->mutex_q));
    while (bp->count == MSG_BUFFER_SIZE)
        pthread_cond_wait(&(bp->xp), &(bp->mutex_q));

    struct item data_item;
    // Adjust the sscanf format to match "command key value"
    if (sscanf(srcBuffer, "%3s %d %19s", data_item.command, &data_item.key, data_item.value) != 3) {
    	 if (sscanf(srcBuffer, "%3s %d", data_item.command, &data_item.key) != 2){
		fprintf(stderr, "Failed to parse buffer data: %s\n", srcBuffer);
		pthread_mutex_unlock(&(bp->mutex_q));
		return;
        }
    }

    // Copy data_item to the buffer
    bp->data[bp->in].clientThreadID = 0; // Assuming clientThreadID is set elsewhere or not needed
    bp->data[bp->in].key = data_item.key;
    strcpy(bp->data[bp->in].command, data_item.command);
    strcpy(bp->data[bp->in].value, data_item.value);
    bp->in = (bp->in + 1) % MSG_BUFFER_SIZE;
    bp->count++;

    if (bp->count == 1)
        pthread_cond_signal(&(bp->xc));

    pthread_mutex_unlock(&(bp->mutex_q));
}

int buf_rem(struct buffer *bp, struct item *itemp) {
    pthread_mutex_lock(&(bp->mutex_q));

    while (bp->count == 0)
        pthread_cond_wait(&(bp->xc), &(bp->mutex_q));

    // Retrieve an item from the buffer
    itemp->clientThreadID = bp->data[bp->out].clientThreadID;
    itemp->key = bp->data[bp->out].key;
    strcpy(itemp->command, bp->data[bp->out].command);
    strcpy(itemp->value, bp->data[bp->out].value);
    bp->out = (bp->out + 1) % MSG_BUFFER_SIZE;
    bp->count--;

    if (bp->count == (MSG_BUFFER_SIZE - 1)) {
        // Wake up all possible workers
        pthread_cond_broadcast(&(bp->xp));
    }

    pthread_mutex_unlock(&(bp->mutex_q));

    return 0; // 0 indicates success
}

int hashFunc(int k, int D) {
    return k % D;
}
