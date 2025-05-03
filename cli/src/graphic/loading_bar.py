class LoadingBar(object):
    def __init__(self, max_item, start_at = 0, step = 1):
        self.max = max_item
        self.position = start_at
        self.step = step

        self.updated = True
        self.computed_string = ""

    def push(self):
        if (self.position < self.max):
            self.position += self.step
            self.updated = True

    def update(self, display):
        self.computed_string = f"[{'#' * (round(50 * self.position / self.max))}{' ' * (50 - round(50 * self.position / self.max))}]"

    def build(self):
        self.updated = False
        return (self.computed_string)
