class Dialog(object):
    @staticmethod
    def ask_until_given(text):
        content = ""

        while (not content):
            content = input(text).strip()

        return (content)

    @staticmethod
    def ask_until_empty(text):
        content = "empty"
        items = []

        while (content):
            content = input(text).strip()
            if (content):
                items.append(content)

        return (items)

    @staticmethod
    def ask_enum(text, possibilities):
        content = ""

        while (content not in possibilities):
            content = input(text).strip()

            if (content not in possibilities):
                print(f"invalid value, should be within these ones: ({', '.join(possibilities)})")

        return (content)

    @staticmethod
    def ask_with_default(text, default):
        content = input(text).strip()

        if (not content):
            return (default)
        return (content)