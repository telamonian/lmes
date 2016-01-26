from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.spec.io.hdf5.hdf5IOSpecs import HDF5IOSpecs

__all__ = ['HistogramIOSpecs']

class HistogramIOSpecs(HDF5IOSpecs):
    @property
    def cache(self):
        return '_%s' % self.histogramName
    
    @property
    def cache_dirty(self):
        return '%s_cache_dirty' % self.histogramName

    @property
    def dims(self):
        return '%s_dims' % self.histogramName

    @property
    def edges(self):
        return '%s_edges' % self.histogramName

    @property
    def init(self):
        return 'init%s' % CamelCaseUpper(self.histogramName)

    @property
    def mask(self):
        return '%s_mask' % self.histogramName

    @property
    def raw(self):
        return '%s_raw' % self.histogramName

    @property
    def threshold(self):
        return '%s_threshold' % self.histogramName

    @property
    def weight(self):
        return '%s_weight' % self.histogramName

    def __init__(self, histogramName):
        self.add()

        self.add(name=self.cache, subKey=self.cache, type='dataset')
        self.add(name=self.edges, subKey=self.edges, type='dataset')
        self.add(name=self.mask, subKey=self.mask, type='dataset')
        self.add(name=self.raw, subKey=self.raw, type='dataset')
        self.add(name=self.threshold, subKey=self.threshold, type='attribute')
        self.add(name=self.weight, subKey=self.weight, type='attribute')