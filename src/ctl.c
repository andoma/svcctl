#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "linenoise.h"

static FILE *fp;

/**
 *
 */
static int
docmd(const char *str)
{
  char line[4096];

  fprintf(fp, "X%s\n", str);

  while(fgets(line, sizeof(line), fp) != NULL) {
    char *r = strchr(line, '\n');
    if(r == NULL) {
      exit(2);
    }
    *r = 0;
    if(isdigit(*line))
      return atoi(line);
    else if(*line == ':')
      printf("%s\n", line + 1);
    else
      printf("???: %s\n", line);
  }
  return 1;
}




/**
 *
 */
static void
completion(const char *buf, linenoiseCompletions *lc)
{
  char line[4096];

  fprintf(fp, "C%s\n", buf);

  while(fgets(line, sizeof(line), fp) != NULL) {
    char *r = strchr(line, '\n');
    if(r == NULL) {
      exit(2);
    }
    *r = 0;
    if(isdigit(*line))
      return;
    else if(*line == ':')
      linenoiseAddCompletion(lc, line+1);
    else
      printf("???: %s\n", line);
  }
}



/**
 *
 */
static void
interactive(const char *name)
{
  char prompt[256];
  char history[256];

  snprintf(prompt, sizeof(prompt), "%s> ", name);
  snprintf(history, sizeof(history), "%s/.%s_history", getenv("HOME"), name);

  linenoiseSetCompletionCallback(completion);
  linenoiseHistoryLoad(history);

  char *line;
  while((line = linenoise(prompt)) != NULL) {
    if(!strcasecmp(line, "quit") || !strcasecmp(line, "exit"))
      exit(0);

    if (line[0] != '\0' && line[0] != '/') {
      docmd(line);
      linenoiseHistoryAdd(line);
      linenoiseHistorySave(history);
    } else if (!strncmp(line,"/historylen",11)) {
      int len = atoi(line+11);
      linenoiseHistorySetMaxLen(len);
    } else if (line[0] == '/') {
      printf("Unreconized command: %s\n", line);
    }
    free(line);
  }
}

/**
 *
 */
int
main(int argc, char **argv)
{
  char buf[2048];
  const char *socketpath = NULL;
  const char *name = "svcctl";
  int c;

  while((c = getopt(argc, argv, "p:n:")) != -1) {
    switch(c) {
    case 'p':
      socketpath = optarg;
      break;
    case 'n':
      name = optarg;
      break;
    }
  }

  if(socketpath == NULL) {
    fprintf(stderr, "usage: %s -p /path/to/ctrlsocket ...\n", argv[0]);
    exit(1);
  }

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if(fd == -1) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_un sun;

  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, socketpath);

  if(connect(fd, (struct sockaddr *)&sun, sizeof(sun))) {
    perror("connect");
    exit(1);
  }

  fp = fdopen(fd, "r+");

  if(optind == argc) {
    interactive(name);
    exit(0);
  }

  int i, l = 0;
  buf[0] = 0;
  for(i = optind; i < argc; i++) {
    l += snprintf(buf + l, sizeof(buf) - l, "%s%s",
                  i == optind ? "" : " ", argv[i]);
  }

  if(buf[0] == 0)
    exit(0);

  docmd(buf);
}
