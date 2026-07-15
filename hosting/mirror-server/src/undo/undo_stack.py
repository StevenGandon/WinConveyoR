from .undo_action import UndoAction

from warnings import warn

class UndoStack(object):
    def __init__(self):
        self.undo_stack = []
        self.redo_stack = []

        self.registering_action = None

    def start_regisering_undo(self, name="generic_action"):
        self.redo_stack.clear()

        self.registering_action = UndoAction(name)

    def register_action(self, step):
        if self.registering_action is None:
            raise RuntimeError("Can't register action when undo action has not started.")

        self.registering_action.add_step(step)

    def end_regisering_undo(self):
        if self.registering_action is not None and len(self.registering_action.do):
            self.undo_stack.append(self.registering_action)

        self.registering_action = None

    def can_undo(self) -> bool:
        return len(self.undo_stack) > 0

    def can_redo(self) -> bool:
        return len(self.redo_stack) > 0

    def undo(self):
        if self.registering_action is not None:
            warn("can't undo while registering an action.", UserWarning)
            return

        if not len(self.undo_stack):
            warn("nothing to undo.", UserWarning)
            return

        action = self.undo_stack.pop()

        action.execute_undo()

        self.redo_stack.append(action)

    def get_lastest_undo(self):
        if self.registering_action is not None:
            warn("can't get get_lastest while registering an action.", UserWarning)
            return

        if not len(self.undo_stack):
            warn("nothing to get.", UserWarning)
            return

        return self.undo_stack[-1]

    def get_lastest_redo(self):
        if self.registering_action is not None:
            warn("can't get get_lastest while registering an action.", UserWarning)
            return

        if not len(self.redo_stack):
            warn("nothing to get.", UserWarning)
            return

        return self.redo_stack[-1]

    def redo(self):
        if self.registering_action is not None:
            warn("can't redo while registering an action.", UserWarning)
            return

        if not len(self.redo_stack):
            warn("nothing to redo.", UserWarning)
            return

        action = self.redo_stack.pop()

        action.execute_redo()

        self.undo_stack.append(action)