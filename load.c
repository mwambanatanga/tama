#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#include "tama.h"


#define DELIM "="

void init_config(struct config *configstruct) {
	configstruct->port = PORT;
	configstruct->maxqueue = MAXQUEUE;
	strncpy(configstruct->tamafile, TAMAFILE, MAXBUF);
	configstruct->initweight = INITWEIGHT;
	configstruct->feedlimit = FEEDLIMIT;
	configstruct->hungertime = HUNGERTIME;
	configstruct->hungerpound = HUNGERPOUND;
	configstruct->deathtime = DEATHTIME;
	configstruct->lonelytime = LONELYTIME;
	configstruct->maxclients = MAXCLIENTS;
	configstruct->maxlist = MAXLIST;
}
/*
 * trim: get rid of trailing and leading whitespace...
 *       ...including the annoying "\n" from fgets()
 */
char *trim(char *s)
{
/**	printf("-%s-\n", s); */
	/* Initialize start, end pointers */
	char *s1 = s;
	char *s2 = &s[strlen(s) - 1];

	/* Trim and delimit right side */
	while ((isspace(*s2)) && (s2 >= s1))
		s2--;
	*(s2 + 1) = '\0';

	/* Trim left side */
	while ((isspace(*s1)) && (s1 < s2))
		s1++;

	/* Copy finished string */
	strcpy(s, s1);
/**	printf("-%s-\n", s); */
	return s;
}

int readconfig(char *filename, struct config *configstruct)
{
	char *s, buff[256];
	FILE *file = fopen(filename, "r");
	if (file != NULL) {
		char line[MAXBUF];
		int i = 0;

		while (fgets(line, sizeof(line), file) != NULL) {
			/* Skip blank lines and comments */
			if (line[0] == '\n' || line[0] == '#')
				continue;
			/* Parse name/value pair from line */
			char name[MAXBUF], value[MAXBUF];
			s = strtok(line, DELIM);
			if (s == NULL)
				continue;
			else {
				trim(s);
				strncpy(name, s, MAXBUF);
			}
			s = strtok(NULL, DELIM);
			if (s == NULL)
				continue;
			else
				strncpy(value, s, MAXBUF);
			trim(value);

			if (strcmp(name, "port") == 0) 
				configstruct->port = atoi(value);
			else if (strcmp(name, "maxqueue") == 0)
				configstruct->maxqueue = atoi(value);
			else if (strcmp(name, "tamafile") == 0)
				strncpy(configstruct->tamafile, value, MAXBUF);
			else if (strcmp(name, "initweight") == 0)
				configstruct->initweight = atoi(value);
			else if (strcmp(name, "feedlimit") == 0)
				configstruct->feedlimit = atoi(value);
			else if (strcmp(name, "hungertime") == 0)
				configstruct->hungertime = atoi(value);
			else if (strcmp(name, "hungerpound") == 0)
				configstruct->hungerpound = atoi(value);
			else if (strcmp(name, "deathtime") == 0)
				configstruct->deathtime = atoi(value);
			else if (strcmp(name, "lonelytime") == 0)
				configstruct->lonelytime = atoi(value);
			else if (strcmp(name, "maxclients") == 0)
				configstruct->maxclients = atoi(value);
			else if (strcmp(name, "maxlist") == 0)
				configstruct->maxlist = atoi(value);
			else
				printf("Unknown option: %s\n", name);
		}		/* End while */
		fclose(file);
	} else {
		fclose(file);
	}
	return 0;
}

/**
int main(int argc, char *argv[])
{
	init_config(&configstruct);
	int opt;
	int i;
	int j = 0;
	while ((opt = getopt(argc, argv, "c:p:q:")) != -1) {
		switch (opt) {
		case 'c':
			printf("Setting config file to \"%s\"\n", optarg);
			if (0 != readconfig(optarg))
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
			if (!j) strncpy(configstruct.maxqueue, optarg, MAXBUF);
			break;
		case 'o':
			printf("Setting port number to \"%s\"\n", optarg);
			for (i=0;i<strlen(optarg); i++) {
				if (!isdigit(optarg[i])) {
					printf ("Port is not a number\n");
					j = 1;
				}
			}
			if (!j) configstruct->port = atoi(optarg);
			
			break;
		}
	}
	printf("Port set to %s\n", configstruct.port);
	printf("Queue limit set to %s\n", configstruct.maxqueue);
	printf("File set to %s\n", configstruct.tamafile);
	return 0;
}
**/
