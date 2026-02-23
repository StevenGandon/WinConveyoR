class ConfigMessage(object):
    def __init__(self, message: str):
        self.message: str = message

    def __str__(self):
        return (f"{self.message}")

    def __repr__(self):
        return (self.__str__())

class ConfigWarning(ConfigMessage):
    pass

class ConfigError(ConfigMessage):
    pass