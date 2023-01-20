#python 

# Install sysv_ipc module firstly if you don't have this
import sysv_ipc as ipc
import sys
import os
import time
import numpy as np

def main(r, w):
    path = "/tmp"
    key = ipc.ftok(path, 2333, silence_warning=True)
    shm = ipc.SharedMemory(key, 0, 0)

    #I found if we do not attach ourselves
    #it will attach as ReadOnly.
    shm.attach(0,0) 
    
    while True:
        readmessage = os.read(r, 8) # read a uint64 from pipe
        data_size = int.from_bytes(readmessage, byteorder='little', signed=False)
        print("[Python] Receive data size from master: ", data_size)
        if data_size == 0:
            break
        
        buf = shm.read(data_size) # read 128*4 fmat from shared memory
        # print("[Python] Data read from shared memory: ", buf)
        pcd_size = int(data_size / 4 / 4) # only for float number 
        fmat = np.frombuffer(buf, dtype='float32').reshape(pcd_size, 4)
        # print("[Python] Convert bytes to array\n", fmat)
        flot = fmat + 10
        print("[Python] Add 10 to the matrix")
        shm.write(flot.tobytes())
        print("[Python] waiting for 3s...")
        time.sleep(3) # do some process
        os.write(w, b"Done")
          
    shm.detach()
    
    pass

if __name__ == '__main__':
    pipe1 = int(sys.argv[1])
    pipe2 = int(sys.argv[2])
    print("[Python] read from pipe1: {}, write to pipe2: {}".format(pipe1, pipe2))
    main(pipe1, pipe2)
    
