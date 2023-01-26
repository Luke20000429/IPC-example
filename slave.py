#python 

# Install sysv_ipc module firstly if you don't have this
import sysv_ipc as ipc
import sys
import os
import time
import numpy as np

class Wrapper:
    
    path = "/tmp"
    projID = 2333
    
    def __init__(self, _r: int, _w: int):
        self.r = _r
        self.w = _w
        
        self.key = ipc.ftok(self.path, self.projID, silence_warning=True)
        self.shm = ipc.SharedMemory(self.key, 0, 0)

        #I found if we do not attach ourselves
        #it will attach as ReadOnly.
        self.shm.attach(0,0) 
        
    def __call__(self, _func):
        # output_data __func(input_buffer, n_bytes)
        while True:
            readmessage = os.read(self.r, 8) # read a uint64 from pipe
            data_size = int.from_bytes(readmessage, byteorder='little', signed=False)
            print("[Python] Receive data size from master: ", data_size)
            if data_size == 0:
                print("[Python] Exit!")
                break
            ibuf = self.shm.read(data_size) # read fmat from shared memory
            # print("[Python] Data read from shared memory: ", buf)
            obuf = _func(ibuf, data_size) # call user defined function
            self.shm.write(obuf.tobytes())
            print("[Python] waiting for 1s...")
            time.sleep(1) # do some process
            os.write(self.w, b"Done")
        
    def __del__(self):
        self.shm.detach()
        print("[Python] Detach from shared memory")

def FLOT(input_buffer, n_bytes):
    pcd_size = int(n_bytes / 4 / 4) # only for float number 
    mat = np.frombuffer(input_buffer, dtype='float32').reshape(pcd_size, 4)
    print("[Python] Add 10 to the matrix")
    return mat + 10    

if __name__ == '__main__':
    pipe1 = int(sys.argv[1])
    pipe2 = int(sys.argv[2])
    wrap = Wrapper(pipe1, pipe2)
    print("[Python] read from pipe1: {}, write to pipe2: {}".format(pipe1, pipe2))
    wrap(FLOT)
    
