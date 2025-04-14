#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#define MAX_USERS 1000
#define PATH_MAX 4096

typedef struct {
    uid_t uid;
    char login[256];
} _userMap;

_userMap users[MAX_USERS];
int user_count = 0;
int file_count = 0;

void load_user_info() {
    FILE *passwd_file = fopen("/etc/passwd", "r");
    if (passwd_file == NULL) {
        perror("Error opening /etc/passwd");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), passwd_file) && user_count < MAX_USERS) {
        char *login = strtok(line, ":");
        if (login == NULL) continue;
        
        char *x = strtok(NULL, ":");
        if (x == NULL) continue;
        
        char *uid_str = strtok(NULL, ":");
        if (uid_str == NULL) continue;
        
        uid_t uid = atoi(uid_str);
        
        users[user_count].uid = uid;
        strncpy(users[user_count].login, login, sizeof(users[user_count].login) - 1);
        users[user_count].login[sizeof(users[user_count].login) - 1] = '\0';
        
        user_count++;
    }

    fclose(passwd_file);
}

const char* get_login_from_uid(uid_t uid) {
    for (int i = 0; i < user_count; i++) {
        if (users[i].uid == uid) {
            return users[i].login;
        }
    }

    struct passwd *pwd = getpwuid(uid);
    if (pwd != NULL) {
        if (user_count < MAX_USERS) {
            users[user_count].uid = uid;
            strncpy(users[user_count].login, pwd->pw_name, sizeof(users[user_count].login) - 1);
            users[user_count].login[sizeof(users[user_count].login) - 1] = '\0';
            user_count++;
        }
        return pwd->pw_name;
    }

    static char uid_str[32];
    snprintf(uid_str, sizeof(uid_str), "%d", uid);
    return uid_str;
}

int has_extension(const char *filename, const char *ext) {
    const char *dot = strrchr(filename, '.');
    if (dot && dot != filename) {
        return strcmp(dot + 1, ext) == 0;
    }
    return 0;
}

void process_file(const char *path, const char *ext) {
    struct stat st;
    
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "Error accessing %s: %s\n", path, strerror(errno));
        return;
    }

    if (S_ISREG(st.st_mode) && has_extension(path, ext)) {
        file_count++;
        printf("%-3d : %-6s %-6ld %s\n", file_count, get_login_from_uid(st.st_uid),(long)st.st_size, path);
    }
}

void search_dir(const char *dir_path, const char *ext) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory %s: %s\n", dir_path, strerror(errno));
        return;
    }

    struct dirent *entry;
    char path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == -1) {
            fprintf(stderr, "Error accessing %s: %s\n", path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            search_dir(path, ext);
        } else if (S_ISREG(st.st_mode)) {
            process_file(path, ext);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <extension>\n", argv[0]);
        return 1;
    }

    const char *dir_path = argv[1];
    const char *ext = argv[2];

    load_user_info();

    printf("NO %-3s OWNER %-4s SIZE %-4s NAME\n", "", "", "");
    printf("-- %-3s ----- %-4s ---- %-4s ----\n", "", "", "");

    search_dir(dir_path, ext);

    printf("+++ %d files match the extension %s\n", file_count, ext);

    return 0;
}