/*C code*/
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#define GETEKYDIR ("/tmp")
#define PROJECTID  (2333)
#define SHMSIZE (1024)

void err_exit(const char *buf) {
    fprintf(stderr, "%s\n", buf);
    exit(1);
}

char *create_shm(int &shmid, size_t __size, const char *__pathname, int __proj_id) {
    key_t key = ftok(__pathname, __proj_id);
    if ( key < 0 )
        err_exit("ftok error");

    shmid = shmget(key, __size, IPC_CREAT | IPC_EXCL | 0664);
    if ( shmid == -1 ) {
        if ( errno == EEXIST ) {
            printf("shared memeory already exist\n");
            shmid = shmget(key ,0, 0);
            printf("reference shmid = %d\n", shmid);
        } else {
            perror("errno");
            err_exit("shmget error");
        }
    }

    char *addr;

    /* Do not to specific the address to attach
     * and attach for read & write*/
    if ( (addr = (char *) shmat(shmid, 0, 0) ) == (void*)-1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1)
            err_exit("shmctl error");
        else {
            printf("Attach shared memory failed\n");
            printf("remove shared memory identifier successful\n");
        }

        err_exit("shmat error");
    }

    return addr;
}

void delete_shm(char *addr, int shmid) {
    // delete shared memory
    if ( shmdt(addr) < 0) 
        err_exit("shmdt error");

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        err_exit("shmctl error");
    else {
        printf("Finally\n");
        printf("remove shared memory identifier successful\n");
    }
}

int create_dualpipe(int pipefds1[2], int pipefds2[2]) {
    int returnstatus1, returnstatus2;
    
    returnstatus1 = pipe(pipefds1);
    
    if (returnstatus1 == -1) {
        printf("Unable to create pipe 1 \n");
        return 1;
    }
    returnstatus2 = pipe(pipefds2);
    
    if (returnstatus2 == -1) {
        printf("Unable to create pipe 2 \n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {

    int shmid;
    char *addr = create_shm(shmid, SHMSIZE, GETEKYDIR, PROJECTID);
    int pipefds1[2], pipefds2[2];
    char pipe1writemessage[20] = "Ready";
    char readmessage[20];

    if (create_dualpipe(pipefds1, pipefds2)) {
        err_exit("Fail to create pipes");
        close(pipefds1[0]); // Close the unwanted pipe1 read side
        close(pipefds2[1]); // Close the unwanted pipe2 write side
        close(pipefds1[1]); // Close the unwanted pipe1 read side
        close(pipefds2[0]); // Close the unwanted pipe2 write side
    }

    strcpy(addr, "Shared memory test\n"); // write to shared memory
    printf("In Parent: Writing to pipe 1 - Message is %s\n", pipe1writemessage);
    write(pipefds1[1], pipe1writemessage, sizeof(pipe1writemessage));

    // call python script to process
    // TODO: make it an independent thread
    stringstream ss;
    ss << "python3 slave.py" << " " << pipefds1[0] << " " << pipefds2[1];
    cout << ss.str() << endl;
    int result = system(ss.str().c_str());

    read(pipefds2[0], readmessage, sizeof(readmessage));
    printf("In Parent: Reading from pipe 2 - Message is %s\n", readmessage);
    cout << "Python return code: " << result << "\n"; 

    delete_shm(addr, shmid);
    
    return 0;
}

