/*
**  Net Tamagotchi v1.1 by Milos Glisic, mglisic@lagged.net
**
**  Usage: tamad [port]
**
**  Greets go to: fredsan, m3lt, printf1, snowman
**  Xtra special thanx to Gopher for finding 78923734 bugs and making
**  it possible for Net Tamagotchi to be at least semi-stable.  
**
*/

/* includes */
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#ifdef _AIX
#include <strings.h>
#endif /* _AIX */

#include "tama.h"

int s;



/* Return ASCII string of current time */
char *logtime()
{
	struct tm *thetime;
	time_t utime;
	char *string;

	utime = time(NULL);
	thetime = localtime(&utime);
	string = asctime(thetime);

	string[strlen(string)-1]=0;
	return string;
}

/* SIGINT handler */
void term(int sig)
{
	/* On ^C, flush all streams and exit. */

	printf("%s Received INT signal. Exiting.\n", logtime());
	fflush(NULL);
	sleep(1);
	exit(0);
}

/* SIGALRM handler */
void timeout(int sig)
{
	/* On timeout, display error, flush all streams and exit. */

	put("\nConnection timed out. Disconnecting.\n");
	printf("%s Connection timed out. Disconnecting.\n", logtime());
	fflush(NULL);
	exit(0);
}

/* SIGSEGV handler */
void segv(int sig)
{
	put("An error has occured in the Net Tamagotchi server.\n");
	put("Please report the circumstances which caused this to algernon@debian.org\n");
	put("Thank you.\n");
	printf("%s Segmentation violation. Client handler exiting.\n", logtime());
	fflush(NULL);	
	exit(0);
}

/* SIGCHLD handler */
void chld(int sig)
{
 /* Do nothing.  Catch signal so that the select() call gets interrupted. */
}

/* read a string from the client - added compatibility */
/* for clients that use \r\n for newline, like win95 telnet */
void get(char *buf)
{
	int ctr;

	for(ctr=0; ctr<BUFLEN; ctr++) {
		if(recv(s, buf+ctr, 1, 0)<0) {
			close(s);
			exit(0);
		}
		if(buf[ctr]=='\r') ctr--;
		else if(buf[ctr]=='\n') break;
	}

	buf[ctr]='\0';
}

/* sends output to client - extended client support */
void put(char *buf)
{
	int ctr;

	for(ctr=0; ctr<strlen(buf); ctr++) {
		send(s, buf+ctr, 1, 0);
		if(buf[ctr]=='\n')
			send(s, "\r", 1, 0);
	}
}

