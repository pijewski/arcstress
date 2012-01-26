# arcstress

This tool generates an I/O workload which stresses the filesystem cache.  It can
be configred for reading or writing, and in either a sequential or random
pattern. 


# Usage

    arcstress [-r] [-w rand | seq] [-s <file size>] [-n <number of files>]
        [-d <dir>]
  
    -r: Read file(s) instead of writing them.  Default is writing
    -w: Specify type of workload.  Default is sequential
    -s: Size of each file to read/write.  Default 10MB.
    -n: Number of files to read/write.  Default is 10.
    -d: Directory in which to read/write files.  Default is $PWD/ab.$PID.

If the tool is configured for reading, it's not necessary to specify the file
size, as the tool will stat each file to determine its size.

In addition, when the random workload is used, the tool will run forever.  With
the sequential workload, the tool will iterate through each file in numerical
order, and exit once it has read/written each file.


# Future Work

This tool is a work in progress, and there is some duplicated code which can be
refactored.  In addition, more tunables are needed to tune the file size, I/O
size, workload type, and files/directories used in this test.


# See Also

This tool was derived from iopattern (http://github.com/pijewski/iopattern).  At
some point, I may recombine the funcionality of both tools into one single tool.
