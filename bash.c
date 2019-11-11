#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <err.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BLUE    "\x1b[34m"

void print_dir() {
  char *pwd = getcwd(NULL, 256), *short_pwd;
  char *user = getenv("USER");
  char host[256];
  gethostname(host, _SC_HOST_NAME_MAX);
  for (int i = 0, j = 0; j < 3; i++) {
      if (pwd[i] == '/') {
          j++;
      }
      short_pwd = &(pwd[i]);
  }
  printf(ANSI_COLOR_GREEN "%s@%s" ANSI_COLOR_RESET ":"
            ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "$ ", user, host, short_pwd);
  fflush(stdout);
  free(pwd);
}

char *get_word(char *endl_ptr) {
    char *word = NULL, c;
    int i = 0;
    c = getchar();
    while (c == ' ')
        c = getchar();
    for (; c != ' ' && c != '\n' && c != '\t'; i++) {
        word = realloc(word, (i + 1) * sizeof(char));
        word[i] = c;
        c = getchar();
    }
    word = realloc(word, (i + 1) * sizeof(char));
    word[i] = '\0';
    *endl_ptr = c;
    return word;
}

char **get_cmd(char *endl_ptr) {   // previous get_list()
    char **cmd = NULL;
    *endl_ptr = 0;
    for (int i = 0; ; i++) {
        cmd = realloc(cmd, (i + 1) * sizeof(char *));
        cmd[i] = get_word(endl_ptr);
        if (!strcmp(cmd[i], "|")) {
            free(cmd[i]);
            cmd[i] = NULL;
            break;
        }
        if (!strcmp(cmd[i], "&&")) {
            *endl_ptr = 'c';
            free(cmd[i]);
            cmd[i] = NULL;
            break;
        }
        if (*endl_ptr == '\n') {
            cmd = realloc(cmd, (i + 2) * sizeof(char *));
            cmd[i + 1] = NULL;
            break;
        }
    }
    return cmd;
}

char ***get_line(int *conv_ptr) {
    char ***line = NULL;
    char endl = 0;
    int i = 0;
    for (; endl != '\n' && endl != 'c'; i++) {
        line = realloc(line, (i + 1) * sizeof(char **));
        line[i] = get_cmd(&endl);
    }
    if (endl == 'c')
        *conv_ptr = 1;
    else
        *conv_ptr = 0;
    line = realloc(line, (i + 1) * sizeof(char **));
    line[i] = NULL;
    return line;
}

void print_line(char ***line) {
    if (line) {
        for (int i = 0; line[i]; i++) {
            for (int j = 0; line[i][j]; j++) {
                for (int k = 0; line[i][j][k]; k++)
                    putchar(line[i][j][k]);
                putchar(' ');
            }
            putchar('\n');
        }
        putchar('\n');
    }
}

void free_line(char ***line) {
    if (line) {
        for (int i = 0; line[i]; i++) {
            for (int j = 0; line[i][j]; j++)
                free(line[i][j]);
            free(line[i]);
        }
        free(line);
    }
}

int check_input(char **cmd) {
    if (cmd) {
        for (int i = 0; cmd[i]; i++) {
            if (!strcmp(cmd[i], "<")) {
                return i;
            }
        }
    }
    return 0;
}

int check_output(char **cmd) {
    if (cmd) {
        for (int i = 0; cmd[i]; i++) {
            if (!strcmp(cmd[i], ">")) {
                return i;
            }
        }
    }
    return 0;
}

void dupping(int fd_a, int fd_b) {
    if (fd_a >= 0)
        dup2(fd_a, fd_b);
}

void close_pipe(int fd[]) {
    if (fd) {
        if (fd[0] >= 0) {
            close(fd[0]);
            fd[0] = -1;
        }
        if (fd[1] >= 0) {
            close(fd[1]);
            fd[1] = -1;
        }
    }
}

