#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 65536

#define DISP_VAR(var, format) \
    do { \
        fprintf(stderr, "%s:%d " #var " = " #format "\n", \
                __FUNCTION__, __LINE__, (var)); \
    } while (0) 
#define DISP_ARRAY(var, format, length) \
    do { \
        fprintf(stderr, "%s:%d " #var " = {", __FUNCTION__, __LINE__); \
        for (size_t i=0; i<(size_t)length; ++i) { \
            fprintf(stderr, #format ", ", (var)[i]); \
        } \
        fprintf(stderr, "}\n"); \
    } while (0)
#define DISP_MEMORY_AS_CHARACHER(var, length) \
    do { \
        fprintf(stderr, "%s:%d " #var " = \"", __FUNCTION__, __LINE__); \
        for (size_t i=0; i<length; ++i) \
            buffer[i] == '\0' ? \
                fprintf(stderr, "\\0") : \
                fputc(buffer[i], stderr); \
        fprintf(stderr, "\"\n"); \
    } while (0)

void print_prompt() {
    char buffer[BUFFER_SIZE];

    printf("juShell ");
    getlogin_r(buffer, BUFFER_SIZE);
    printf("\033[32m%s\033[m:", buffer);
    getcwd(buffer, BUFFER_SIZE);
    printf("\033[33;1m%s\033[m $ ", buffer);
}

void parse_command(char * buffer, char *** argv_ptr, int * argc_ptr) {
    fgets(buffer, BUFFER_SIZE, stdin);
    if (feof(stdin)) {
        exit(0);
    }

    buffer[BUFFER_SIZE-1] = '\0';  /* to prevent buffer overflow */
    strtok(buffer, "\n");  /* remove the invisible tailing character */

    char quoted_chr='\0';  /* support for quotation mark */
    size_t cmd_buf_size=strlen(buffer);
    
    /* first scan to convert specific char to NUL for splitting the command line */
    for (size_t i=0; i<cmd_buf_size; ++i) {
        if (quoted_chr == '\0') {
            if (buffer[i] == ' ') {
                buffer[i] = '\0';
            } else if (buffer[i] == '\'' || buffer[i] == '"') {
                quoted_chr = buffer[i];
                buffer[i] = '\0';
            }
        } else if (buffer[i] == quoted_chr) {
            quoted_chr = '\0';
            buffer[i] = '\0';
        }
    }

    DISP_MEMORY_AS_CHARACHER(buffer, cmd_buf_size);

    /* second scan to count argc */
    *argc_ptr = 0;
    for (size_t i=0, start_at=0; i<=cmd_buf_size; ++i) {
        i += strchr(buffer+i, '\0') - buffer - i;
        start_at < i ? *argc_ptr += 1 : 0;
        start_at = i+1;
    }

    /* alloc memory for argv */
    *argv_ptr = (char **)calloc(1, sizeof(char*)*(1+*argc_ptr));
    
    /* third scan to redirect the each argument pointer into command buffer */
    for (size_t i=0, start_at=0, arg_i=0; arg_i<(size_t)*argc_ptr; ++i) {
        i += strchr(buffer+i, '\0') - buffer - i;
        (start_at < i) ? *(*argv_ptr + arg_i++) = buffer + start_at : 0;
        start_at = i+1;
    }
}

void __close_all_pipes(int fd[][2], int pipes) {
    for (int i=0; i<pipes; ++i) {
        close(fd[i][0]);
        close(fd[i][1]);
    }
}


void __sub_process_entry(int no, int pipes, char ** arg_p, int fd[][2]) {

    pipes > 0 ?
        no == 0 ? dup2(fd[0][1], 1) : 
            no == pipes ? dup2(fd[pipes-1][0], 0) : 
                dup2(fd[no][1], 1), dup2(fd[no-1][0], 0)
        : 0;

    __close_all_pipes(fd, pipes);
    
    /* count the number of param */
    int param_no=0;
    for (char ** item=arg_p; NULL != *item; ++item, ++param_no);

    DISP_ARRAY(arg_p, %s, param_no);

    /* find the sign for IO redirection */
    char * redirect_stdin=NULL;
    char * redirect_stdout=NULL;
    for (size_t i=0; i<(size_t)param_no; ++i) {
        if (NULL != arg_p[i]) {
            if (0 == strcmp("<", arg_p[i])) {
                redirect_stdin = arg_p[i+1];
                arg_p[i] = arg_p[i+1] = NULL;
            } else if (0 == strcmp(">", arg_p[i])) {
                redirect_stdout = arg_p[i+1];
                arg_p[i] = arg_p[i+1] = NULL;
            }
        }
    }
    DISP_VAR(redirect_stdin, %s);
    DISP_VAR(redirect_stdout, %s);

    /* advance all the non-NULL value */
    /* before: {"echo", NULL, NULL, "123", NULL, NULL, "234", NULL, "345"}
     * after:  {"echo", "123", "234", "345", NULL, NULL, NULL, NULL, NULL}
     */
    size_t p=0;
    DISP_ARRAY(arg_p, %s, param_no);
    while (NULL != arg_p[p]) p++;
    DISP_VAR(p, %zu);
    for (size_t i=p; i<(size_t)param_no; ++i) {
        if (NULL != arg_p[i]) {
            arg_p[p++] = arg_p[i];
            arg_p[i] = NULL;
        }
    }
    DISP_ARRAY(arg_p, %s, param_no);

    /* handle the redirect_stdout of stdin and stdout */
    if (NULL != redirect_stdin) {
        int fd=open(redirect_stdin, O_RDONLY);
        dup2(fd, 0);
        close(fd);
    }
    if (NULL != redirect_stdout) {
        int fd=open(redirect_stdout, O_CREAT | O_WRONLY);
        chmod(redirect_stdout, 0644);
        dup2(fd, 1);
        close(fd);
    }

    execvp(arg_p[0], arg_p);
}

void _execute_external_program(char *** argv_ptr, int argc) {
    int pipes=0;

    /* count pipe sign and set it to NULL*/
    for (int i=0; i<argc; ++i) {
        if (0 == strcmp("|", (*argv_ptr)[i])) {
            pipes += 1;
            (*argv_ptr)[i] = NULL;
        }
    }
    DISP_VAR(pipes, %d);
    
    int fd[pipes][2];

    /* initialize pipes */
    for (int i=0; i<pipes; ++i) {
        pipe(fd[i]);
    }

    char ** arg_p=*argv_ptr;

    for (int i=0; i<pipes+1; ++i) {
        0 == fork() ? __sub_process_entry(i, pipes, arg_p, fd) : 0;
        while (NULL != *arg_p++);
    }

    __close_all_pipes(fd, pipes);

    for (int i=0; i<pipes+1; ++i) {
        wait(NULL);
    }
}

void _execute_internal_cd(char ** argv, int argc) {
    argc == 1 ? chdir(getenv("HOME")) : chdir(argv[1]);
}

void _execute_internal_pwd() {
    char buffer[BUFFER_SIZE];

    getcwd(buffer, BUFFER_SIZE);
    puts(buffer);
}

void execute_command(char *** argv_ptr, int argc) {
    DISP_VAR(argc, %d);
    DISP_ARRAY(*argv_ptr, %s, argc);

    if (0 == strcmp("cd", *argv_ptr[0])) {
        _execute_internal_cd(*argv_ptr, argc);
    } else if (0 == strcmp("pwd", *argv_ptr[0])) {
        _execute_internal_pwd();
    } else if (0 == strcmp("exit", *argv_ptr[0])) {
        exit(0);
    } else 
        _execute_external_program(argv_ptr, argc);
}

void clean_up(char ** argv) {
    free(argv);
}

void loop() {
    char buffer[BUFFER_SIZE];
    char ** argv=NULL;
    int argc;

    print_prompt();
    parse_command(buffer, &argv, &argc);
    execute_command(&argv, argc);
    clean_up(argv);
}

int main() {

    while (true) {
        loop();
    }

    return 0;
}

