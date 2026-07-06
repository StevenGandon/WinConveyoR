class UndoAction(object):
    def __init__(self, name = "generic_action"):
        self.name = name

        self.do = []

    def add_step(self, step):
        self.do.append(step)

    def execute_undo(self):
        for item in self.do:
            item.undo()

    def execute_redo(self):
        for item in self.do:
            item.redo()

class UndoStepPop(object):
    def __init__(self, src, e, idx = -1):
        self._src = src
        self._e = e
        self._prev = None
        self._idx = idx

    def undo(self):
        self._src.insert(self._idx, self._e)

        self._prev = id(self._src[self._idx])

    def redo(self):
        if (self._prev is None):
            return
        
        res = tuple(filter(lambda x: id(self._src[x]) == self._prev, range(len(self._src))))

        self._prev = None

        if (not len(res)):
            return
        
        self._e = self._src.pop(res)
        self._idx = res

class UndoStepInsert(object):
    def __init__(self, src, idx = -1):
        self._src = src
        self._e_id = id(src[idx])
        self._prev = None
        self._idx = idx

    def undo(self):
        res = tuple(filter(lambda x: id(self._src[x]) == self._e_id, range(len(self._src))))

        if (not len(res)):
            return
        
        self._prev = self._src.pop(res)

    def redo(self):
        if (self._prev is None):
            return

        self._src.insert(self._idx, self._prev)
        self._e_id = id(self._src[self._idx])

        self._prev = None
        
class UndoStepSetItem(object):
    def __init__(self, src, k = -1):
        self._src = src
        self._prev = None
        self._k = k

    def undo(self):
        self._prev = self._src[self._k]

        del self._src[self._k]

    def redo(self):
        if (not self._prev):
            return

        self._src[self._k] = self._prev

class UndoStepDelItem(object):
    def __init__(self, e, src, k = -1):
        self._src = src
        self._e = e
        self._prev = None
        self._k = k

    def undo(self):
        self._prev = self._src[self._k]
        self._src[self._k] = self._e
        
    def redo(self):
        if (self._k in self._src):
            del self._src[self._k]
    
        if (not self._prev):
            return
        
        self._src[self._k] = self._prev



        
