import http.client, urllib


class PushoverClient:
    def __init__(self, config=None):
        if config is None:
            config = "/etc/pushover/pushover-config"

        self.config = {}
        with open(config, "r") as cfile:
            for line in cfile:
                key, value = line.split("=")
                key = key.lower().strip()
                self.config[key] = value.strip()

    def sendMessage(self, message, title=None):
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
