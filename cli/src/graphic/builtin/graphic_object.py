class GraphicObject(object):
    def __init__(self):
        self.computed_string = ""

        self.updated: bool = True

    def update(self, display):
        pass

    def build(self):
        self.updated: bool = False

        return (self.computed_string)
