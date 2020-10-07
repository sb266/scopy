#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>

//structure for a user, containing name and permission
typedef struct
{
	char name[32];
	char perm;
} user;

//changes EUID to whatever is passed in
_Bool set_euid(uid_t id)
{
	if (seteuid(id) < 0)
	{
		printf("Failed to change uid.\n");
		return 0;
	}
	return 1;
}

//looks through the access control file and assigns permissions to structs.
//each line of the access control is examined and matched against a 'string character'
//format. failure to match is reported to main, which will subsequently quit.
// if 'character' is b, r, or w, it is assigned to the user for that line.
//if not, the function reports failure to main, which will quit.
int check_access(FILE * file, user * usr)
{
	int user_no = 0;
	char buf[32], name[32], perm;
	while(fgets(buf, 32, file))
	{
		if(sscanf(buf, "%s %c", name, &perm) != 2)
		{
			printf("Access control file is incorrectly formatted.\n");
			return 0;
		}
		if(perm == 'b' || perm == 'r' || perm == 'w')
		{
			strcpy(usr[user_no].name, name);
			usr[user_no].perm = perm;
			user_no++;
		}
		else
		{
			printf("Permission '%c' not recognized.\n", perm);
			return 0;
		}
	}
	return user_no;
}

//performs safe string concatenation to add the file extension
//to the permissions file.
char * add_extension(const char * file_name, const char * ext)
{
	//get string lengths of the file name and ".acl"
	size_t const name_len = strlen(file_name);
	size_t const ext_len = strlen(ext);

	//allocate space for the full file name to live in
	char * full_name = malloc(name_len + ext_len + 1);
	if(!full_name)
	{
		return full_name;
	}
	
	//copy the file name into the mem slot, then
	//copy the extension into the mem slot after
	//the name part ends.
	memcpy(full_name, file_name, name_len);
	memcpy(full_name + name_len, ext, ext_len + 1);

	return full_name;
}

int main(int argc, char ** argv)
{
	//make sure scopy was run correctly
	if(argc < 3 || strlen(argv[1]) < 1)
	{
		printf("Bad input.\n");
		return 1;
	}

	uid_t ruid = getuid(), euid = geteuid();

	//turn on real UID
	if(!set_euid(ruid)) return 0;

	//collect the source and destination filenames from input.
	//access control file is assumed to be <destination file>.acl
	char * file_name = argv[1];
	char * file_dest_name = argv[2];

	char * file_perm_name = add_extension(file_name, ".acl");

	if(!file_perm_name)
	{
		printf("Failed to perform concatenation.\n");
		return 0;
	}

	//change effective UID to access source and access control files
	if(!set_euid(euid)) return 0;

	int src_file = open(file_name, O_RDONLY), src_file_perm = open(file_perm_name, O_RDONLY), i;

	//return to real UID
	if(!set_euid(ruid)) return 0;

	if(src_file < 0 || src_file_perm < 0)
	{
		printf("Unable to access file(s).\n");
		return 0;
	}

	struct stat file_buf, perm_buf;

	if(fstat(src_file, &file_buf) < 0 || fstat(src_file_perm, &perm_buf) < 0)
	{
		printf("File stat failure.\n");
		return 0;
	}
	if(S_ISLNK(perm_buf.st_mode) || !S_ISREG(file_buf.st_mode))
	{
		printf("Irregular file.\n");
		return 0;
	}
	if(file_buf.st_uid != perm_buf.st_uid)
	{
		printf("Files are owned by different users.\n");
		return 0;
	}
	if(perm_buf.st_mode & S_IRGRP)
	{
		printf("Group permissions for access control file are incorrect.\n");
		return 0;
	}
	if(perm_buf.st_mode & S_IROTH)
	{
		printf("Other permissions for access control file are incorrect.\n");
		return 0;
	}

	//get at the access control file and initialize a collection of users
	FILE * perm_file = fdopen(src_file_perm, "r");
	user users[16];
	
	//make sure the .acl file has users and permissions inside
	int num_users;
	if((num_users = check_access(perm_file, users)) < 1)
	{
		return 0;
	}

	//see if the current user has permission to continue.
	//check current user's name against the name of every
	//user in the access control file.
	_Bool allowed = 0;
	struct passwd * usr_entry = getpwuid(ruid);
	user _user;
	for(i = 0; i < num_users; i++)
	{
		if(strcmp(usr_entry->pw_name, users[i].name) == 0)
		{	
			_user.perm = users[i].perm;
			strcpy(_user.name, users[i].name);
			allowed = 1;
			break;
		}
	}

	if(!allowed)
	{
		printf("%s doesn't have permission.\n", usr_entry->pw_name);
		return 0;
	}

	//If the user doesn't have read permission, they can't open the file
	//to copy it.
	if(_user.perm == 'w')
	{
		printf("You don't have permission to read this file.");
		return 0;
	}

	//open descriptors for source and destination files for the copy
	FILE * source = fdopen(src_file, "r");
	FILE * dest = fopen(file_dest_name, "w");

	if(!dest)
	{
		perror("fopen");
		printf("Exiting...\n");
		return 0;
	}

	if(!source)
	{
		printf("Failed to open source.\n");
		return 0;
	}
	
	//traverse the source file and write each character to destination
	char c;
	while((c = fgetc(source)) != EOF)
	{
		fputc(c, dest);
	}

	//match permissions on the destination file to those found in
	//the access control file.
	if(_user.perm == 'r') chmod(file_dest_name, S_IRUSR);
	if(_user.perm == 'w') chmod(file_dest_name, S_IWUSR);
	if(_user.perm == 'b') chmod(file_dest_name, (S_IRUSR | S_IWUSR));
	
	fclose(perm_file);
	fclose(source);
	fclose(dest);

	free(file_perm_name);

	close(src_file);
	close(src_file_perm);

	return 0;
}
