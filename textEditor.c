/*** includes ***/
#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/

struct editorConfig {
  int screenrows;
  int screencols;
  struct termios originalTerm;
};

struct editorConfig E;

/*** terminal ***/
void die(char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.originalTerm) == -1)
    die("tcsetattr");
}
char editorReadKey() {
  char c;
  int nread;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.originalTerm) == -1)
    die("tcgetattr");

  atexit(disableRawMode);

  struct termios raw = E.originalTerm;

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

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  // write the postion of cursor using escape sequence
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  // read the output into a buffer, R signifies the end
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }

  buf[i] = '\0';

  // read the sizes
  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    // moves the cursor to the bottom right then asks its postion if above fails
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** output ***/

void editorDrawRows() {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "|", 1);

    if (y < E.screenrows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}
void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** input ***/
void editorProcessKeypress() {
  char c = editorReadKey();
  switch (c) {
  case CTRL_KEY('q'):

    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    exit(0);
    break;
  }
}

/*** init ***/

void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    (die("getWindowSize"));
}

int main() {
  enableRawMode();
  initEditor();
  char c;
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
