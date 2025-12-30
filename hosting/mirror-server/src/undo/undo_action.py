class UndoAction(object):
    def __init__(self, name = "generic_action"):
        self.name = name

        self.do = []
