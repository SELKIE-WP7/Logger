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
