import sys
import pathlib
import pytest

sys.path.append(str(pathlib.Path(__file__).resolve().parent.parent / 'cli' / 'src'))

from ..arghandler import (
    ArgumentParserSettings,
    ArgumentParser,
    TooMuchArguments,
    TooFewArguments,
    ArgumentExclusionMatch,
    ArgumentInclusionMissing,
    ArgumentNecessaryMissing,
    InvalidArgumentType,
    GenericArgument,
)


def test_argument_parsing_basic():
    settings = ArgumentParserSettings(min_argv=1, max_argv=2)
    settings.define_argument(name="first")
    settings.define_option("-n", args_after=1)
    settings.define_parameter("mode")

    parser = ArgumentParser(["arg", "-n", "5", "mode=test"], settings)

    assert parser.arguments[0].value == "arg"
    assert parser.options["-n"].value == ["5"]
    assert parser.parameters["mode"].value == "test"

def test_too_many_arguments():
    settings = ArgumentParserSettings(max_argv=1)
    ArgumentParser(["a"], settings)
    with pytest.raises(TooMuchArguments):
        ArgumentParser(["a", "b"], settings)

def test_too_few_arguments_global():
    settings = ArgumentParserSettings(min_argv=2)
    settings.define_argument(name="first")
    settings.define_argument(name="second")
    with pytest.raises(TooFewArguments):
        ArgumentParser(["only"], settings)

def test_option_missing_argument():
    settings = ArgumentParserSettings()
    settings.define_option("-o", args_after=1)
    with pytest.raises(TooFewArguments):
        ArgumentParser(["-o"], settings)

def test_generic_argument_invalid_type():
    with pytest.raises(InvalidArgumentType):
        GenericArgument("num", int, value="foo")

def test_option_exclusion_match():
    settings = ArgumentParserSettings()
    settings.define_option("-a", exclusion=["-b"])
    settings.define_option("-b", exclusion=["-a"])
    with pytest.raises(ArgumentExclusionMatch):
        ArgumentParser(["-a", "-b"], settings)

def test_option_inclusion_missing():
    settings = ArgumentParserSettings()
    settings.define_option("-a", inclusion=["-b"])
    settings.define_option("-b")
    with pytest.raises(ArgumentInclusionMissing):
        ArgumentParser(["-a"], settings)

def test_option_necessary_missing():
    settings = ArgumentParserSettings()
    settings.define_option("-a", necessary=True)
    with pytest.raises(ArgumentNecessaryMissing):
        ArgumentParser([], settings)

def test_parameter_necessary_missing():
    settings = ArgumentParserSettings()
    settings.define_parameter("mode", necessary=True)
    with pytest.raises(ArgumentNecessaryMissing):
        ArgumentParser([], settings)