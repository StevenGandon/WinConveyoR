class PatternBuilder(object):
    def __init__(self, base_pattern: str):
        self._pattern: str = base_pattern

    def build(self, **kwargs):
        temp = self._pattern
        for k, v in kwargs.items():
            temp = temp.replace(f"$+{k}", str(v))

            if (f"$*{k}" in temp):
                temp = temp.replace(f"$*{k}", ' ' * (v[1] - len(PatternBuilder(v[0]).build(**kwargs))))
            if (f"$?{k}" in temp):
                temp = temp.replace(f"$?{k}", PatternBuilder(v[1]).build(**kwargs) if v[0] in kwargs and kwargs[v[0]] else '')
        return (temp)
