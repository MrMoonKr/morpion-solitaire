#include <string.h>
#include <stdlib.h>
// #include <curses.h>
#include <ncurses/ncurses.h> // pacman -S mingw-w64-ucrt-x86_64-ncurses 설치 필요

#include "ui.h"
#include "game.h"
#include "points.h"
#include "globals.h"

#define ESCAPE_KEY 27
#define ENTER_KEY 10

#define GRAPHIC_CASE_H 2
#define GRAPHIC_CASE_W 4

#define WIN_TITLE_WIDTH (2 + GRAPHIC_CASE_W * GRID_SIZE)
#define WIN_MESSAGE_WIDTH (4 + GRAPHIC_CASE_W * GRID_SIZE)
#define TITLE_TOP 1
#define GRID_TOP 3
#define WIN_MESSAGE_TOP (GRID_TOP + TITLE_TOP + 1 + (GRAPHIC_CASE_H * GRID_SIZE))
#define GRID_LEFT MAX((COLS - WIN_MESSAGE_WIDTH) / 2, 0)

#define BUFFER_SIZE 64

typedef enum
{
    CLR_DEFAULT = 1,
    CLR_CASE,
    CLR_CASE_SELECTED,
    CLR_CASE_EMPTY_SELECTED,
    CLR_LINES,
    CLR_LINES_PLAYABLE,
    CLR_MESSAGE,
    CLR_MESSAGE_ERROR,
    CLR_MESSAGE_SUCCESS,
    CLR_RED,
    CLR_BLUE,
    CLR_GREEN,
    CLR_YELLOW
} CLR;

WINDOW* win_grid;
WINDOW* win_message;
WINDOW* win_title;

/// Static functions

static void setColor(WINDOW *win, int color)
{
    wattron(win, COLOR_PAIR(color));
}
static void ui_printMessage(char *str)
{
    mvwhline(win_message, 1, 1, ' ', WIN_MESSAGE_WIDTH - 2); // clean message
    mvwprintw(win_message, 1, (WIN_MESSAGE_WIDTH - strlen(str)) / 2, str);
}

static Point toGraphicCoord(Point p)
{
    Point newP;
    newP.x = GRAPHIC_CASE_W / 2 + GRAPHIC_CASE_W * p.x;
    newP.y = GRAPHIC_CASE_H / 2 + GRAPHIC_CASE_H * (GRID_SIZE - p.y - 1);
    return newP;
}

static void drawLines(Line *lines, int nlines)
{
    Point a, b, p, p2;
    int hasReverseDiag;
    int x, y, i, j;
    Line line;

    for (i = 0; i < nlines; ++i)
    {
        line = lines[i];
        for (j = 0; j < LINE_LENGTH - 1; ++j)
        {
            p = toGraphicCoord(line.points[j]);
            p2 = toGraphicCoord(line.points[j + 1]);
            x = (p.x + p2.x + GRAPHIC_CASE_W) / 2;
            y = (p.y + p2.y + GRAPHIC_CASE_H) / 2;
            if (p.x == p2.x)
                mvwprintw(win_grid, y, x, ":");
            else if (p.y == p2.y)
                mvwprintw(win_grid, y, x - 1, "---");
            else
            {
                // compute a & b to be the reverse diagonal of p & p2
                a = line.points[j];
                b = line.points[j + 1];
                if (a.x < b.x)
                {
                    a.x++;
                    b.x--;
                }
                else
                {
                    a.x--;
                    b.x++;
                }
                hasReverseDiag = line_hasCollinearAndContainsTwo(lines, nlines, a, b);
                if ((p.y < p2.y && p.x < p2.x) || (p.y > p2.y && p.x > p2.x))
                    mvwprintw(win_grid, y, x, hasReverseDiag ? "X" : "\\");
                else
                    mvwprintw(win_grid, y, x, hasReverseDiag ? "X" : "/");
            }
        }
    }
}

