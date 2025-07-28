class PatternCompiled(object):
    def __init__(self, pattern_str: str, pattern_charset: str, pattern_standard: str, consts: dict, *, include_higher_standard: bool = True):
        self.pattern_str: str = pattern_str
        self.pattern_charset: str = pattern_charset
        self.pattern_standard: str = pattern_standard
        self.consts: dict = consts

        self.include_higher_standard: bool = include_higher_standard

class PatternCompiler(object):
    def __init__(self):
        self.patterns: list = []

    def add_pattern(self, pattern: str, charset: str = "utf8", standard: str = "full_ansi", consts: dict = {}, include_higher_standard: bool = True):
        self.patterns.append(PatternCompiled(pattern, charset, standard, consts, include_higher_standard=include_higher_standard))

    def get_pattern(self, charset: str, standard: str, priority_charset: tuple, priority_standard: tuple):
        corresponding = None

        for item in self.patterns:
            if (item.pattern_charset == charset and item.pattern_standard == standard):
                corresponding = item
                break

            if (priority_charset.index(item.pattern_charset) < priority_charset.index(charset)):
                continue

            if (item.pattern_standard == standard):
                corresponding = item
                continue

            if (not item.include_higher_standard):
                continue

            if (priority_standard.index(item.pattern_standard) < priority_standard.index(standard)):
                continue

            if (corresponding and (priority_standard.index(item.pattern_standard) > priority_standard.index(corresponding.pattern_standard))):
                continue

            if (corresponding and (priority_charset.index(item.pattern_charset) > priority_charset.index(corresponding.pattern_charset))):
                continue

            corresponding = item

        return (corresponding)