import time

__all__ = ['timedec', 'timewith']

def timedec(f):
    '''
    taken from https://zapier.com/engineering/profiling-python-boss/
    '''
    def f_timer(*args, **kwargs):
        start = time.time()
        result = f(*args, **kwargs)
        end = time.time()
        print(f.__name__, 'took', end - start, 'time')
        return result
    return f_timer

class timewith():
    '''
    taken from https://zapier.com/engineering/profiling-python-boss/
    '''
    def __init__(self, name='', finishMessage='finished'):
        self.name = name
        self.finishMessage = finishMessage
        self.start = time.time()

    @property
    def elapsed(self):
        return time.time() - self.start

    def checkpoint(self, name=''):
        print('{timer} {checkpoint} took {elapsed} seconds'.format(timer=self.name,
                                                                   checkpoint=name,
                                                                   elapsed=self.elapsed).strip())

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.checkpoint(self.finishMessage)