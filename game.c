#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "globals.h"
#include "utils.h"
#include "game.h"
#include "export.h"

#define LINES_ALLOC_WINDOW 2

#define HIGHSCORE_MAX 10

struct _Game {
  Line* lines;
  int nlines;
  int lines_current_max; // lines capacity for current allocation
  
  int score;
  
  Grid grid; // computed turn by turn
  
  Point lastPlay;
  
  Line possibilities[MAX_POSSIBILITIES];
  int possibilities_length;
  
  char* nickname;
  char* filepath;
  
  GameMode mode;
};

static Point noPoint = {-1, -1};

extern Point newPoint(int x, int y) {
  Point p;
  p.x = x;
  p.y = y;
  return p;
}
extern int pointEquals(Point a, Point b) {
  return a.x==b.x && a.y==b.y;
}
extern int pointExists(Point p) {
  return p.x>=0 && p.y>=0 && p.x<GRID_SIZE && p.y<GRID_SIZE;
}

extern Game* game_init() {
  Game* game = malloc(sizeof(Game));
  game->lines = 0;
  game->nlines = 0;
  game->score = 0;
  game->lines_current_max = 0;
  game_initGrid(&(game->grid));
  game->lastPlay = noPoint;
  game->mode = GM_SOBER;
  game->nickname = 0;
  game->filepath = 0;
  game_computeAllPossibilities(game);
  return game;
}
extern void game_close(Game* game) {
  if(game->lines)
    free(game->lines);
  free(game);
}

extern int game_getLineBetween(Point from, Point to, Line* line) {
  int i, dx, dy, incrX, incrY;
  Point p;
  dx = to.x-from.x;
  dy = to.y-from.y;
  incrX = dx==0 ? 0 : (dx<0 ? -1 : 1);
  incrY = dy==0 ? 0 : (dy<0 ? -1 : 1);
  for(i=0, p.x=from.x, p.y=from.y; i<LINE_LENGTH && pointExists(p); p.x+=incrX, p.y+=incrY, ++i)
    line->points[i] = p;
  return i; // count
}

extern Grid* game_getGrid(Game* game) {
  return & (game->grid);
}

extern int game_pointsInSameAxis(Point a, Point b) {
  return a.x==b.x || a.y==b.y;
}
extern int game_pointsInSameDiagonal(Point a, Point b) {
  return util_abs(a.x-b.x) == util_abs(a.y-b.y);
}

// new version
extern int game_countOccupiedCases(Game* game, Line line) {
  int i, count = 0;
  for(i=0; i<LINE_LENGTH; ++i)
    if(game_isOccupied(game, line.points[i]))
      ++count;
  return count;
}

/*
static int game_pointBetweenExclusif(Point p, Point a, Point b) {
  return util_inRangeExclusif(p.x, a.x, b.x) && util_inRangeExclusif(p.y, a.y, b.y);
}
*/

extern int game_pointsEquals(Point a, Point b) {
  return a.x==b.x && a.y==b.y;
}

extern int game_lineContainsPoint(Line line, Point point) {
  int i;
  for(i=0; i<LINE_LENGTH; ++i)
    if(pointEquals(point, line.points[i]))
      return TRUE;
  return FALSE;
}

extern int game_hasCollinearAndContainsTwo(Line* lines, int nlines, Point from, Point to) {
  Line l;
  int i, j;
  int count;
  for(i=0; i < nlines; ++i) {
    l = lines[i];
    count = 0;
    for(j=0; j<LINE_LENGTH; ++j)
      if(game_pointsEquals(l.points[j], from) || game_pointsEquals(l.points[j], to))
        count ++;
    if(count>1)
      return TRUE;
  }
  return FALSE;
}

extern int game_hasCollinearAndContains(Game* game, Line line) {
  Line l;
  int i, j, k;
  int count;
  
  for(i=0; i < game->nlines; ++i) {
    l = game->lines[i];
    count = 0;
    for(j=0; j<LINE_LENGTH; ++j)
      for(k=0; k<LINE_LENGTH; ++k)
        if(game_pointsEquals(l.points[j], line.points[k]))
          count ++;
    if(count>1)
      return TRUE;
  }
  return FALSE;
}

extern int game_isValidLineBetween(Point from, Point to) {
  int dx = to.x-from.x;
  int dy = to.y-from.y;
  return pointExists(from) && pointExists(to)
      && (util_abs(dx)==LINE_LENGTH-1 || util_abs(dy)==LINE_LENGTH-1)
      && (game_pointsInSameAxis(from, to) || game_pointsInSameDiagonal(from, to));
}


extern int game_computeAllPossibilities(Game* game) {
  Line* lines = game->possibilities;
  Line line;
  int possibilities = 0;
  int x, y;
  
  for(y=0; y<GRID_SIZE; ++y) {
    for(x=0; x<GRID_SIZE; ++x) {
      if(x>=5) {
        game_getLineBetween(newPoint(x-5, y), newPoint(x, y), &line);
        if(game_isPlayableLine(game, line))
          lines[possibilities++] = line;
      }
      if(y>=5) {
        game_getLineBetween(newPoint(x, y-5), newPoint(x, y), &line);
        if(game_isPlayableLine(game, line))
          lines[possibilities++] = line;
      }
      if(x>=5 && y>=5) {
        game_getLineBetween(newPoint(x-5, y-5), newPoint(x, y), &line);
        if(game_isPlayableLine(game, line))
          lines[possibilities++] = line;
        game_getLineBetween(newPoint(x-5, y), newPoint(x, y-5), &line);
        if(game_isPlayableLine(game, line))
          lines[possibilities++] = line;
      }
    }
  }
  return game->possibilities_length = possibilities;
}
extern Line* game_getAllPossibilities(Game* game, int* length) {
  *length = game->possibilities_length;
  return game->possibilities;
}

