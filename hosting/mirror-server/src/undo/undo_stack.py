from .undo_action import UndoAction

from warnings import warn

class UndoStack(object):
    def __init__(self):
        self.undo_stack = []
        self.redo_stack = []

        self.registering_action = None

    def start_regisering_undo(self, name = "generic_action"):
        self.redo_stack.clear()

        self.registering_action = name

    def register_action(self):
        if (self.registering_action is None):
            raise RuntimeError("Can't register action when undo action has not started.")

    def end_regisering_undo(self):
        self.undo_stack.append(self.register_action)
        self.register_action = None

    def undo(self):
        if (self.registering_action is not None):
            warn("can't undo while registering an action.", UserWarning)
            return

        action = self.undo_stack.pop()

        self.redo_stack.append(action)

    def redo(self):
        if (self.registering_action is not None):
            warn("can't redo while registering an action.", UserWarning)
            return

        action = self.redo_stack.pop()

        self.undo_stack.append(action)
