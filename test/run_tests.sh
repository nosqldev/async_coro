#!/bin/bash

cputs()
{
    printf "\033[01;36;40m$1\033[00m\n"
}

gputs()
{
    printf "\033[01;32m$1\033[00m\n"
}

yputs()
{
    printf "\033[01;33m$1\033[00m\n"
}

rputs()
{
    printf "\033[01;31;40m$1\033[00m\n"
}

warn_puts()
{
    printf "\033[01;33;41m$1\033[00m\n"
}

compile_obj()
{
    src=$1

    echo -n "compiling "
    cputs "$src.c"
    cc -Wall -Wextra -c -g -std=gnu99 -D_GNU_SOURCE -DTESTMODE -I../src -I../vendor ../src/${src}.c -o ${src}.o
}

CNT=0

run_ts()
{
    rm -rf data
    if [ -x $1 ]
    then
        CNT=$[ $CNT + 1 ]
        yputs "\n                 ----- [$CNT] $1 -----"
        $1 | sed -n '7,$p' | perl -lne 's/asserts(\s+\d+)(\s+\d+)(\s+\d+)(\s+\d+)(\s+.*)$/asserts\033[01;32m\1\033[00m\033[01;32m\2\033[00m\033[01;34m\3\033[00m\033[01;31m\4\033[00m\033[01;32m\5\033[00m/g; print'
    else
        warn_puts "\n ####### $1 doesn't exists ########\n"
        exit
    fi

    if [ $2 ]
    then
        rm -f $1
    fi
}

cleanup_env()
{
    rm -f core*
    rm -f *.o
}

gputs "remove temp files"
cleanup_env

gputs "compile necessary obj files\n"
cc -c -g ../vendor/ev.c -o ./ev.o
compile_obj "acoro"

gputs "\ncompile test suits"
printf "\033[01;31;40m" # red
cc -Wall -Wextra -g -std=gnu99 -D_GNU_SOURCE -lpthread -lcunit -lm -I../src -I../vendor test_acoro.c acoro.o ev.o -o test_acoro
printf "\033[00m"

run_ts "./test_acoro" del

gputs "finished"
cleanup_env
