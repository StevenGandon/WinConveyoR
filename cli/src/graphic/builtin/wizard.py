from ..pattern.builder import PatternBuilder

from .graphic_object import GraphicObject

class WizardGraphic(GraphicObject):
    def __init__(self):
        self.pattern = PatternBuilder(WizardGraphic._pattern)

        super().__init__()

    def update(self, display):
        self.computed_string = self.pattern.build(
            text="test_text",

            wspace=("$+text", WizardGraphic._const.get("width-char", '#'))
        )