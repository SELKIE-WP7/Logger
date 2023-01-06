from . import main
from .N2KConvert import N2KConvertGUI
from .SLConvert import SLConvertGUI
from .SLExtract import SLExtractGUI


def runN2KConvertGUI():
    return main(N2KConvertGUI)


def runSLConvertGUI():
    return main(SLConvertGUI)


def runSLExtractGUI():
    return main(SLExtractGUI)
