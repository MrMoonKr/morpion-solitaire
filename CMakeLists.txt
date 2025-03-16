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

add_executable( particles 
    ./src/particles.cpp
)

target_link_libraries( solitaire ${CURSES_LIBRARIES} )
target_link_libraries( particles ${CURSES_LIBRARIES} )

add_executable( windowing 
    ./src/windowing.cpp
)
target_link_libraries( windowing ${CURSES_LIBRARIES} )

add_executable( window-resize 
    ./src/window-resize.cpp
)
target_link_libraries( window-resize ${CURSES_LIBRARIES} )

