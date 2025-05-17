from .graphic_object import GraphicObject
from ..pattern.builder import PatternBuilder
from datetime import timedelta

from time import time

class LoadingBar(GraphicObject):
    def __init__(self, max_item, start_at = 0, step = 1):
        self.pattern = PatternBuilder(r"$+oa/$+ot$*pat[$+ol$+ob]$?sn$+on$*pnETA: $+oeta")
        self.max = max_item
        self.position = start_at
        self.step = step

        self.first_time = time()
        self.last_time = self.first_time
        self.eta_stack = []
        self.eta_stack_size = 10

        self.item_name = ""

        super().__init__()

    def push(self, item_name = ""):
        actual_time = time()

        if (self.position < self.max):
            self.position += self.step
            self.item_name = item_name

            self.eta_stack.append(actual_time - self.last_time)
            self.last_time = actual_time
            if (len(self.eta_stack) > self.eta_stack_size):
                self.eta_stack.pop(0)

            self.updated = True

    def update(self, display):
        ratio: int = round(50 * self.position / self.max)

        self.computed_string = self.pattern.build(
            ol='#' * (ratio),
            ob=' ' * (50 - ratio),
            ot=self.max,
            oa=self.position,
            on=self.item_name,
            oeta=timedelta(seconds=round((sum(self.eta_stack) / len(self.eta_stack)) * (self.max - self.position))),

            sn=("on", r" "),

            pat=(r"$+oa/$+ot", 14),
            pn=(r"$+on", 12)
        )
