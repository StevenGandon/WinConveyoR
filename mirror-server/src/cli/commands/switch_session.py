def switch_session_command(cli, /, session_id = None):
    if (session_id is None):
        cli.active_session = None
        cli.prompt = cli.prompt_base
        return
    if (session_id not in cli.client.sessions):
        print("invalid session.")
        return
    cli.active_session = session_id
    cli.prompt = cli.prompt_session.replace("$session_id", hex(cli.active_session).split('0x')[-1])
