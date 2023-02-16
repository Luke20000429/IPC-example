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
#define SHMSIZE (1024000)

#define CurrTimeMS (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
#define CurrTimeNS (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

int PROJECTID = 2333;

int main(int argc, char **argv) {

    if (argc > 1) {
        PROJECTID = atoi(argv[1]);
    }
    arma::fmat mat1;
    // mat1.fill(12);
    mat1.load("data/bin/000074.dat", arma::raw_binary);
    int n_points = mat1.size()/3;
    mat1.reshape(3, n_points);
    cout << mat1.col(0) << mat1.col(1) << mat1.n_cols << endl; // one point 

    // auto t1 = CurrTimeMS;

    pySubProc flot(SHMSIZE, GETEKYDIR, PROJECTID);
    flot.Launch((char *) mat1.memptr(), mat1.size() * sizeof(float));
    cout << mat1.col(0) << mat1.col(1) << endl;
    // flot.Launch((char *) mat1.memptr(), mat1.size() * sizeof(float));
    // cout << mat1(0, 0) << endl;
    // flot.Launch((char *) mat1.memptr(), mat1.size() * sizeof(float));
    // cout << mat1(0, 0) << endl;

    // auto t2 = CurrTimeMS;

    // cout << "Time spent = " << t2 - t1 << " ms.\n";
    
    return 0;
}

