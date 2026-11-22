from json import loads

class Settings:
    def __init__(self):
        self.interval = 60.0
        self.auto_update = True
        self.auto_clean = False
        self.auto_verify = True

    @staticmethod
    def from_file(file_path: str):
        S = Settings()

        with open(file_path, 'r') as fp:
            content = loads(fp.read())
        S.interval = int(content.get("interval", 60.0))
        S.auto_update = bool(content.get("auto_update", True))
        S.auto_clean = bool(content.get("auto_clean", False))
        S.auto_verify = bool(content.get("auto_verify", True))

        return (S)