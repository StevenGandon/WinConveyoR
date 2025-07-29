from xml.etree import ElementTree

from .compiler import PatternCompiler
from .exceptions import *
from ..builtin import GraphicObject

import sys

CHARSET_PRIORITY = (
    "utf8",
    "ascii"
)

STANDARD_PRIORITY = (
    "allansi",
    "256color",
    "88color",
    "16color",
    "colorless",
    "noansi"
)

class PatternLoader(object):
    def __init__(self, file: str):
        self.file: str = file
        self.compiled_patterns: list = []
        self.references: dict = {}

        self.load()

    def deploy(self, standard = "colorless", charset = "ascii", _globals = None, _locals =  None, overwrite: bool = True, allowed_class = (GraphicObject,)):
        if (_globals is None):
            _globals = globals()
        if (_locals is None):
            _locals = locals()

        def _lookup(item, _env):
            value = _env[item]

            if (hasattr(value, "__qualname__") and
               value.__qualname__ in self.references and
               any(map(lambda x: x in allowed_class, value.__mro__))):
                pattern = tuple(filter(lambda x: id(x) == self.references[value.__qualname__], self.compiled_patterns))[0]
                compiled = pattern.get_pattern(charset, standard, CHARSET_PRIORITY, STANDARD_PRIORITY)

                if (compiled is None):
                    sys.stderr.write(f"no instance of pattern for {value.__qualname__} match current settings: {standard}, {charset}")
                    return
                value._pattern = compiled.pattern_str
                value._const = compiled.consts.copy()


        for item in _globals:
            _lookup(item, _globals)

        for item in _locals:
            _lookup(item, _locals)

    def load(self):
        try:
            tree: ElementTree = ElementTree.parse(self.file)
        except Exception as e:
            raise PatternXMLParseError(f"Failed to parse {self.file}, {e}")

        root = tree.getroot()

        if (root.tag != "patterns"):
            raise PatternInvalidXMLStructure(f"Root tag should be 'patterns'.")

        for item in root:
            if (item.tag != "pattern"):
                raise PatternInvalidXMLStructure(f"'patterns' should only contain 'pattern' tags.")

            if ("for" not in item.attrib):
                sys.stderr.write(f"warning: missing 'for' attribute for pattern")

            compiled_pattern = PatternCompiler()

            for pattern in item:
                if (pattern.tag == "standard"):
                    pattern_type = pattern.attrib.get("type", "colorless")
                    charset = pattern.attrib.get("charset", "utf8")
                    include_higher = pattern.attrib.get("include-higher", "false")
                    pattern_str = ""
                    pattern_const = {}

                    for data in pattern:
                        if (data.tag == "value"):
                            pattern_str = data.text.replace('\\ESC', "\x1b")
                        if (data.tag == "const"):
                            for const in data:
                                pattern_const[const.tag] = const.text.replace('\\ESC', "\x1b")

                    compiled_pattern.add_pattern(pattern_str, charset, pattern_type, pattern_const, include_higher == "true")

            if ("for" in item.attrib):
                self.references[item.attrib["for"]] = id(compiled_pattern)

            self.compiled_patterns.append(compiled_pattern)
