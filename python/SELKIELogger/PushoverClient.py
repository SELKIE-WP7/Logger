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

import http.client, urllib

## @file


class PushoverClient:
    """!A very simple Pushover client.
    Will attempt to read the default configuration file from pushover-bash script.
    """

    def __init__(self, config=None):
        """! Create PushoverClient instance
        @param[in] config Configuration file to use instead of default
        """
        if config is None:
            config = "/etc/pushover/pushover-config"  ## Default file path

        self.config = {}  ## Dictionary of configuration options from file
        with open(config, "r") as cfile:
            for line in cfile:
                key, value = line.split("=")
                key = key.lower().strip()
                self.config[key] = value.strip()

    def sendMessage(self, message, title=None):
        """!Send specified message via Pushover, optionally customising title.
        @param[in] message Message text
        @param[in] title If set, override message title from configuration file
        @returns None
        """
        if title is None:
            title = self.config.get("title", "SELKIELogger")

        conn = http.client.HTTPSConnection("api.pushover.net:443")
        conn.request(
            "POST",
            "/1/messages.json",
            urllib.parse.urlencode(
                {
                    "token": self.config["api_token"],
                    "user": self.config["user_key"],
                    "message": message,
                    "title": title,
                }
            ),
            {"Content-type": "application/x-www-form-urlencoded"},
        )
        conn.getresponse()


if __name__ == "__main__":
    PushoverClient().sendMessage("Test message")
