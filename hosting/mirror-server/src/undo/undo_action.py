class UndoAction(object):
    def __init__(self, name="generic_action"):
        self.name = name

        self.do = []

    def __str__(self):
        return f"UndoAction<{self.name}>[{len(self.do)} step(s)]"

    def __repr__(self):
        return self.__str__()

    def add_step(self, step):
        self.do.append(step)

    def execute_undo(self):
        for item in reversed(self.do):
            item.undo()

    def execute_redo(self):
        for item in self.do:
            item.redo()

class UndoStepPop(object):
    def __init__(self, src, e, idx=-1):
        self._src = src
        self._e = e
        self._prev = None
        self._idx = idx

    def undo(self):
        self._src.insert(self._idx, self._e)

        self._prev = id(self._src[self._idx])

    def redo(self):
        if self._prev is None:
            return

        res = tuple(filter(lambda x: id(self._src[x]) == self._prev, range(len(self._src))))

        self._prev = None

        if not len(res):
            return

        self._idx = res[0]
        self._e = self._src.pop(self._idx)


class UndoStepInsert(object):
    def __init__(self, src, idx=-1):
        self._src = src
        self._e_id = id(src[idx])
        self._prev = None
        self._idx = idx

    def undo(self):
        res = tuple(filter(lambda x: id(self._src[x]) == self._e_id, range(len(self._src))))

        if not len(res):
            return

        self._prev = self._src.pop(res[0])

    def redo(self):
        if self._prev is None:
            return

        self._src.insert(self._idx, self._prev)
        self._e_id = id(self._src[self._idx])

        self._prev = None


class UndoStepSetItem(object):
    def __init__(self, src, k=-1):
        self._src = src
        self._k = k
        self._new = src[k]

    def undo(self):
        if self._k in self._src:
            del self._src[self._k]

    def redo(self):
        self._src[self._k] = self._new


class UndoStepSetAttr(object):
    def __init__(self, obj, attr, prev):
        self._obj = obj
        self._attr = attr
        self._prev = prev
        self._new = getattr(obj, attr)

    def undo(self):
        setattr(self._obj, self._attr, self._prev)

    def redo(self):
        setattr(self._obj, self._attr, self._new)


class UndoStepDelItem(object):
    def __init__(self, e, src, k=-1):
        self._src = src
        self._e = e
        self._k = k
        self._idx = list(src.keys()).index(k) if k in src else len(src)

    def undo(self):
        if self._k in self._src:
            return

        items = list(self._src.items())
        idx = min(self._idx, len(items))
        items.insert(idx, (self._k, self._e))

        self._src.clear()
        self._src.update(items)

    def redo(self):
        if self._k in self._src:
            del self._src[self._k]