#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 
#include <pthread.h>

#define QUITE_MESSAGE "/qser"
#define QUEUE_PERMISSIONS   0660
#define MAX_MESSAGES        10
#define MAX_MSG_SIZE        256
#define MSG_BUFFER_SIZE     (MAX_MSG_SIZE + 10)
static void *clientFE(void *arg_ptr);
static void *processInput(void *arg_ptr);

pthread_mutex_t mutex;
pthread_cond_t condVar;
struct mq_attr attr;
mqd_t qd_server, qd_client, terminate_message;   // queue descriptors

struct ThreadData {
    char* inputFileName;
    int threadNumber;
};

int processed_input_count;
int replyKey; // This is set by the part of your program that receives replies
//*****************
void getFileName(char *buffer, size_t bufferSize, const char *fname, int fileNumber) {
    if (buffer == NULL || fname == NULL) {
        fprintf(stderr, "Invalid argument to getFilename\n");
        return;
    }
    int written = snprintf(buffer, bufferSize, "%s%d.txt", fname, fileNumber);
    if (written < 0 || (size_t)written >= bufferSize) {
        fprintf(stderr, "Buffer size is too small for the filename\n");
    }
}
void serializeBinaryMessage(char *message, char *buffer) {
    memcpy(buffer, message, MAX_MSG_SIZE);
}

// Function to deserialize a byte array into a binary message
void deserializeBinaryMessage(char *buffer, char *message) {
    memcpy(message, buffer, MAX_MSG_SIZE);
}

