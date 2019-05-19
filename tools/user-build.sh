#!/bin/bash

cmocka()
{
    cmocka_path=$PROJECT_DIR/deps/cmocka

    if [ ! "$(find $PROJECT_DIR/lib* -maxdepth 1 -name *${FUNCNAME[0]}*)" ]; then
        mkdir -p $cmocka_path/build && cd $cmocka_path/build
        cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$PROJECT_DIR
        make -j$JOBS && make install
        [ ! $? -eq 0 ] && exit 1
    fi
}

extlib()
{
    mkdir -p $PROJECT_DIR/build && cd $PROJECT_DIR/build
    cmake .. -DBUILD_TESTS=on -DBUILD_DEBUG=$DEBUG && make -j$JOBS && make test
    [ ! $? -eq 0 ] && exit 1
}

main()
{
    do_build cmocka
    do_build extlib
}
