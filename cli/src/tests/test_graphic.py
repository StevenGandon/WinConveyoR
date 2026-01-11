import sys
import pathlib

sys.path.append(str(pathlib.Path(__file__).resolve().parent.parent / 'cli' / 'src'))

from ..graphic import *

def test_remove_color_and_ansi():
    colored = "\x1b[31mred\x1b[0m"
    assert Graphic.remove_color(colored) == "red"
    assert Graphic.remove_ansi(colored) == "red"

def test_lex_string_settings_control():
    text = "\x1b[31mred\x1b[0m"
    g_no_ansi = Graphic(GraphicSettings(ansi=False, mode=MODE_DISPLAY_NO_ANIMATION))
    assert g_no_ansi._lex_string(text) == "red"

    g_no_color = Graphic(GraphicSettings(ansi=True, color=False, mode=MODE_DISPLAY_NO_ANIMATION))
    assert g_no_color._lex_string(text) == "red"

    g_full = Graphic(GraphicSettings(ansi=True, color=True, mode=MODE_DISPLAY_NO_ANIMATION))
    assert g_full._lex_string(text) == text