class ConfigKey(object):
    def __init__(self, content_type = str, /, required = True, default = None):
        self.required = required
        self.default = default

        self.type = content_type

class ConfigSchema(object):
    def __init__(self, content):
        self.content = content

class ConfigSchemaBank(object):
    def __init__(self, **kwargs):
        self.schemas = {**kwargs}
