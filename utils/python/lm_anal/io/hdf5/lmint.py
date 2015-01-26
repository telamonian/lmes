import h5py

class LmInt(object):
    
    def LoadInt(self):
        with open(self.picklePath, 'rb') as pickF:
            self.sims = pickle.load(pickF)
        
    def SaveInt(self):
        self.SaveMod()
        with h5py.File(self.intPath, 'w') as intF:
            pickle.dump(self.sims, pickF)