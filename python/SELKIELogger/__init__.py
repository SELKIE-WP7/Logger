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

"""SELKIELogger data format support

SELKIELogger.scripts - interactive tools and utilities
SELKIELogger.scripts.GUI - Graphical tools and utilities
SELKIELogger.Processes - Common data handling tasks
SELKIELogger.PushoverClient - (Very) simple client for sending Pushover messages
SELKIELogger.SLFiles - Classes representing the different files used within the project
SELKIELogger.SLMessages - Classes representing the MessagePack based data format used within the project
SELKIELogger.WebInterface - Flask based UI for deployment on a device running the C data logging service
"""

from . import SLMessages
from . import SLFiles
