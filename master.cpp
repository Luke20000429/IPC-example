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
#include <armadillo>
#include <thread>
#include <chrono>

#include "pySubProc.hpp"

using namespace std;

#define GETEKYDIR ("/tmp")
#define PROJECTID  (2333)
#define SHMSIZE (1024000)

#define CurrTimeMS (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#define CurrTimeNS (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

int main(int argc, char **argv) {

    arma::fmat mat1(12000, 4);
    mat1.fill(12);

    auto t1 = CurrTimeMS;

    pySubProc flot(SHMSIZE, GETEKYDIR, PROJECTID);
    flot.Launch((char *) mat1.memptr(), mat1.size() * sizeof(float));
    cout << mat1(0, 0) << endl;
    flot.Launch((char *) mat1.memptr(), mat1.size() * sizeof(float));
    cout << mat1(0, 0) << endl;
    flot.Launch((char *) mat1.memptr(), mat1.size() * sizeof(float));
    cout << mat1(0, 0) << endl;

    auto t2 = CurrTimeMS;

    cout << "Time spent = " << t2 - t1 << " ms.\n";
    
    return 0;
}

