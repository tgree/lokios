#include "tmock.h"
#include "tcolor.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

struct test_case
{
    void        (*fn)();
    const char* name;
};

static tmock::internal::test_case_info* test_cases;
uint64_t tmock::internal::mode_flags = 0;

tmock::internal::test_case_registrar::test_case_registrar(
    tmock::internal::test_case_info* tci)
{
    tci->next = NULL;

    if (!test_cases)
    {
        test_cases = tci;
        return;
    }

    tmock::internal::test_case_info* pos = test_cases;
    while (pos->next)
        pos = pos->next;

    pos->next = tci;
}

static int
run_one_test(const char* name)
{
    for (tmock::internal::test_case_info* tci = test_cases;
         tci;
         tci = tci->next)
    {
        if (strcmp(name,tci->name) != 0)
            continue;

        tci->fn();
        return 0;
    }

    return -1;
}

static int
run_all_tests(const char* argv0)
{
    // Convert the mode flags to a hex string.
    char mode_flags_str[20];
    snprintf(mode_flags_str,sizeof(mode_flags_str),"0x%lX",
             tmock::internal::mode_flags);

    // Spawn all the tests so they run in parallel.
    for (tmock::internal::test_case_info* tci = test_cases;
         tci;
         tci = tci->next)
    {
        tci->pid = fork();
        if (tci->pid == 0)
        {
            // Child.
            execl(argv0,argv0,tci->name,mode_flags_str,NULL);
            printf("Error invoking child: %s %s\n",argv0,tci->name);
            return -1;
        }
    }

    // Wait on them all.
    int rc = 0;
    for (tmock::internal::test_case_info* tci = test_cases;
         tci;
         tci = tci->next)
    {
        int status;
        pid_t pid = waitpid(tci->pid,&status,0);
        if (pid != tci->pid)
        {
            printf("Error waiting for child process %d!\n",tci->pid);
            return -1;
        }
        printf("%s:%s ",argv0,tci->name);
        if (status == 0)
        {
            if (!(tci->flags & TCI_FLAG_FAILURE_EXPECTED))
                printf(GREEN "expectedly passed" RESET "\n");
            else
            {
                printf(RED "unexpectedly passed" RESET "\n");
                rc = -1;
            }
        }
        else
        {
            if (tci->flags & TCI_FLAG_FAILURE_EXPECTED)
                printf(GREEN "expectedly " RESET RED "failed with ");
            else
            {
                printf(RED "unexpectedly failed with ");
                rc = -1;
            }
            if (WIFEXITED(status))
                printf("exit status %d",WEXITSTATUS(status));
            else if (WIFSIGNALED(status))
                printf("signal %d",WTERMSIG(status));
            else if (WIFSTOPPED(status))
                printf("stop signal %d",WSTOPSIG(status));
            else
                printf("unknown error");
            printf(RESET "\n");
        }
    }

    return rc;
}

static int
usage(const char* argv0)
{
    printf("usage: %s [test_case_name [mode_flags]]\n",argv0);
    return -1;
}

int
tmock::run_tests(int argc, const char* argv[])
{
    if (argc > 3)
        return usage(argv[0]);

    if (argc == 3)
    {
        char* endptr;
        tmock::internal::mode_flags = strtoul(argv[2],&endptr,0);
        if (!*argv[2] || *endptr != '\0')
            return usage(argv[0]);
    }

    if (argc == 2 || argc == 3)
        return run_one_test(argv[1]);

    tmock::internal::mode_flags |= TMOCK_MODE_FLAG_SILENT;
    return run_all_tests(argv[0]);
}