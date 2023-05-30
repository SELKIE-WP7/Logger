# Copyright (C) 2023 Swansea University
#
# This file is part of the SELKIELogger suite of tools.
#
# SELKIELogger is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this SELKIELogger product.
# If not, see <http://www.gnu.org/licenses/>.

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
