from .undo_action import UndoAction

from warnings import warn

class UndoStack(object):
    def __init__(self):
        self.undo_stack = []
        self.redo_stack = []

        self.registering_action = None

    def start_regisering_undo(self, name = "generic_action"):
        self.redo_stack.clear()

        self.registering_action = UndoAction(name)

    def register_action(self, step):
        if (self.registering_action is None):
            raise RuntimeError("Can't register action when undo action has not started.")
        
        self.registering_action.add_step(step)

    def end_regisering_undo(self):
        self.undo_stack.append(self.registering_action)
        self.registering_action = None

    def undo(self):
        if (self.registering_action is not None):
            warn("can't undo while registering an action.", UserWarning)
            return

        action = self.undo_stack.pop()

        action.undo()

        self.redo_stack.append(action)

    def redo(self):
        if (self.registering_action is not None):
            warn("can't redo while registering an action.", UserWarning)
            return

        action = self.redo_stack.pop()

        action.redo()

        self.undo_stack.append(action)
