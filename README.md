# scopy
Scopy with custom permissioning. Instead of Unix file access controls, this version uses file UID bits and a permissions file to allow or deny user access. The program can be used by authorized users to make their own copies of files which they may not necessarily be able to interact with directly.

This project is a self contained C file which should run on just about any *nix system.
Before beginning, please take a moment to configure permissions for several files.
    > The compiled executable should have its SetUID bit set.
    > The source file should be accessible only by its creator.
    > The access control file should be accessible only by its creator.
Commands are provided below to accomplish these steps:
    Executable: chmod u+s <executable name>
    Source file: chmod 700 <source file name>
    Access control file: chmod 700 <access control file name>

The command structure for scopy is as follows:
    ./scopy file_to_copy_from file_to_copy_to

Note the following:
    > The above command example assumes the executable is named scopy.
    > scopy is highly unlikely to function properly unless the file to copy to is aimed at a directory over which the user running scopy has control

When scopy has completed, the user who ran scopy will have a new file whose name is whatever they entered as the second argument to scopy. The file's permissions will depend on the contents of the access control file, which must have the following syntax:
	user p
Where user is the user's username, and p is in {r, w, b}. r will apply only write permissions, w only write, and b both. The access control file may be modified at any time, causing scopy to reflect those permissions in any files created during subsequent runs, and preventing files from being read or copied if the user does not have read or write permissions.

Example usage (assuming valid access control file with standard user sam's permissions set to 'b'):
    gcc -o scopy scopy.c
    chmod u+s scopy
	chmod 700 source_file.txt
	chmod 700 source_file.txt.acl
	su sam
	./scopy source_file.txt /home/sam/scopy_test_dir/sam_source_file.txt

sam_source_file.txt should then have the same contents as source_file.txt, and the following permissions:
-rw------- 1 sam sam
