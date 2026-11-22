class CLICommandArg(object):
    ARG_MANDATORY = 0
    ARG_OPTIONAL = 1

    def __init__(self, name="argument", arg_type: int = 0, *, argument_parser = str, argument_checker = None, type_name = "string"):
        self.name = name
        self.type = arg_type
        self.type_name = type_name

        self.parser = argument_parser
        self.checker = argument_checker

    def check(self, value):
        if (self.checker):
            return (self.checker(value))
        return (True)

    def parse(self, value):
        if (self.parser):
            return (self.parser(value))
        return (value)

class CLICommand(object):
    def __init__(self, name, command_callback, args: list = None):
        self.name = name

        self.args = (args if args is not None else list())
        self.callback = command_callback

        if (not self._check_order()):
            raise ValueError("mandatory argument can't be placed after optionnal ones.")

    def _check_order(self):
        encountered_optional: bool = False
    
        for item in self.args:
            if (not encountered_optional and item.type == CLICommandArg.ARG_OPTIONAL):
                encountered_optional = True

            if (encountered_optional and item.type == CLICommandArg.ARG_MANDATORY):
                return (False)

        return (True)

    def _get_mandatory(self):
        return (tuple(filter(lambda x: x.type == CLICommandArg.ARG_MANDATORY, self.args)))

    def _count_mandatory(self):
        return (len(self._get_mandatory()))

    def run_command(self, cli, arguments: list):
        if (self._count_mandatory() > len(arguments)):
            print(f"not enough arguments, mandatory arguments are: {tuple(map(lambda x: x.name, self._get_mandatory()))}")
            return
        if (len(self.args) < len(arguments)):
            print(f"too much arguments argument list is: {tuple(map(lambda x: x.name, self.args))}")
            return
        for i, item in enumerate(arguments):
            if (not self.args[i].check(item)):
                print(f"invalid argument type for {self.args[i].name}, looking for {self.args[i].type_name}, can't be parsed from {item}")
                return
            arguments[i] = self.args[i].parse(item)

        self.callback(cli, **{self.args[i].name: item for i, item in enumerate(arguments)})
