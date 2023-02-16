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
#include <armadillo>
#include <thread>
#include <queue>

inline void err_exit(const char *buf) {
    fprintf(stderr, "%s\n", buf);
    exit(1);
}

inline void python_thread(std::string arg) {
    std::cout << "Run Python program:\n " << arg << "\n";
    int result = system(arg.c_str());
    std::cout << "Python return code: " << result << "\n"; 
}

class pySubProc {
private:
    /* data */
    char *create_shm(int &shmid, size_t __size, const char *__pathname, int __proj_id);
    void delete_shm();
    int create_dualpipe();

    const char * GETEKYDIR;
    int PROJECTID;
    size_t SHMSIZE;

    int shmid;
    char *addr;
    int pipefds1[2], pipefds2[2];
    std::thread * pt;

    bool busy;

public:
    pySubProc(size_t max_pcd_size, const char *__pathname, int __proj_id);
    ~pySubProc();
    int Launch(char *data, size_t size); // NOTE: currently only support single thread
};

inline pySubProc::pySubProc(size_t max_shm_size=100000, const char *__pathname="/tmp", int __proj_id=2333) : 
    GETEKYDIR(__pathname), PROJECTID(__proj_id), SHMSIZE(max_shm_size) {
    std::cout << "Create shared memory on " << GETEKYDIR << PROJECTID << " of size " << SHMSIZE << " Bytes.\n";
    addr = create_shm(shmid, SHMSIZE, GETEKYDIR, PROJECTID);

    if (create_dualpipe()) {
        err_exit("Fail to create pipes");
        close(pipefds1[0]); // Close the unwanted pipe1 read side
        close(pipefds2[1]); // Close the unwanted pipe2 write side
        close(pipefds1[1]); // Close the unwanted pipe1 read side
        close(pipefds2[0]); // Close the unwanted pipe2 write side
    }

    std::stringstream ss;
    ss << "python3 slave.py" << " " << pipefds1[0] << " " << pipefds2[1] << " " << PROJECTID;
    pt = new std::thread(python_thread, ss.str());

    busy = false;
}

inline pySubProc::~pySubProc() {

    size_t signal = 0;
    ssize_t wbytes = write(pipefds1[1], &signal, sizeof(signal)); // write 0 to signal an process exit
    pt->join();

    delete_shm();

    close(pipefds1[0]); // Close the unwanted pipe1 read side
    close(pipefds2[1]); // Close the unwanted pipe2 write side
    close(pipefds1[1]); // Close the unwanted pipe1 read side
    close(pipefds2[0]); // Close the unwanted pipe2 write side
}

inline int pySubProc::Launch(char *data, size_t size) {
    // NOTE: size in bytes
    if (busy) {
        std::cerr << "[Error] IPC launch currently only support single thread!\n";
        return -1;
    }
    if (size > SHMSIZE) {
        std::cerr << "[Error] data exceeds max shared memory size!\n";
        return 1;
    }
    std::memcpy(addr, data, size); // copy data to shared memory

    // std::string writemessage = std::to_string(size);
    char readmessage[20];
    
    ssize_t wbytes = write(pipefds1[1], &size, sizeof(size));
    printf("In Parent: Writing to pipe 1 - Message is \"%s\", length: %zu\n", &size, wbytes);
    
    ssize_t rbytes = read(pipefds2[0], readmessage, sizeof(readmessage)); // this call will block 
    printf("In Parent: Reading from pipe 2 - Message is \"%s\", length: %zu\n", readmessage, rbytes);

    std::memcpy(data, addr, size); // copy data to shared memory

    return 0;
}

inline char * pySubProc::create_shm(int &shmid, size_t __size, const char *__pathname, int __proj_id) {
    key_t key = ftok(__pathname, __proj_id);
    if ( key < 0 )
        err_exit("ftok error, try another project id");

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

inline void pySubProc::delete_shm() {
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

inline int pySubProc::create_dualpipe() {
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
