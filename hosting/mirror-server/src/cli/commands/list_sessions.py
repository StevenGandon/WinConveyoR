def list_sessions_command(cli):
    print('\n'.join(map(lambda x: hex(x).split('0x')[-1], cli.client.sessions.keys())))
