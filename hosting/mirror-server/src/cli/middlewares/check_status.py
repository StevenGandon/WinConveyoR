def check_status(cli, args, response):
    if (response.content["code"] != 0):
        print(f"command failed: {response.content['data'].get('msg', "uknown_error")}")
        return (False, response)
    return (True, response)