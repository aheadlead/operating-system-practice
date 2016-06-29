#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>

bool endswith(const char * mon, const char * sub) {
    return strlen(mon) >= strlen(sub) &&
        strcmp(mon + strlen(mon) - strlen(sub), sub) == 0;
}

void load_and_execute(const char * shared_library_name) {

    printf("Load %s\n", shared_library_name);

    void * handle;
    char * error;

    void (*print)(void);
    
    handle = dlopen(shared_library_name, RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        exit(1);
    }

    print = dlsym(handle, "print");
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        exit(1);
    }

    print();

    dlclose(handle);
    
}

int main() {

    DIR * dir;
    struct dirent * ptr;
    dir = opendir(".");
    while ((ptr = readdir(dir)) != NULL) {
        if (endswith(ptr->d_name, ".so")) {
            load_and_execute(ptr->d_name);
        }
    }
    closedir(dir);

    return 0;

}

