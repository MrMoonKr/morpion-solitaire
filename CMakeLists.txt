cmake_minimum_required( VERSION 3.30 )

project( console-solitaire
    LANGUAGES C CXX
    VERSION 0.1.0
    DESCRIPTION "Console Based Solitaire Game"
)

include( FindCurses )
include_directories( ${CURSES_INCLUDE_DIR} )

set(CURSES_NEED_NCURSES TRUE)

# Define NCURSES_STATIC
add_definitions(-DNCURSES_STATIC)

add_executable( solitaire
    export.h export.c
    game.h game.c
    globals.h
    highscore.h highscore.c
    main.c
    points.h points.c
    ui.h ui.c
    utils.h utils.c
)

target_link_libraries( solitaire ${CURSES_LIBRARIES} )