extern Point game_getCursor(Game* game) {
  return game->grid.cursor;
}
extern void game_setCursor(Game* game, Point p) {
  game->grid.cursor = p;
}
extern int game_isOccupied(Game* game, Point p) {
  return game->grid.grid[p.x][p.y] != CASE_EMPTY;
}
extern void game_occupyCase(Game* game, Point p) {
  game->grid.grid[p.x][p.y] = CASE_OCCUPIED;
}

extern int game_getScore(Game* game) {
  return game->score;
}

extern void game_selectCase(Game* game, Point p) {
  game->grid.select = p;
}
extern Point game_getSelect(Game* game) {
  return game->grid.select;
}
extern void game_emptySelection(Game* game) {
  game->grid.select.x = -1;
  game->grid.select.y = -1;
}

extern void game_setNickname(Game* game, char* nickname) {
  game->nickname = nickname;
}
extern char* game_getNickname(Game* game) {
  return game->nickname;
}
extern void game_setFilepath(Game* game, char* filepath) {
  game->filepath = filepath;
}
extern char* game_getFilepath(Game* game) {
  return game->filepath;
}

extern int game_getLinesCount(Game* game) {
  return game->nlines;
}

extern GameMode game_getMode(Game* game) {
  return game->mode;
}
extern void game_setMode(Game* game, GameMode mode) {
  game->mode = mode;
}

extern GameMode game_toggleMode(Game* game) {
  switch(game->mode) {
    case GM_SOBER: return game->mode = GM_VISUAL;
    case GM_VISUAL: return game->mode = GM_HELP;
    default: return game->mode = GM_SOBER;
  }
}

extern int game_mustDisplayPossibilities(Game* game) {
  return game->mode!=GM_SOBER;
}

extern Line* game_getLines(Game* game, int* length) {
  *length = game->nlines;
  return game->lines;
}
extern void game_addLine(Game* game, Line l) {
  if(game->nlines==game->lines_current_max) {
    game->lines_current_max += LINES_ALLOC_WINDOW;
    game->lines = game->lines==0 ? malloc(sizeof(Line)*game->lines_current_max) : realloc(game->lines, sizeof(Line)*game->lines_current_max);
  }
  game->lines[game->nlines++] = l;
}

extern int game_isPlayableLine(Game* game, Line line) {
    int count = game_countOccupiedCases(game, line);
    return ((count==LINE_LENGTH || count==LINE_LENGTH-1) 
    && !game_hasCollinearAndContains(game, line));
}

extern void game_recomputeGrid(Game* game) {
  int i;
  game_initGrid(&(game->grid));
  Line* lines = game->lines;
  int nlines = game->nlines;
  game->score = 0;
  game->lines = 0;
  game->nlines = 0;
  game->lines = malloc(sizeof(Line)*game->lines_current_max);
  for(i=0; i<nlines; ++i) {
    game_consumeLine(game, lines[i]);
  }
  free(lines);
}
extern void game_undoLine(Game* game) {
  game->nlines = MAX(game->nlines-1, 0);
  game_recomputeGrid(game);
}
extern void game_consumeLine(Game* game, Line line) {
  int i;
  int count = game_countOccupiedCases(game, line);
  for(i=0; i<LINE_LENGTH; ++i)
    game_occupyCase(game, line.points[i]);
  game->score += (count==LINE_LENGTH) ? POINTS_TRACE_LINE : POINTS_PUT_POINT;
  game_addLine(game, line);
}

static int highscoreEquals(Highscore *a, Highscore *b) {
  return a->score==b->score && strcmp(a->nickname, b->nickname)==0;
}

extern int game_saveScore(Game* game) {
  int rank = 0;
  Highscore highscores[HIGHSCORE_MAX];
  Highscore highscore;
  highscore.score = game_getScore(game);
  strncpy(highscore.nickname, game_getNickname(game), NICKNAME_LENGTH);
  int length = ie_retrieveHighscores(highscores, HIGHSCORE_MAX);
  ie_sortHighscores(highscores, length);
  if(length!=HIGHSCORE_MAX || highscores[length-1].score < highscore.score) {
    highscores[length!=HIGHSCORE_MAX ? length : length-1] = highscore;
    ++ length;
    ie_sortHighscores(highscores, length);
    
    for(rank=length-1; !highscoreEquals(&highscores[rank], &highscore) && rank>=0; --rank);
    rank ++;
  }
  ie_storeHighscores(highscores, MIN(HIGHSCORE_MAX, length));
  return rank;
}

extern void game_initGrid(Grid* grid) {
  Point p;
  grid->cursor.x = GRID_SIZE/2;
  grid->cursor.y = GRID_SIZE/2;
  grid->select.x = -1;
  grid->select.y = -1;
  for(p.y=0; p.y<GRID_SIZE; ++p.y) {
    for(p.x=0; p.x<GRID_SIZE; ++p.x) {
      if(((p.y==4||p.y==13) && p.x>=7 && p.x<=10) ||
         ((p.y==7||p.y==10) && ((p.x>=4 && p.x<=7)||(p.x>=10 && p.x<=13))) ||
         ((p.x==4||p.x==13) && p.y>=7 && p.y<=10) ||
         ((p.x==7||p.x==10) && ((p.y>=4 && p.y<=7)||(p.y>=10 && p.y<=13)))) {
        grid->grid[p.x][p.y] = CASE_OCCUPIED;
      }
      else
        grid->grid[p.x][p.y] = CASE_EMPTY;
    }
  }
}