static void drawPoint(Point p, char c)
{
    Point graphicPoint = toGraphicCoord(p);
    mvwprintw(win_grid, graphicPoint.y + 1, graphicPoint.x + 2, "%c", c);
}

static void drawPointsOnGrid(Point *points, int length, Point cursor, Point select, char c, CLR clr_selected, CLR clr)
{
    int i;
    Point p;
    for (i = 0; i < length; ++i)
    {
        p = points[i];
        if (point_equals(p, cursor))
            wattron(win_grid, A_REVERSE);
        setColor(win_grid, point_equals(p, select) ? clr_selected : clr);
        drawPoint(p, c);
        wattroff(win_grid, A_REVERSE);
    }
}

static void cleanGrid()
{
    setColor(win_grid, CLR_DEFAULT);
    int i, wLength = GRAPHIC_CASE_W * GRID_SIZE, hLength = GRAPHIC_CASE_H * GRID_SIZE + 1;
    for (i = 1; i < hLength; ++i)
        mvwhline(win_grid, i, 1, ' ', wLength);
}

// Functions

extern void ui_init()
{
    initscr();
    ESCDELAY = 0;
    if (has_colors() == FALSE)
    {
        endwin();
        fprintf(stderr, "Your terminal does not support colors\n");
        exit(1);
    }
    cbreak();
    noecho();
    win_title = newwin(2, WIN_TITLE_WIDTH, TITLE_TOP, GRID_LEFT);
    win_grid = newwin(GRAPHIC_CASE_H * (GRID_SIZE + 1), GRAPHIC_CASE_W * (GRID_SIZE + 1), GRID_TOP, GRID_LEFT);
    win_message = newwin(3, WIN_MESSAGE_WIDTH, WIN_MESSAGE_TOP, GRID_LEFT);
    keypad(win_title, TRUE);
    keypad(win_grid, TRUE);
    keypad(win_message, TRUE);
    curs_set(0);
    start_color();
    init_pair(CLR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
    init_pair(CLR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CLR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(CLR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(CLR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(CLR_LINES, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CLR_LINES_PLAYABLE, COLOR_BLUE, COLOR_BLACK);
    init_pair(CLR_CASE, COLOR_WHITE, COLOR_BLACK);
    init_pair(CLR_CASE_SELECTED, COLOR_GREEN, COLOR_BLACK);
    init_pair(CLR_CASE_EMPTY_SELECTED, COLOR_BLUE, COLOR_GREEN);
    init_pair(CLR_MESSAGE, COLOR_WHITE, COLOR_BLACK);
    init_pair(CLR_MESSAGE_ERROR, COLOR_RED, COLOR_BLACK);
    init_pair(CLR_MESSAGE_SUCCESS, COLOR_GREEN, COLOR_BLACK);

    box(win_grid, 0, 0);
    box(win_message, 0, 0);
    refresh();
    ui_refresh();
}

extern void ui_close()
{
    clrtoeol();
    ui_refresh();
    delwin(win_grid);
    delwin(win_message);
    endwin();
}

extern void ui_refresh()
{
    wrefresh(win_grid);
    wrefresh(win_message);
    wrefresh(win_title);
    refresh();
}

extern Action ui_getAction()
{
    int ch = wgetch(win_grid);
    switch (ch)
    {
    case KEY_LEFT:
    case 'q':
        return Action_LEFT;

    case KEY_RIGHT:
    case 'd':
        return Action_RIGHT;

    case KEY_UP:
    case 'z':
        return Action_UP;

    case KEY_DOWN:
    case 's':
        return Action_DOWN;

    case ENTER_KEY:
    case ' ':
        return Action_VALID;

    case 'y':
        return Action_YES;

    case 'h':
        return Action_TOGGLE_HELP;

    case ESCAPE_KEY:
        return Action_CANCEL;

    case KEY_BACKSPACE: // work on linux
    case 127:           // for macosx
        return Action_UNDO;

    default:
        ui_refresh();
        return Action_NONE;
    }
    return Action_NONE;
}

extern void ui_printMessage_info(char *str)
{
    setColor(win_message, CLR_MESSAGE);
    ui_printMessage(str);
}
extern void ui_printMessage_success(char *str)
{
    setColor(win_message, CLR_MESSAGE_SUCCESS);
    ui_printMessage(str);
}
extern void ui_printMessage_error(char *str)
{
    setColor(win_message, CLR_MESSAGE_ERROR);
    ui_printMessage(str);
}

extern void ui_printInfos(Game *game)
{
    int saved = game_getLinesCount(game) > 0;
    GameMode mode = game_getMode(game);
    PlayEvaluation evalution = game_getLastPlayEvaluation(game);
    char buf[BUFFER_SIZE];
    int x;
    setColor(win_title, CLR_DEFAULT);
    mvwhline(win_title, 0, 1, ' ', WIN_TITLE_WIDTH);
    mvwhline(win_title, 1, 1, ' ', WIN_TITLE_WIDTH);

    char *filepath = game_getFilepath(game);
    if (saved && filepath)
    {
        setColor(win_title, CLR_BLUE);
        snprintf(buf, BUFFER_SIZE, "save: %s", filepath);
        x = (WIN_TITLE_WIDTH - strlen(buf)) / 2;
        mvwprintw(win_title, 0, x, buf);
    }
    else
    {
        setColor(win_title, CLR_YELLOW);
        x = (WIN_TITLE_WIDTH - strlen(game_getNickname(game)) - 7) / 2;
        mvwprintw(win_title, 0, x, "Hello,");
        wattron(win_title, A_BOLD);
        mvwprintw(win_title, 0, x + 7, game_getNickname(game));
        wattroff(win_title, A_BOLD);
    }

    setColor(win_title, CLR_DEFAULT);
    snprintf(buf, BUFFER_SIZE, "score: ");
    mvwprintw(win_title, 1, 1, buf);
    wattron(win_title, A_BOLD);
    snprintf(buf, BUFFER_SIZE, "%d", game_getScore(game));
    mvwprintw(win_title, 1, 8, buf);
    wattroff(win_title, A_BOLD);

    strcpy(buf, mode == GM_SOBER ? "sober" : "visual");
    setColor(win_title, mode == GM_SOBER ? CLR_YELLOW : CLR_BLUE);
    mvwprintw(win_title, 0, WIN_TITLE_WIDTH - strlen(buf), buf);

    if (mode == GM_VISUAL && evalution)
    {
        if (evalution == PE_BAD)
            snprintf(buf, BUFFER_SIZE, "bad...");
        else if (evalution == PE_ORDINARY)
            snprintf(buf, BUFFER_SIZE, "ordinary.");
        else if (evalution == PE_GREAT)
            snprintf(buf, BUFFER_SIZE, "great :)");
        else if (evalution == PE_IMPRESSIVE)
            snprintf(buf, BUFFER_SIZE, "impressive!");
        else if (evalution == PE_AWESOME)
            snprintf(buf, BUFFER_SIZE, "awesome !!!");
        setColor(win_title, CLR_DEFAULT);
        mvwprintw(win_title, 1, WIN_TITLE_WIDTH - strlen(buf) - 14, "Last play was ");
        wattron(win_title, A_BOLD);
        mvwprintw(win_title, 1, WIN_TITLE_WIDTH - strlen(buf), buf);
        wattroff(win_title, A_BOLD);
    }

    setColor(win_title, CLR_DEFAULT);
    snprintf(buf, BUFFER_SIZE, "lines:");
    mvwprintw(win_title, 0, 1, buf);
    wattron(win_title, A_BOLD);
    snprintf(buf, BUFFER_SIZE, "%d", game_getLinesCount(game));
    mvwprintw(win_title, 0, 8, buf);
    wattroff(win_title, A_BOLD);
}

extern void ui_updateGrid(Game *game)
{
    Point points[GRID_SIZE * GRID_SIZE];
    int npoints;

    Point p;
    Grid baseGrid;
    game_initGrid(&baseGrid);

    Grid *grid = game_getGrid(game);
    int length, i, j;
    Line *lines, possibleLinesForSelect[8 * LINE_LENGTH];
    int nlinesForSelect;
    int displayPossibilities = game_mustDisplayPossibilities(game);
    int possibilitiesOnHover = point_exists(game_getSelect(game));

    cleanGrid();

    if (displayPossibilities && !possibilitiesOnHover)
    {
        lines = game_getAllPossibilities(game, &length);
        setColor(win_grid, CLR_LINES_PLAYABLE);
        drawLines(lines, length);
    }

    lines = game_getLines(game, &length);
    setColor(win_grid, CLR_LINES);
    drawLines(lines, length);

    npoints = 0;
    for (p.y = 0; p.y < GRID_SIZE; ++p.y)
        for (p.x = 0; p.x < GRID_SIZE; ++p.x)
            if (game_isOccupied(game, p))
                points[npoints++] = p;
    drawPointsOnGrid(points, npoints, grid->cursor, grid->select, 'o', CLR_CASE_SELECTED, CLR_LINES);

    if (!point_equals(grid->cursor, grid->select) && point_indexOf(points, npoints, grid->cursor) == -1)
    {
        wattron(win_grid, A_REVERSE);
        setColor(win_grid, CLR_CASE);
        drawPoint(grid->cursor, ' ');
        wattroff(win_grid, A_REVERSE);
    }
    if (point_indexOf(points, npoints, grid->select) == -1)
    {
        setColor(win_grid, CLR_CASE_EMPTY_SELECTED);
        drawPoint(grid->select, ' ');
    }

    npoints = 0;
    for (p.y = 0; p.y < GRID_SIZE; ++p.y)
        for (p.x = 0; p.x < GRID_SIZE; ++p.x)
            if (baseGrid.grid[p.x][p.y] == CASE_OCCUPIED)
                points[npoints++] = p;
    drawPointsOnGrid(points, npoints, grid->cursor, grid->select, 'o', CLR_CASE_SELECTED, CLR_CASE);

    if (displayPossibilities)
    {
        lines = game_getAllPossibilities(game, &length);
        if (possibilitiesOnHover)
        {
            setColor(win_grid, CLR_LINES_PLAYABLE);
            drawLines(lines, length);
        }

        npoints = 0;
        for (i = 0; i < length; ++i)
        {
            for (j = 0; j < LINE_LENGTH; ++j)
            {
                p = lines[i].points[j];
                if (!game_isOccupied(game, p) && point_indexOf(points, npoints, p) == -1)
                    points[npoints++] = p;
            }
        }
        drawPointsOnGrid(points, npoints, grid->cursor, grid->select, '*', CLR_CASE_SELECTED, CLR_LINES_PLAYABLE);

        if (point_exists(grid->select))
        {
            nlinesForSelect = 0;
            npoints = 0;
            for (i = 0; i < length && nlinesForSelect < 8 * LINE_LENGTH; ++i)
                if (line_pointAtExtremity(lines[i], grid->select))
                {
                    possibleLinesForSelect[nlinesForSelect++] = lines[i];
                    for (j = 0; j < LINE_LENGTH; ++j)
                    {
                        p = lines[i].points[j];
                        if (!game_isOccupied(game, p) && point_indexOf(points, npoints, p) == -1)
                            points[npoints++] = p;
                    }
                }
            wattron(win_grid, A_BOLD);
            drawLines(possibleLinesForSelect, nlinesForSelect);
            drawPointsOnGrid(points, npoints, grid->cursor, grid->select, '*', CLR_CASE_SELECTED, CLR_LINES_PLAYABLE);
            wattroff(win_grid, A_BOLD);
        }
    }

    wattroff(win_grid, A_REVERSE);
}
