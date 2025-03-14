from typing import Iterable

class ArgumentHandlerException(Exception):
    pass

class TooMuchArguments(ArgumentHandlerException):
    pass

class TooFewArguments(ArgumentHandlerException):
    pass

class InvalidArgumentType(ArgumentHandlerException):
    pass

class ArgumentExclusionMatch(ArgumentHandlerException):
    pass

class ArgumentInclusionMissing(ArgumentHandlerException):
    pass

class ArgumentNecessaryMissing(ArgumentHandlerException):
    pass

class GenericArgument(object):
    def __init__(self, name: str = "unknown", typing = str, inclusion: list = [], exclusion: list = [], necessary: bool = False, args_after: int = 0, value = None):
        self.name = name
        try:
            self.value = list(map(typing, value)) if isinstance(value, Iterable) and not isinstance(value, (str, bytes, bytearray)) else typing(value)
        except ValueError:
            raise InvalidArgumentType(f"invalid argument type for argument {name}, got: {value}, expected: {typing}.")
        self.args_after = args_after
        self.typing = typing
        self.exclusion = exclusion
        self.inclusion = inclusion
        self.necessary = necessary

    def build_arguments(self):
        return (self.name, self.typing, self.inclusion, self.exclusion, self.necessary, self.args_after)

class ArgumentParserSettings(object):
    def __init__(self, min_argv: int = 0, max_argv: int = -1) -> None:
        self.min_argv: int = min_argv
        self.max_argv: int = max_argv

        self.arguments: list = []
        self.options: dict = {}
        self.parameters: dict = {}

        self.validated: bool = False

    def define_argument(self, typing = str, name=None) -> None:
        self.arguments.append(GenericArgument(name if name is not None else f"argument#{len(self.arguments)}", typing))

    def define_option(self, key, typing = str, name=None, inclusion = [], exclusion = [], necessary = False, args_after: int = 0) -> None:
        self.options[key] = GenericArgument(name if name is not None else key, typing, inclusion, exclusion, necessary, args_after)

    def define_parameter(self, key, typing = str, name=None, inclusion = [], exclusion = [], necessary = False) -> None:
        self.parameters[key] = GenericArgument(name if name is not None else key, typing, inclusion, exclusion, necessary)

    def validate(self):
        if (self.max_argv != -1):
            while (len(self.arguments) < self.max_argv):
                self.define_argument()

        self.validated: bool = True

class ArgumentParser(object):
    def __init__(self, argv: list, settings: ArgumentParserSettings = ArgumentParserSettings()) -> None:
        self._argv: list = [*argv]
        self._argc: int = len(self._argv)

        self.arguments: list = []   # any not classified options/parameters
        self.options: dict = {}     # -opt [value] | --opt [value] | opt [value]
        self.parameters: dict = {}  # key=value or --key=value

        self.settings: ArgumentParserSettings = settings

        self.parse()

    def parse(self):
        i: int = 0

        if (not self.settings.validated):
            self.settings.validate()

        while (i < self._argc):
            value: str = self._argv[i]

            if ('=' in value and value.split('=')[0] in self.settings.parameters):
                self.parameters[value.split('=')[0]] = GenericArgument(*self.settings.parameters[value.split('=')[0]].build_arguments(), value='='.join(value.split('=')[1:]))
            elif (value in self.settings.options):
                args_after = self.settings.options[value].args_after

                if (i + 1 + args_after > self._argc):
                    raise TooFewArguments(f"too few arguments for option {self.settings.options[value].name}, expecting: {args_after}.")

                self.options[value] = GenericArgument(*self.settings.options[value].build_arguments(), value=[*self._argv[i + 1:i + 1 + args_after]] if args_after else None)
                i += args_after
            else:
                if (self.settings.max_argv != -1 and len(self.arguments) >= self.settings.max_argv):
                    raise TooMuchArguments(f"too much arguments, excepecting: {self.settings.max_argv}.")
                if (len(self.settings.arguments) > len(self.arguments)):
                    self.arguments.append(GenericArgument(*self.settings.arguments[len(self.arguments)].build_arguments(), value=value))
                else:
                    self.arguments.append(GenericArgument(f"argument#{len(self.arguments)}", value=value))
            i += 1

        if (len(self.arguments) < self.settings.min_argv):
            raise TooFewArguments(f"too few arguments, excepecting: {self.settings.min_argv}, next argument needed: ({self.settings.arguments[len(self.arguments)].name}).")

        for item in self.settings.parameters.values():
            if (item.necessary and item not in self.parameters):
                raise ArgumentNecessaryMissing(f"missing necessary parameter {item.name}.")
        for item in self.settings.options.values():
            if (item.necessary and item not in self.options):
                raise ArgumentNecessaryMissing(f"missing necessary option {item.name}.")

        for item in self.options.values():
            if (any(map(lambda x: x in self.options, item.exclusion))):
                raise ArgumentExclusionMatch(f"{item.name} exclusion options match in exclusion list {item.exclusion}.")
            if (not all(map(lambda x: x in self.options, item.inclusion))):
                raise ArgumentInclusionMissing(f"{item.name} inclusion options mismatch in inclusion list {item.inclusion}.")

        for item in self.parameters.values():
            if (any(map(lambda x: x in self.parameters, item.exclusion))):
                raise ArgumentExclusionMatch(f"{item.name} exclusion parameters match in exclusion list {item.exclusion}.")
            if (not all(map(lambda x: x in self.parameters, item.inclusion))):
                raise ArgumentInclusionMissing(f"{item.name} inclusion parameters mismatch in inclusion list {item.inclusion}.")
