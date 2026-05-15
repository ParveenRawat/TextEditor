/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/
struct termios originalTerm;

/*** terminal ***/
void die(char *s) {
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTerm) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &originalTerm) == -1)
    die("tcgetattr");

  atexit(disableRawMode);

  struct termios raw = originalTerm;

  // set terminal configurations to diable echo and other flags
  raw.c_iflag &= ~(ICRNL | INPCK | BRKINT | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  raw.c_cflag |= ~(CS8);

  // read returns if no input for 1 * 10 ms
  raw.c_cc[VTIME] = 1;
  // minimum no of bytes required for read to return
  raw.c_cc[VMIN] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

/***init ***/
int main() {
  enableRawMode();
  char c;
  while (1) {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
      die("read");

    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d (%c)\r\n", c, c);
    }

    if (c == 'q') {
      break;
    }
  }
  return 0;
}
