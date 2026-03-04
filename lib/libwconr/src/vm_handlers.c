#include "vm_internal.h"

#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <sys/stat.h>
#    include <sys/wait.h>
#    include <dirent.h>
#endif

#ifdef _WIN32

static int vm_copy_file(const char *from, const char *to)
{
    HANDLE hfrom;
    HANDLE hto;
    BYTE buf[4096];
    DWORD nread;
    DWORD nwritten;
    DWORD written;

    hfrom = CreateFileA(from, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfrom == INVALID_HANDLE_VALUE)
        return (-1);

    hto = CreateFileA(to, GENERIC_WRITE, 0, NULL,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hto == INVALID_HANDLE_VALUE) {
        CloseHandle(hfrom);
        return (-1);
    }

    while (ReadFile(hfrom, buf, sizeof(buf), &nread, NULL) && nread > 0) {
        written = 0;
        while (written < nread) {
            if (!WriteFile(hto, buf + written, nread - written,
                           &nwritten, NULL)) {
                CloseHandle(hfrom);
                CloseHandle(hto);
                return (-1);
            }
            written += nwritten;
        }
    }

    CloseHandle(hfrom);
    CloseHandle(hto);
    return (0);
}

static int vm_remove_tree_recursive(const char *path)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pattern[4096];
    char child[4096];
    DWORD attrs;

    attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return (-1);

    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return (DeleteFileA(path) ? 0 : -1);

    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return (-1);

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
        if (vm_remove_tree_recursive(child) != 0) {
            FindClose(h);
            return (-1);
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return (RemoveDirectoryA(path) ? 0 : -1);
}

static int vm_copy_tree_recursive(const char *from, const char *to)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pattern[4096];
    char src_child[4096];
    char dst_child[4096];
    DWORD attrs;

    attrs = GetFileAttributesA(from);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return (-1);

    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return vm_copy_file(from, to);

    if (!CreateDirectoryA(to, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return (-1);

    snprintf(pattern, sizeof(pattern), "%s\\*", from);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return (-1);

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        snprintf(src_child, sizeof(src_child), "%s\\%s", from, fd.cFileName);
        snprintf(dst_child, sizeof(dst_child), "%s\\%s", to, fd.cFileName);
        if (vm_copy_tree_recursive(src_child, dst_child) != 0) {
            FindClose(h);
            return (-1);
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return (0);
}

#else /* POSIX */

static int vm_copy_file(const char *from, const char *to)
{
    int fd_from;
    int fd_to;
    unsigned char buf[4096];
    ssize_t nread;
    ssize_t nwritten;
    ssize_t w;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return (-1);

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_to < 0) {
        (void)close(fd_from);
        return (-1);
    }

    while ((nread = read(fd_from, buf, sizeof(buf))) > 0) {
        nwritten = 0;
        while (nwritten < nread) {
            w = write(fd_to, buf + nwritten, (size_t)(nread - nwritten));
            if (w < 0) {
                (void)close(fd_from);
                (void)close(fd_to);
                return (-1);
            }
            nwritten += w;
        }
    }

    (void)close(fd_from);
    (void)close(fd_to);
    return (nread < 0 ? -1 : 0);
}

static int vm_remove_tree_recursive(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char child[4096];

    if (lstat(path, &st) != 0)
        return (-1);

    if (!S_ISDIR(st.st_mode))
        return unlink(path);

    dir = opendir(path);
    if (!dir)
        return (-1);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        if (vm_remove_tree_recursive(child) != 0) {
            closedir(dir);
            return (-1);
        }
    }

    closedir(dir);
    return rmdir(path);
}

static int vm_copy_tree_recursive(const char *from, const char *to)
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char src_child[4096];
    char dst_child[4096];

    if (lstat(from, &st) != 0)
        return (-1);

    if (!S_ISDIR(st.st_mode))
        return vm_copy_file(from, to);

    if (mkdir(to, st.st_mode & 07777) != 0 && errno != EEXIST)
        return (-1);

    dir = opendir(from);
    if (!dir)
        return (-1);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(src_child, sizeof(src_child), "%s/%s", from, entry->d_name);
        snprintf(dst_child, sizeof(dst_child), "%s/%s", to, entry->d_name);
        if (vm_copy_tree_recursive(src_child, dst_child) != 0) {
            closedir(dir);
            return (-1);
        }
    }

    closedir(dir);
    return (0);
}

