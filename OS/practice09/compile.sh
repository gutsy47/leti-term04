#!/bin/bash

g++ -o main main.cpp
if [ $? -eq 0 ]; then
    echo "Компиляция прошла успешно."
else
    echo "Ошибка компиляции."
    exit 1
fi

# Set root capability for CLONE_NEWPID
echo "Функция создания нового пространства имен PID требует прав администратора."
echo "Для этого можно либо установить CAP_SYS_ADMIN=eip для файла, либо запускать файл через 'sudo'"
read -p "Установить CAP_SYS_ADMIN=eip для исполняемого файла? [y/n]: " response
if [[ $response =~ ^[YyДд]$ ]]; then
    sudo setcap cap_sys_admin=eip main
    if [ $? -ne 0 ]; then
        echo "Не удалось установить права. Программу необходимо запустить через 'sudo ./main'"
        exit 1
    fi
    echo "Права успешно установлены."
else
    echo "Установка прав отменена пользователем."
fi