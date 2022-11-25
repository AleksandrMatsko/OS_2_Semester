# OS_2_Semester
Operating Systems Labs
Laboratory works were submitted together with https://github.com/mentalMint

# Notes:
./utilities is my own library
To use the library you need:
1) execute Makefile in the utilities
2) run such command:
      $ export LD_LIBRARY_PATH="LD_LIBRARY_PATH:[path]"

   or add such line to /.bashrc:
      export LD_LIBRARY_PATH="[path]"

   where [path] is path to folder: utilities
3) during compilation, use such keys:
  -lsyncpipe -L [path] //last two keys means the path to lib files
