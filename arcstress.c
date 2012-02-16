#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#define	BLOCKSIZE	131072

char *databuf;

typedef enum as_workload {
	AS_WORKLOAD_RANDOM,
	AS_WORKLOAD_SEQUENTIAL,
	AS_WORKLOAD_MAX
} as_workload_t;

static ssize_t
fullwrite(int fd, const void *buf, size_t nbytes)
{
	ssize_t total = 0, ret;

	while (total < nbytes) {
		if ((ret = write(fd, (void *)((uintptr_t)buf + total),
		    nbytes - total)) < 0)
			return (ret);
		total += ret;
	}

	return (total);
}

static ssize_t
fullread(int fd, void *buf, size_t nbytes)
{
	ssize_t total = 0, ret;

	while (total < nbytes) {
		if ((ret = read(fd, (void *)((uintptr_t)buf + total),
		    nbytes - total)) < 0)
			return (ret);
		total += ret;
	}

	return (total);
}

/* XXX Could stand to be refactored */
static int
as_write_files_rand(const char *dir, long nfiles)
{
	int ii, filenum, offset, randvalue, fd;
	char filename[PATH_MAX];
	struct stat statbuf;

	while (1) {
		filenum = rand() % nfiles;
		(void) snprintf(filename, PATH_MAX,
		    "%s/as_data.%d", dir, filenum);

		if (stat(filename, &statbuf) != 0) {
			fprintf(stderr, "Error stating file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		if ((fd = open(filename, O_WRONLY)) < 0) {
			fprintf(stderr, "Error opening file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		offset = rand() % (statbuf.st_size / BLOCKSIZE);

		/*
		 * Fill buffer with a random pattern
		 */
		randvalue = rand();
		for (ii = 0; ii < BLOCKSIZE; ii += 2) {
			databuf[ii] = randvalue;
			databuf[ii + 1] = randvalue >> 8;
		}

		if (lseek(fd, offset * BLOCKSIZE, SEEK_SET) < 0) {
			fprintf(stderr, "Failed to seek to offset %d: %s\n",
			    offset * BLOCKSIZE, strerror(errno));
			close(fd);
			return (-1);
		}

		if (fullwrite(fd, databuf, BLOCKSIZE) < BLOCKSIZE) {
			fprintf(stderr, "Failed to write %d bytes: %s\n",
			    BLOCKSIZE, strerror(errno));
			return (-1);
		}

		close(fd);
	}

	/*NOTREACHED*/
	return (0);
}

static int
as_write_file(int fd, ssize_t filesz)
{
	int ii, jj;

	for (ii = 0; ii < (filesz / BLOCKSIZE); ii++) {
		/*
		 * Fill buffer with a eight-byte pattern.  Blocks have a
		 * sequential pattern: a block of 0x0000, 0x0001, 0x0002, ...,
		 * 0x000f, 0x0010, 0x0011, ...
		 */
		for (jj = 0; jj < BLOCKSIZE; jj += 2) {
			databuf[jj] = ii;
			databuf[jj + 1] = ii >> 8;
		}

		if (fullwrite(fd, databuf, BLOCKSIZE) < BLOCKSIZE) {
			fprintf(stderr, "Failed to write %d bytes: %s\n",
			    BLOCKSIZE, strerror(errno));
			return (-1);
		}
	}

	return (0);
}

int
as_write_files_seq(char *dir, long nfiles, ssize_t filesz)
{
	int ii, fd;
	char filename[PATH_MAX];

	for (ii = 0; ii < nfiles; ii++) {
		(void) snprintf(filename, PATH_MAX, "%s/as_data.%d", dir, ii);
		if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		    0644)) < 0) {
			fprintf(stderr, "Error opening file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		if (as_write_file(fd, filesz) != 0)
			return (-1);

		close(fd);
		printf("Wrote %d MiB to %s.\n", filesz / 1024 / 1024, filename);
	}

	return (0);
}

static int
as_read_files_rand(const char *dir, long nfiles)
{
	int ii, filenum, offset, fd;
	char filename[PATH_MAX];
	struct stat statbuf;

	while (1) {
		filenum = rand() % nfiles;
		(void) snprintf(filename, PATH_MAX,
		    "%s/as_data.%d", dir, filenum);

		if (stat(filename, &statbuf) != 0) {
			fprintf(stderr, "Error stating file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		if ((fd = open(filename, O_RDONLY)) < 0) {
			fprintf(stderr, "Error opening file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		offset = rand() % (statbuf.st_size / BLOCKSIZE);

		if (lseek(fd, offset * BLOCKSIZE, SEEK_SET) < 0) {
			fprintf(stderr, "Failed to seek to offset %d: %s\n",
			    offset * BLOCKSIZE, strerror(errno));
			close(fd);
			return (-1);
		}

		if (fullread(fd, databuf, BLOCKSIZE) < BLOCKSIZE) {
			fprintf(stderr, "Failed to read %d bytes: %s\n",
			    BLOCKSIZE, strerror(errno));
			return (-1);
		}

		close(fd);
	}

	/*NOTREACHED*/
	return (0);
}

static int
as_read_file(int fd, ssize_t filesz)
{
	int ii;

	for (ii = 0; ii < (filesz / BLOCKSIZE); ii++) {
		if (fullread(fd, databuf, BLOCKSIZE) < BLOCKSIZE) {
			fprintf(stderr, "Failed to read %d bytes: %s\n",
			    BLOCKSIZE, strerror(errno));
			return (-1);
		}
	}

	return (0);
}

static int
as_read_files_seq(char *dir, long nfiles)
{
	int ii, fd, rv;
	char filename[PATH_MAX];
	struct stat statbuf;

	for (ii = 0; ii < nfiles; ii++) {
		(void) snprintf(filename, PATH_MAX, "%s/as_data.%d", dir, ii);

		if (stat(filename, &statbuf) != 0) {
			fprintf(stderr, "Error stating file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		if ((fd = open(filename, O_RDONLY)) < 0) {
			fprintf(stderr, "Error opening file %s: %s",
			    filename, strerror(errno));
			return (-1);
		}

		rv = as_read_file(fd, statbuf.st_size);
		close(fd);

		if (rv != 0)
			return (-1);

		printf("Read %d MiB from %s.\n",
		    statbuf.st_size / 1024 / 1024, filename);
	}

	return (0);
}

int
main(int argc, char *argv[])
{
	char dir[PATH_MAX], c;
	ssize_t filesz;
	boolean_t isread;
	int nfiles, rv;

	extern int optind, optopt;
	extern char *optarg;
	struct stat statbuf;
	as_workload_t workload;

	isread = B_FALSE;
	dir[0] = '\0';
	nfiles = 10;
	workload = AS_WORKLOAD_SEQUENTIAL;
	filesz = 10 * 1024 * 1024;  /* 10MB */

	while ((c = getopt(argc, argv, "d:n:rs:w:")) != EOF) {
		switch (c) {
		case 'd':
			strncpy(dir, optarg, PATH_MAX);
			break;
		case 'n':
			nfiles = atoi(optarg);
			break;
		case 'r':
			isread = B_TRUE;
			break;
		case 's':
			filesz = atoi(optarg);
			break;
		case 'w':
			if (strncasecmp(optarg, "rand", 4) == 0 ||
			    strcasecmp(optarg, "random") == 0)
				workload = AS_WORKLOAD_RANDOM;
			else if (strncasecmp(optarg, "seq", 3) == 0 ||
			    strcasecmp(optarg, "sequential") == 0)
				workload = AS_WORKLOAD_SEQUENTIAL;
			else
				fprintf(stderr,
				    "Unrecognized workload: '%s'\n", optarg);
			break;
		default:
			fprintf(stderr,
			    "Unrecognized option: -%c\n", optopt);
			rv = -1;
			goto out;
		}
	}

	if ((databuf = malloc(BLOCKSIZE)) == NULL) {
		perror("malloc");
		return (-1);
	}

	if (dir[0] == '\0')
		(void) snprintf(dir, PATH_MAX, "./as.%d", getpid());

	if (filesz % BLOCKSIZE != 0) {
		fprintf(stderr, "Error: file size %d not even multiple "
		    "of block size (128 KB)\n", filesz);
		rv = -1;
		goto out;
	}

	if (isread) {
		if (stat(dir, &statbuf) != 0) {
			fprintf(stderr, "Error reading files: %s",
			    strerror(errno));
			goto out;
		}
		rv = (workload == AS_WORKLOAD_SEQUENTIAL ?
		    as_read_files_seq(dir, nfiles) :
		    as_read_files_rand(dir, nfiles));
	} else {
		if (stat(dir, &statbuf) != 0) {
			if (errno == ENOENT) {
				if (mkdir(dir, 0755) != 0) {
					fprintf(stderr, "Error creating "
					    "directory: %s", strerror(errno));
					goto out;
				}
			} else {
				perror("stat");
				goto out;
			}
		}

		rv = (workload == AS_WORKLOAD_SEQUENTIAL ?
		    as_write_files_seq(dir, nfiles, filesz) :
		    as_write_files_rand(dir, nfiles));
	}

out:
	free(databuf);
	return (rv);
}
