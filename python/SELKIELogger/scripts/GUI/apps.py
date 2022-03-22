from . import main
from .SLConvert import SLConvertGUI
from .SLExtract import SLExtractGUI


def runSLConvertGUI():
    return main(SLConvertGUI)


def runSLExtractGUI():
    return main(SLExtractGUI)
