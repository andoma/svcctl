#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <locale.h>

#include <libtecla.h>

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
static int
completion(WordCompletion *cpl, void *data,
           const char *buf, int word_end)
{
  char line[4096];

  fprintf(fp, "c%s\n", buf);

  while(fgets(line, sizeof(line), fp) != NULL) {
    char *r = strchr(line, '\n');
    if(r == NULL) {
      exit(2);
    }
    *r = 0;
    if(isdigit(*line)) {
      return 0;
    } else if(*line == ':') {
      int mode;
      int start_pos;
      char s1[512];
      if(sscanf(line + 1, "%d %d %s", &mode, &start_pos, s1) == 3) {
          if(mode == 1) {
            cpl_add_completion(cpl, buf, start_pos, word_end, s1, NULL, " ");
          } else {
            cpl_add_completion(cpl, buf, start_pos, word_end, "", s1, NULL);
          }
      }
    } else {
      return 1;
    }
  }
  return 1;
}

/**
 *
 */
static void
interactive(const char *name)
{
  setlocale(LC_CTYPE, "");
  GetLine *gl = new_GetLine(1024, 10000);
  char *line;

  gl_configure_getline(gl, "bind ^W backward-delete-word", NULL, NULL);

  gl_customize_completion(gl, NULL, completion);

  while((line = gl_get_line(gl, "> ", NULL, -1)) != NULL) {
    char *x = strrchr(line, '\n');
    if(x != NULL)
      *x = 0;
    if(!strcasecmp(line, "quit") || !strcasecmp(line, "exit"))
      exit(0);
    int r = docmd(line);
    if(r)
      printf("%% Command failed with error %d\n", r);
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
