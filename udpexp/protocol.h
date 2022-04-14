#define BUFLEN 1000
#define SHUTDOWN 0
#define DOWNLOAD 1
#define UPLOAD 2
#define GETDIR 3
#define YES 4
#define NO 5
#define START 6
#define END 7
#define CONTENT 8
#define OK 9

struct protocol
{
	int command;
	int len; 
	int no;
	char buf[BUFLEN];
};