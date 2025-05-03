from os import get_terminal_size
from re import compile as compile_regex

import sys

MODE_DISPLAY_NO_ANIMATION = 0
MODE_DISPLAY_SIMPLE = 1

ANSI_COLORS = compile_regex(r"\x1b\[[0-9;]+m")
ANSI_CODE = compile_regex(r'(?:\x1B[@-Z\\-_]|[\x80-\x9A\x9C-\x9F]|(?:\x1B\[|\x9B)[0-?]*[ -/]*[@-~])')

class GraphicElement(object):
    def __init__(self, element, allocated_pos = 0):
        self.allocated_pos = allocated_pos
        self.element = element

class GraphicSettings(object):
    def __init__(self, ansi: bool = True, color: bool = True, mode = MODE_DISPLAY_SIMPLE):
        self.ansi = ansi
        self.color = color
        self.mode = mode

class Graphic(object):
    def __init__(self, settings: GraphicSettings = GraphicSettings()):
        self._settings: GraphicSettings = settings

        self._display = []
        self._elements = []
        self._prev_sz = []

        self.size = get_terminal_size(sys.stdout.fileno())

    def add_elements(self, element):
        self._elements.append(GraphicElement(element, len(self._elements)))

    def update(self):
        self.size = get_terminal_size(sys.stdout.fileno())

        for item in self._elements:
            if (not hasattr(item.element, "update")):
                continue
            item.element.update(self)

    @staticmethod
    def remove_color(text):
        return (ANSI_COLORS.sub('', text))

    @staticmethod
    def remove_ansi(text):
        return (ANSI_CODE.sub('', text))

    def _lex_string(self, text):
        if (not self._settings.ansi):
            return (Graphic.remove_ansi(text))
        if (not self._settings.color):
            return (Graphic.remove_color(text))
        return (text)

    def draw(self, clear=True):
        strings = ""

        if (self._settings.ansi and self._settings.mode != MODE_DISPLAY_NO_ANIMATION):
            if (len(self._prev_sz) > 1):
                strings += (f"\033[{len(self._prev_sz) - 1}A\r")
            if (clear and len(self._prev_sz)):
                strings += ('\r\n'.join(map(lambda x: x[1] * ' ' if x[0] != (len(self._prev_sz) - 1) else ' ' * self.size[0], enumerate(self._prev_sz))))
                strings += (f"\033[{len(self._prev_sz) - 1 if len(self._prev_sz) > 1 else 1}A\r")

        for item in self._elements:
            if (not hasattr(item.element, "build")):
                continue
            if (hasattr(item.element, "updated") and not item.element.updated):
                continue
            if (self._settings.mode != MODE_DISPLAY_NO_ANIMATION):
                while (item.allocated_pos >= len(self._display)):
                    self._display.append("")

                self._display[item.allocated_pos] = item.element.build()
            else:
                self._display.insert(item.allocated_pos, item.element.build())

        self._prev_sz = list(map(len, self._display))

        strings += ('\n'.join(map(self._lex_string, self._display)))

        if (self._settings.mode == MODE_DISPLAY_NO_ANIMATION):
            self._display.clear()
        sys.stdout.write(strings)
        sys.stdout.flush()
