from os import get_terminal_size
from re import compile as compile_regex
from os import write

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
        self._prev_sz = 0

        if (self._settings.mode == MODE_DISPLAY_NO_ANIMATION):
            self.size = (0, 0)
        else:
            self.size = get_terminal_size(sys.stdout.fileno())

    def clear_state(self):
        self._display.clear()
        self._elements.clear()
        self._prev_sz = 0

        print('\r')

    def add_elements(self, element):
        self._elements.append(GraphicElement(element, len(self._elements)))

    def update(self):
        if (self._settings.mode != MODE_DISPLAY_NO_ANIMATION):
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
        strings = bytearray()

        if (self._settings.ansi and self._settings.mode != MODE_DISPLAY_NO_ANIMATION):
            if (self._prev_sz > 1):
                strings.extend(f"\033[{self._prev_sz - 1}A\r".encode())
            if (clear and self._prev_sz):
                strings.extend(b'\r')
                strings.extend(b'\033[0K\r\n' * (self._prev_sz - 1))
                strings.extend(b"\033[0K")
                if (self._prev_sz > 1):
                    strings.extend((f"\033[{self._prev_sz - 1}A\r").encode())

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

        self._prev_sz = sum(map(lambda x: len(self.remove_ansi(x).split('\n')), self._display))

        strings.extend('\n'.join(map(self._lex_string, self._display)).encode())

        if (self._settings.mode == MODE_DISPLAY_NO_ANIMATION):
            strings.extend(b'\n')
            self._display.clear()
        write(sys.stdout.fileno(), bytes(strings))
        sys.stdout.flush()