#endif /* _WIN32 */

static int vm_handle_noop(struct vm_arg_value *args, uint32_t count)
{
    (void)args; (void)count;
    return (0);
}

static int vm_handle_mkdir(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
#ifdef _WIN32
    if (!CreateDirectoryA(args[0].v.str.data, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
        return (-1);
#else
    if (mkdir(args[0].v.str.data, 0755) != 0 && errno != EEXIST)
        return (-1);
#endif
    return (0);
}

static int vm_handle_copy(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
    return vm_copy_file(args[0].v.str.data, args[1].v.str.data);
}

static int vm_handle_run(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD exit_code;
    char cmdline[4096];
    uint32_t i;
    int len;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    len = snprintf(cmdline, sizeof(cmdline), "%s", args[0].v.str.data);
    for (i = 0; i < args[1].v.str_array.count && len < (int)sizeof(cmdline) - 1; i++)
        len += snprintf(cmdline + len, sizeof(cmdline) - len,
                        " %s", args[1].v.str_array.data[i]);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL,
                        &si, &pi))
        return (-1);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (exit_code == 0 ? 0 : -1);
#else
    char **argv;
    uint32_t argc;
    uint32_t i;
    pid_t pid;
    int status;

    argc = args[1].v.str_array.count;
    argv = (char **)malloc(sizeof(char *) * (argc + 2));
    if (!argv)
        return (-1);

    argv[0] = args[0].v.str.data;
    for (i = 0; i < argc; i++)
        argv[i + 1] = args[1].v.str_array.data[i];
    argv[argc + 1] = NULL;

    pid = fork();
    if (pid < 0) { (void)free(argv); return (-1); }
    if (pid == 0) { execvp(argv[0], argv); _exit(127); }

    waitpid(pid, &status, 0);
    (void)free(argv);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return (-1);
    return (0);
#endif
}

static int vm_handle_chmod(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(args[0].v.str.data);

    if (attrs == INVALID_FILE_ATTRIBUTES)
        return (-1);
    if (args[1].v.u16_val & 0200)
        attrs &= ~FILE_ATTRIBUTE_READONLY;
    else
        attrs |= FILE_ATTRIBUTE_READONLY;
    return (SetFileAttributesA(args[0].v.str.data, attrs) ? 0 : -1);
#else
    return chmod(args[0].v.str.data, (mode_t)args[1].v.u16_val);
#endif
}

static int vm_handle_remove(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
#ifdef _WIN32
    return (DeleteFileA(args[0].v.str.data) ? 0 : -1);
#else
    return unlink(args[0].v.str.data);
#endif
}

static int vm_handle_remove_tree(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
    return vm_remove_tree_recursive(args[0].v.str.data);
}

static int vm_handle_copy_tree(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
    return vm_copy_tree_recursive(args[0].v.str.data, args[1].v.str.data);
}

static int vm_handle_rmdir(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
#ifdef _WIN32
    return (RemoveDirectoryA(args[0].v.str.data) ? 0 : -1);
#else
    return rmdir(args[0].v.str.data);
#endif
}

static int vm_handle_move(struct vm_arg_value *args, uint32_t count)
{
    (void)count;
#ifdef _WIN32
    return (MoveFileA(args[0].v.str.data, args[1].v.str.data) ? 0 : -1);
#else
    return rename(args[0].v.str.data, args[1].v.str.data);
#endif
}

static const struct {
    const char *name;
    vm_handler_t handler;
} vm_handler_table[] = {
    { "noop",        vm_handle_noop },
    { "mkdir",       vm_handle_mkdir },
    { "copy",        vm_handle_copy },
    { "run",         vm_handle_run },
    { "chmod",       vm_handle_chmod },
    { "remove",      vm_handle_remove },
    { "remove_tree", vm_handle_remove_tree },
    { "copy_tree",   vm_handle_copy_tree },
    { "rmdir",       vm_handle_rmdir },
    { "move",        vm_handle_move },
    { "move_tree",   vm_handle_move },
    { NULL,          NULL }
};

vm_handler_t vm_find_handler(const char *name)
{
    int i;

    for (i = 0; vm_handler_table[i].name != NULL; i++) {
        if (strcmp(vm_handler_table[i].name, name) == 0)
            return (vm_handler_table[i].handler);
    }

    return (NULL);
}