int main(int argc, char **argv)
{
	init_config(&configstruct);
	int opts;
	int i;
	int j = 0;
	while ((opts = getopt(argc, argv, "c:p:q:")) != -1) {
		switch (opts) {
		case 'c':
			printf("Setting config file to \"%s\"\n", optarg);
			if (0 != readconfig(optarg, &configstruct))
				printf("Can not read config file.\n");
			break;
		case 'q':
			printf("Setting queue size to %s\n", optarg);
			for (i=0;i<strlen(optarg); i++) {
				if (!isdigit(optarg[i])) {
					printf ("Queue size is not a number\n");
					j = 1;
				}
			}
			if (!j) configstruct.maxqueue = atoi(optarg);
			break;
		case 'p':
			printf("Setting port number to %s\n", optarg);
			for (i=0;i<strlen(optarg); i++) {
				if (!isdigit(optarg[i])) {
					printf ("Port is not a number\n");
					j = 1;
				}
			}
			if (!j) configstruct.port = atoi(optarg);
			break;
		}
	}

	printf("Port number set to %i\n", configstruct.port);
	printf("Queue limit set to %i\n", configstruct.maxqueue);
	printf("Pets file set to %s\n", configstruct.tamafile);
	printf("Initial weight set to %i\n", configstruct.initweight);
	printf("Feeding interval set to %i\n", configstruct.feedlimit);
	printf("Hunger time set to %i\n", configstruct.hungertime);
	printf("Time to lose a pound set to %i\n", configstruct.hungerpound);
	printf("Time to death set to %i\n", configstruct.deathtime);
	printf("Time to getting lonely set to %i\n", configstruct.lonelytime);
	printf("Max number of clients set to %i\n", configstruct.maxclients);
	printf("List length limit to %i\n", configstruct.maxlist);

	fd_set input;
	pid_t pid;
	int rs, ns, port, fd, opt = 1, flags, clients = 0;
	struct timeval to;
	struct sockaddr_in sin;
	struct sockaddr_in fsin;
	struct hostent *hp;
	int result;
	char buf[BUFLEN], name[MAXNAME+1], arg[BUFLEN], *ptr; 
/**
	if(argc>1) {
		if(argc>2 || atoi(argv[1])==0) {
			fprintf(stderr, COMMANDLINE);
			return 1;
		} else port = atoi(argv[1]);
	} else 
*/
	port = configstruct.port;

	/* Hook signals */
	(void) signal(SIGINT, term);
	(void) signal(SIGALRM, timeout);
	(void) signal(SIGSEGV, segv);
	(void) signal(SIGCHLD, chld);

	printf("%s [%d] Starting %s", logtime(), getpid(), VER);
	if((ns = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
		perror("socket()");
		return 1;
	}
	printf("%s Created socket: s=%d\n", logtime(), ns);

	setsockopt(ns, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

	memset(&sin, 0, sizeof(struct sockaddr));
	memset(&fsin, 0, sizeof(struct sockaddr));

	sin.sin_family=AF_INET;
	sin.sin_port=htons(port);
	sin.sin_addr.s_addr=htonl(inet_addr(LOCAL));

	if(bind(ns, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		printf("%s ", logtime());
		fflush(stdout);
		perror("bind()");
		return 1;
	}
	printf("%s Bound socket to port %d\n", logtime(), port);

	if(listen(ns, configstruct.maxqueue) < 0) {
		perror("listen()");
		return 1;
	}

	printf("%s Listening for connections...\n", logtime());
	while(1) {
		socklen_t fromlen;
		char clienthost[1024];
		char clientport[20];
		/* Set time interval in which to perform zombie checks...
		** We have to do this every time it loops because select()
		** under Linux clears the timeout struct... lame. */
		to.tv_sec = CHECKTIME * 60;
		to.tv_usec = 0;

		/* Clear input */
		FD_ZERO(&input);
		FD_SET(ns, &input);
		if(select(ns + 1, &input, NULL, NULL, &to) > 0) {
			if((rs = accept(ns, (struct sockaddr *)&fsin, &fromlen)) < 0) {
				printf("%s ", logtime());
				fflush(stdout);
				perror("accept()");
				printf("rs = %i\n", rs);
				printf("ns = %i\n", ns);
				continue;
			}
		} else {
			/* Kill off zombies */
			while((pid = waitpid(0, NULL, WNOHANG)) > 0) {
				printf("%s [%d] Connection closed - purging session\n", logtime(), pid);
				clients--;
			}
			continue;
		}

		if(clients >= configstruct.maxclients) {
			s = rs;
			put("\nSorry, Net Tamagotchi is full right now.\nTry logging in later.\n\n");
			close(rs);
			continue;
		}

		clients++;
		flags = fcntl(ns, F_GETFL);
		flags |= O_NONBLOCK;

		if(fcntl(ns, F_SETFL, flags) < 0) {
			printf("%s ", logtime());
			fflush(stdout);
			perror("fcntl()");
		}

		/* Fork a child to handle the session and wait for it to terminate */
		if((pid=fork()) > 0) {

		/* Resolve remote hostname */
//			hp = gethostbyaddr((char *)&fsin.sin_addr, sizeof(struct in_addr), fsin.sin_family);
			result = getnameinfo((struct sockaddr *)&fsin, sizeof(fsin), clienthost, sizeof(clienthost), clientport, sizeof(clientport), 0);
/*
			if (hp)
				host = hp->h_name;
			else
				host = inet_ntoa(fsin.sin_addr);	
*/	
			if (result == 0) printf("%s [%d] Accepted connection from %s:%s\n", logtime(), pid, clienthost, clientport);
			close(rs);
			continue;
		}

		/* Login */
		s = rs;

		if((fd=open(MOTD, O_RDONLY)) > 0)
			putmotd(fd);

		put(INTRO);
		alarm(TIMELIMIT);	/* Set timeout alarm */
printf("1\n");
		get(buf);
printf("2 - %s\n", buf);
		strncpy(name, buf, MAXNAME);
printf("3 - %s\n", name);
		if(exist(name)<0) {
			printf("4\n");
			/* Check username format */
			if(check(name)<0) {
				put("That name is invalid.\n");
				put(STRINGRULE);
				close(s);
				exit(0);
			}
			put("That Tamagotchi doesn't exist. Would you like to create it? ");
			get(buf);
			if(buf[0]!='y' && buf[0]!='Y') {
				put("Fine, but you're missing out!\n");
				close(s);
				exit(0);
			}
			while(1) {
				put("Please choose a password: ");
				get(buf);
	
			/* Check password format validity */
				if(check(buf)==0) break;
				put(STRINGRULE);
			}
			if(new(name, buf) < 0) {
				put(NOACCESS );
				close(s);
				exit(0);
			}
			put("\nNew Tamagotchi \"");
			put(name);
			put("\", created.\n");
			printf("%s Created %s\n", logtime(), name);
		} else {
			printf("4\n");
			put("Tamagotchi found. Please enter password: ");
			get(buf);
			if(checkpass(name, buf)<0) {
				printf("%s Incorrect password for %s\n", logtime(), name);
				put("Password incorrect.\n");
				return 1;
			}
		}

		printf("%s [%d] `%s` logged in\n", logtime(), getpid(), name);
		put("Hi! The time limit for this session is 5 minutes\n");
		status(name, 1);

		while(1) {
			do {
				buf[0]='\0';
				put("> ");
				get(buf);
			} while(strlen(buf)==0);
	
			printf("%s Got command from %s: %s\n", logtime(), name, buf);

			/* parse argument */
			if(strstr(buf, " ")!=NULL) {
				ptr = buf;
				while(isalnum(ptr[0])) ptr++;
				ptr[0] = 0;
				while(!isalnum(ptr[0])) ptr++;
				strncpy(arg, ptr, BUFLEN);
			} else arg[0] = 0;

			if(strstr(arg, " ")!=NULL) {
				ptr = arg;
				while(isalnum(ptr[0])) ptr++;
				ptr[0] = 0;
				while(!isalnum(ptr[0])) ptr++;
			} else ptr = NULL;

			if(exec(buf, arg, ptr, name) < 0) {
				put(BYE);
				close(s);
				return 0;
			}
		}
	}
	return 0;
}
