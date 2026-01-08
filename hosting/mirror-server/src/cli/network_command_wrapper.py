from copy import deepcopy

def default_json_payload_filler(payload, args):
    class StackEntry(object):
        def __init__(self, parent, item):
            self.parent = parent
            self.item = item

        def get(self):
            return (self.parent, self.item)

    stack = [StackEntry(payload.content, payload.content.items() if isinstance(payload.content, dict) else enumerate(payload.content))]

    while (stack):
        parent, values = stack.pop().get()
        for key, item in values:
            if (isinstance(item, str)):
                for arg in args:
                    if (f"${arg}" == item):
                        parent[key] = args[arg]
                        continue
                    if (f"${arg}" in item):
                        parent[key] = item.replace(f'${arg}', str(args[arg]))
            if (isinstance(item, list)):
                stack.append(StackEntry(item, enumerate(item)))
            if (isinstance(item, dict)):
                stack.append(StackEntry(item, item.items()))

    return (payload)

def network_command(payload, payload_filler, middlewares = []):
    def network_command_wrapper(command_function):
        def wrapper(cli, **kwargs):
            payload_filled = payload_filler(deepcopy(payload), kwargs)
            cli.client.write(payload_filled)
            response = cli.client.read()
            
            for middleware in middlewares:
                status, response = middleware(cli, kwargs, response)

                if (not status):
                    return

            return command_function(cli, **kwargs, response=response)
        return (wrapper)
    return (network_command_wrapper)