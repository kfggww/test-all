#!/bin/bash

# 显示TODO列表. 显示最近几天; 按照日期精确显示.

# 添加TODO项目. 添加今天的TODO项目, 从0开始编号.

# 修改TODO项目. 修改今天的TODO项目, 输入要修改的编号.

# 删除DOTO项目. 删除今天的某个TODO项目, 同时今天其他项目的更新编号.

# 数据统计和展示. 待定, 可以使用nlp提取关键词, 用其他工具展示统计结果.

# 显示帮助菜单.

todo_file=""
command=""
debug="true"

function show_list() {
    echo ""
}

function add_todo() {
    echo ""
}

function update_todo() {
    echo ""
}

function remove_todo() {
    echo ""
}

function show_help() {
    echo "---------------------------"
    printf "<<< Menus:\n"
    printf "<<< \t0) show n;\n"
    printf "<<< \t1) add;\n"
    printf "<<< \t2) update id;\n"
    printf "<<< \t3) delete id;\n"
    printf "<<< \t4) quit;\n"
    printf "<<< \t5) help;\n"
    echo "---------------------------"
}

function get_command() {
    printf ">>> "
    read command
    local commands=(${command})

    case ${commands[0]} in
    "show")
        show_list ${commands[*]}
        ;;
    "add")
        add_todo ${commands[*]}
        ;;
    "update")
        update_todo ${commands[*]}
        ;;
    "quit")
        printf "<<< Bye\n"
        return
        ;;
    "help")
        show_help
        ;;
    *)
        printf "<<< unknown command\n\n"
        show_help
        ;;
    esac
}

function main() {
    if [ "$debug" = "true" ]; then
        todo_file=".todo_list.json"
    else
        todo_file="~/.todo_list.json"
    fi

    show_help
    while [ "$command" != "quit" ]; do
        get_command
    done
}

main
