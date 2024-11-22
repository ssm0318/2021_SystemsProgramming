```bash
SIGSETOPS(3)             BSD Library Functions Manual             SIGSETOPS(3)

NAME
     sigaddset, sigdelset, sigemptyset, sigfillset, sigismember -- manipulate
     signal sets

LIBRARY
     Standard C Library (libc, -lc)

SYNOPSIS
     #include <signal.h>

     int
     sigaddset(sigset_t *set, int signo);

     int
     sigdelset(sigset_t *set, int signo);

     int
     sigemptyset(sigset_t *set);

     int
     sigfillset(sigset_t *set);

     int
     sigismember(const sigset_t *set, int signo);

DESCRIPTION
     These functions manipulate signal sets, stored in a sigset_t.  Either
     sigemptyset() or sigfillset() must be called for every object of type
     sigset_t before any other use of the object.

     The sigemptyset() function initializes a signal set to be empty.

     The sigfillset() function initializes a signal set to contain all signals.

     The sigaddset() function adds the specified signal signo to the signal set.

     The sigdelset() function deletes the specified signal signo from the signal
     set.

     The sigismember() function returns whether a specified signal signo is con-
     tained in the signal set.

     These functions are provided as macros in the include file <signal.h>.
     Actual functions are available if their names are undefined (with #undef
     name).

RETURN VALUES
     The sigismember() function returns 1 if the signal is a member of the set, 0
     otherwise.  The other functions return 0.

ERRORS
     Currently, no errors are detected.

SEE ALSO
     kill(2), sigaction(2), sigsuspend(2)

STANDARDS
     These functions are defined by IEEE Std 1003.1-1988 (``POSIX.1'').

BSD                              June 4, 1993                              BSD
(END)
```