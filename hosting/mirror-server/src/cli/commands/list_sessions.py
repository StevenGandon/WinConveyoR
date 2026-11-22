def get_permissions_name(session):
    permissions_name = []

    if (session.has_flags(1 << 0)):
        permissions_name.append("Admin")
    if (session.has_flags(1 << 1)):
        permissions_name.append("User")
    return (permissions_name)

def list_sessions_command(cli):
    print('\n'.join(map(lambda x: hex(x).split('0x')[-1] + " - " + ';'.join(get_permissions_name(cli.client.sessions[x])), cli.client.sessions.keys())))
