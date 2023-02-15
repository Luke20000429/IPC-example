#python 

# used by shm
import sysv_ipc as ipc
import sys
import os
import time
import numpy as np

# used by flot
import torch
import argparse
from tqdm import tqdm
from flot.models.scene_flow import FLOT
from flot.datasets.kitti_flownet3d import Kitti

class Wrapper:
    
    path = "/tmp"
    projID = 2333
    
    def __init__(self, _r: int, _w: int, _model):
        self.r = _r
        self.w = _w
        self.model = _model
        
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
            obuf = _func(self.model, ibuf, data_size) # call user defined function
            self.shm.write(obuf.tobytes())
            # print("[Python] waiting for 1s...")
            # time.sleep(1) # do some process
            os.write(self.w, b"Done")
        
    def __del__(self):
        self.shm.detach()
        print("[Python] Detach from shared memory")
        
def loadModel(path2ckpt):
    # Load FLOT model
    scene_flow = FLOT(nb_iter=0)
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    scene_flow = scene_flow.to(device, non_blocking=True)
    sm = torch.jit.script(scene_flow)
    file = torch.load(path2ckpt)
    scene_flow.nb_iter = file["nb_iter"]
    scene_flow.load_state_dict(file["model"])
    scene_flow = scene_flow.eval()
    return scene_flow

def subsample(buffer, nb_points=2048):
    # frame_size is number of points per frame
    # pcd per frame is normally equal to n_bytes/size_of_float/3
    # nb point is 2048 by default
    # NOTE: assume pcd size in each frame is the same
    sequence = np.frombuffer(buffer, dtype='float32').reshape(2, -1)[:, (1, 2, 0)]
    # the original data set might store data as (z, x, y), thus reorder may not necessary for us 
    # assert(sequence.shape[1] == frame_size)
    # Restrict to 35m
    # NOTE: is 35m the height?
    loc = sequence[0][:, 2] < 35
    sequence[0] = sequence[0][loc]
    loc = sequence[1][:, 2] < 35
    sequence[1] = sequence[1][loc]
    # Choose points in first scan
    ind1 = np.random.permutation(sequence[0].shape[0])[:nb_points]
    sequence[0] = sequence[0][ind1]
    # Choose point in second scan
    ind2 = np.random.permutation(sequence[1].shape[0])[:nb_points]
    sequence[1] = sequence[1][ind2]
    # NOTE: why are we using different indices
    sequence = [torch.unsqueeze(torch.from_numpy(s), 0).float() for s in sequence]
    return sequence

def EstFlow(model, input_buffer, n_bytes):
    # NOTE: begin
    # Send data to GPU
    _device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    # sample shape # 2 x (batch_size, pcd_size after downsample, coordinates in (1, 2, 0))
    # normally 2 x (1, 2048, 3) can be (2, 4096, 3)
    # tbuf = torch.frombuffer(input_buffer, dtype=torch.float32).reshape(2, 1, 2048, 3) # ignore the warning as it is read-only
    tbuf = subsample(input_buffer)
    tbuf = tbuf.to(_device, non_blocking=True)
    
    print("Total bytes:", n_bytes, " tbuf shape:", tbuf.shape)

    # Estimate flow
    with torch.no_grad():
        print("ptcl shape", tbuf[0].shape)
        start_t = time.time()
        est_flow = model(tbuf[0], tbuf[1])
        print("time: ", time.time() - start_t)
        
    return est_flow.cpu().numpy()
    # return tbuf[0].cpu().numpy()

def AddTen(model, input_buffer, n_bytes):
    pcd_size = int(n_bytes / 4 / 4) # only for float number 
    # mat = np.frombuffer(input_buffer, dtype='float32').copy().reshape(pcd_size, 4)
    _device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    tbuf = torch.frombuffer(input_buffer, dtype=torch.float32).reshape(pcd_size, 4) # ignore the warning as it is read-only
    tbuf = tbuf.to(_device, non_blocking=True)
    print("[Python] Add 10 to the matrix")
    # return mat + 10   
    return (tbuf+10).cpu().numpy() 

if __name__ == '__main__':
    pipe1 = int(sys.argv[1])
    pipe2 = int(sys.argv[2])
    model = loadModel("./pretrained_models/model_2048.tar")
    
    wrap = Wrapper(pipe1, pipe2, model)

    print("[Python] read from pipe1: {}, write to pipe2: {}".format(pipe1, pipe2))
    wrap(AddTen)
    # wrap(EstFlow)
    
