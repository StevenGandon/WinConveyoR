class PatternLoaderException(Exception):
    pass

class PatternXMLParseError(PatternLoaderException):
    pass

class PatternInvalidXMLStructure(PatternLoaderException):
    pass