//*****************
int main(int argc, char **argv) {
    processed_input_count = 0;
    replyKey = -1;
    int client_num = 0, dlevel = 0, vsize = 0;
    char *fname = NULL;
    char *mqname1 = NULL;
    char *mqname2 = NULL;
    // parsing
        int option;
        while ((option = getopt(argc, argv, "n:f:d:s:m:")) != -1) {
            switch (option) {
                case 'n':
                    client_num = atoi(optarg);
                    break;
                case 'f':
                    fname = optarg;
                    break;
                case 'd':
                    dlevel = atoi(optarg);
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

    if (client_num < 0 || dlevel <= 0 || vsize <= 0 || !fname || !mqname1|| !mqname2) {
        fprintf(stderr, "All options must be specified and positive integers where required.\n");
        exit(EXIT_FAILURE);
    }
    // -------------------InputFile storing
    char **inputFiles = malloc(client_num * sizeof(char*));
    int size = (vsize);
    for(int i = 0; i < client_num; i++){
        inputFiles[i] = malloc(size * sizeof(char));
        getFileName(inputFiles[i], size, fname, i+1);
        //printf("input f name is %s\n",inputFiles[i]);
    }
    char MQ1_QUEUE_NAME[MSG_BUFFER_SIZE];
    char MQ2_QUEUE_NAME[MSG_BUFFER_SIZE];
    strcpy(MQ1_QUEUE_NAME, mqname1);
    strcpy(MQ2_QUEUE_NAME, mqname2);
    // Create the client queue for receiving messages from server
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    if ((qd_client = mq_open(MQ2_QUEUE_NAME, O_RDONLY, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Client: mq_open (client)");
        exit(1);
    }
    if ((qd_server = mq_open(MQ1_QUEUE_NAME, O_WRONLY,QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Client: mq_open (server)");
        exit(1);
    }
    if ((terminate_message = mq_open(QUITE_MESSAGE, O_WRONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Client: mq_open (client)");
        exit(1);
    }

    if (client_num == 0){
        printf("Interactive Mode:\n");
        while( 1 ){
        printf("Enter: ");
            char inter[MSG_BUFFER_SIZE];
            fgets(inter, MAX_MSG_SIZE, stdin);// Remove newline character from the end if present
            inter[strcspn(inter, "\n")] = 0;
            if (strcmp(inter,"QUIT") == 0 || strcmp(inter, "quit") == 0){
                printf("client server is terminated\n");
                break;
            }
            else if (strcmp(inter,"QUITSERVER") == 0|| strcmp(inter,"quitserver")== 0){
                char q_mes[MSG_BUFFER_SIZE];
                strcpy(q_mes,inter);
                if (mq_send(terminate_message, q_mes, strlen(q_mes) + 1, 0) == -1) {
                    perror("Client: Not able to send message to server");
                    exit(1);
                }
                break;
            }
            else{
		     //snprintf(inter, sizeof(inter), "0 %s", inter); // error cikarabilir
                    char binary_message[MSG_BUFFER_SIZE];
                    serializeBinaryMessage(inter,binary_message);
                    printf("The read message :%s\n",inter);
                    printf("The sent message :%s\n",binary_message);
                    // Send message to server
                    if (mq_send(qd_server, binary_message, strlen(binary_message) + 1, 0) == -1) {
                        perror("Client: Not able to send message to server");
                        exit(1);
                    }
                    int mq_status = mq_getattr(qd_client, &attr);
                    if (mq_status == -1) {
                        perror("Error getting message queue attributes");
                        exit(EXIT_FAILURE);
                    }
                    sleep(1);
                    char response[MSG_BUFFER_SIZE];
			// Get the next message from the queue
			if (mq_receive(qd_client, response, MSG_BUFFER_SIZE, NULL) == -1) {
			    perror("Server: mq_receive");
			    continue;
			}
			//printf("response: %s\n",response);
                    
            }
        }
    }
    else{
    // Create an array of worker thread handles and worker thread data.
        pthread_t tids[client_num+1];
        struct ThreadData t_args[client_num+1];
        char *retmsg;
        //MUTEX and CONDITION
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&condVar, NULL);
        // -----------------------------------------Create and start worker threads.
        for (int i = 0; i < client_num; i++) {
            t_args[i].inputFileName = inputFiles[i];/* assign intermediate file name */
            t_args[i].threadNumber = i;
            int ret = pthread_create(&(tids[i]), NULL, processInput, (void*)&t_args[i]);
            if (ret != 0) {
                printf("thread create failed \n");
                exit(1);
            }
        }
        // -----------------------------------------FE thread -> tids[client_num]
        t_args[client_num].inputFileName = "";
        t_args[client_num].threadNumber = client_num;
        int e = pthread_create(&(tids[client_num]), NULL, clientFE, (void*)&t_args[client_num]);
        if (e != 0) {
            printf("thread create failed \n");
            exit(1);
        }
        for (int i = 0; i < client_num ; i++) {
            int ret = pthread_join(tids[i], (void **)&retmsg); 
            if (ret != 0) {
                printf("thread join failed \n");
                exit(1);
            }
            free (retmsg);
        }
        for(int i = 0; i < client_num; i++) {
        free(inputFiles[i]);
        }
        free(inputFiles);
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&condVar);
    }
    


          
    for(int i = 0; i < client_num; i++) {
        free(inputFiles[i]);
    }
    free(inputFiles);      
    mq_close(terminate_message);
    mq_close(qd_client);
    mq_close(qd_server);
    mq_unlink(QUITE_MESSAGE);

    return 0;
}

static void *clientFE(void *arg_ptr){


    int clientNum = ((struct ThreadData *) arg_ptr)->threadNumber; // Cast to int* and dereference
    int is_queue_empty = 0;
        while(processed_input_count < clientNum || is_queue_empty != 1){ //until all the files are processed
            int mq_status = mq_getattr(qd_client, &attr);
            if (mq_status == -1) {
                perror("Error getting message queue attributes");
                exit(EXIT_FAILURE);
            }
            if (attr.mq_curmsgs == 0) {
                // Message queue is empty
                is_queue_empty = 1;
                continue; // Exit the loop if the queue is empty
            }
            else{
                is_queue_empty = 0;
                
                char in_buffer[MSG_BUFFER_SIZE];// Receive the reply from the server
                char bk[MSG_BUFFER_SIZE];// Receive the reply from the server
                if (mq_receive(qd_client, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
                    perror("Client: mq_receive");
                    exit(1);
                }
                int c_key = 0;
                snprintf(in_buffer, strlen(in_buffer)+1, "%d %s", c_key, bk);
                printf("Client: message received from server: %s\n", in_buffer);
                // deserialize
                //deserialize_item(itemp2, bufferp2);
                replyKey = c_key;
            }
        }
    terminate_message = mq_open("termination_queue", O_RDWR | O_CREAT, 0666, NULL );
    int wakeup_message = 1;
    mq_send(terminate_message, (char *)&wakeup_message, sizeof(wakeup_message) , 0);
    mq_close(terminate_message);
    sleep(1); // Bu dogru mu
    
}
static void *processInput(void *arg_ptr){

    FILE *fp = fopen(((struct ThreadData *) arg_ptr)->inputFileName, "a+"); // check a+
    if (fp == NULL) {
        perror("do_task:");
        exit(1);
    }
    char line[MSG_BUFFER_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        // If the thread is awake :
        char requestType[4]; // "PUT", "DEL", or "GET"
        int key;
        int L = 50;
        char value[L]; // Array for storing values
        if (sscanf(line, "%s %d %s", requestType, &key, value) >= 2) {
            /*// Handle the request
            itemp1->clieantThreadID = ((struct ThreadData *)arg_ptr)->threadNumber;
            strcpy(*itemp1->command, requestType);
            itemp1->key = key;
            if (strcmp(requestType, "PUT") == 0) {
                // Initialize the whole array to NULL
                char fullValue[L];
                memset(fullValue, '\0', L);
                strcpy(fullValue, value);
                strcpy(*itemp1->value, fullValue);
                //printf("PUT request: Key = %d, Value = %s\n", key, fullValue);
            } /*else if (strcmp(requestType, "DEL") == 0) {
                printf("DEL request: Key = %d\n", key);
                // Handle delete logic here
            } else if (strcmp(requestType, "GET") == 0) {
                printf("GET request: Key = %d\n", key);
                // Handle get logic here
            }*/ 
        } else {
            printf("Failed to parse line: %s", line);
        }
        pthread_mutex_lock(&mutex);
        // Logic to send request to MQ1
       // serialize_item(itemp1, bufferp1);

        int n = mq_send(qd_server, line, strlen(line) + 1, 0);
        if (n == -1) {
            perror("mq_send failed\n");
            exit(1);
        }
        // Wait until the request arrives
        while (replyKey != ((struct ThreadData *)arg_ptr)->threadNumber) {
            pthread_cond_wait(&condVar, &mutex);
        }
        // After receiving the reply (signaled by FE)
        pthread_mutex_unlock(&mutex);
    }
    fclose(fp);
    processed_input_count++;
   // mq_close(mq1);
    pthread_exit(NULL); //  tell a reason to th
}