void execute(char ***line) {
    if (strcmp(line[0][0], "cd") == 0) {
        char *home = getenv("HOME");
        if (!line[0][1] || !strcmp(line[0][1] , "~")) {
            chdir(home);
        } else {
            chdir(line[0][1]);
        }
        return;
    }
    int fd1[2] = {-1, -1}, fd2[2] = {-1, -1}, pid, i;
    int input_index, output_index, last_cmd_index = 0, fd_in = -1, fd_out = -1;
    char *temp_in, *temp_out;
    while (line[last_cmd_index + 1])
        last_cmd_index++;
    input_index = check_input(line[0]);
    if (input_index)
        fd_in = open(line[0][input_index + 1], O_RDONLY);
    output_index = check_output(line[last_cmd_index]);
    if (output_index)
        fd_out = open(line[last_cmd_index][output_index + 1],
            O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (input_index) {
        temp_in = line[0][input_index];
        line[0][input_index] = NULL;
    }
    if (output_index) {
        temp_out = line[last_cmd_index][output_index];
        line[last_cmd_index][output_index] = NULL;
    }
    fd1[0] = fd_in;
    for (i = 0; line[i]; i++) {
        if (i != last_cmd_index)
            pipe(fd2);
        else
            fd2[1] = fd_out;
        pid = fork();
        if (pid < 0) {
            perror("Error");
            exit(1);
        } else if (pid == 0) {
            dupping(fd1[0], 0);
            dupping(fd2[1], 1);
            close_pipe(fd1);
            close_pipe(fd2);
            if (execvp(line[i][0], line[i]) < 0) {
                perror("Error");
                exit(1);
            }
        }
        close_pipe(fd1);
        fd1[0] = fd2[0];
        fd1[1] = fd2[1];
    }
    if (input_index)
        line[0][input_index] = temp_in;
    if (output_index)
        line[last_cmd_index][output_index] = temp_out;
    for (; i >= 0; i--)
        wait(NULL);
}

int check_proc(char ***line) {
    int i, j, k;
    if (line) {
        if (!line[0] || !line[0][0] || !line[0][0][0])
            return -1;
        for (i = 0; line[i]; i++) {
            for (j = 0; line[i][j]; j++) {
                for (k = 0; line[i][j][k]; k++) {}
            }
        }
        if (line[i - 1][j - 1][k - 1] == '&') {
            if (k == 1) {
                free(line[i - 1][j - 1]);
                line[i - 1][j - 1] = NULL;
            } else {
                line[i - 1][j - 1][k - 1] = '\0';
            }
            if (!line[0] || !line[0][0] || !line[0][0][0]) {
                printf("Error: no such file or directory\n");
                return -1;
            }
            return 1;
        }
    }
    return 0;
}

void make_proc(char ***line, int *counter_ptr, int *conv_ptr) {
    int pid = fork();
    if (pid < 0) {
        perror("Error!");
        exit(1);
    } else if (pid == 0) {
        execute(line);
        free_line(line);
        exit(0);
    } else {
        if (*conv_ptr) {
            int wstatus;
            wait(&wstatus);
            printf("%d\n", WEXITSTATUS(wstatus));
        } else {
            (*counter_ptr)++;
            printf("[%d] %d\n", *counter_ptr, pid);
        }
    }
}

void handler(int signo) {
    putchar('\n');
    print_dir();
}

int main(void) {
    signal(SIGINT, handler);
    char ***line;
    int checker, counter = 1, conv;
    print_dir();
    line = get_line(&conv);
    checker = check_proc(line);
    while (!line[0][0]) {
        free_line(line);
        print_dir();
        line = get_line(&conv);
        checker = check_proc(line);
    }
    while (strcmp(line[0][0], "exit") && strcmp(line[0][0], "quit")) {
        if (!checker && !conv) {
            execute(line);
        } else if (checker > 0 || (checker >= 0 && conv)) {
            make_proc(line, &counter, &conv);
        }
        free_line(line);
        if (!conv) {
            print_dir();
        }
        line = get_line(&conv);
        checker = check_proc(line);
        while (!line[0][0]) {
            free_line(line);
            print_dir();
            line = get_line(&conv);
            checker = check_proc(line);
        }
    }
    free_line(line);
    return 0;
}
