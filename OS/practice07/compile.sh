#!/bin/bash

# Compile programs
g++ -D MODE=1 -o writer main.cpp -lcap
g++ -D MODE=0 -o reader main.cpp -lcap
if [ $? -ne 0 ]; then
  echo "Ошибка компиляции. Необходима библиотека libcap для проверки прав программы (CAP_SYS_RESOURCE)"
  exit 1
fi
echo "Компиляция прошла успешно."

# Set capabilities
echo "Установка CAP_SYS_RESOURCE позволит превысить стандартный предел размера очереди mq_maxmsg = 10"
read -p "Установить CAP_SYS_RESOURCE=eip для исполняемых файлов? [y/n]: " response
if [[ $response =~ ^[YyДд]$ ]]; then
    sudo setcap cap_sys_resource=eip writer
    if [ $? -ne 0 ]; then
        echo "Не удалось установить права. Программа установит размер очереди mq_maxmsg = 5"
        exit 1
    fi
    sudo setcap cap_sys_resource=eip reader
    if [ $? -ne 0 ]; then
        echo "Не удалось установить права. Программа установит размер очереди mq_maxmsg = 5"
        exit 1
    fi
    echo "Права успешно установлены"
else
    echo "Установка прав отменена пользователем. Программа установит размер очереди mq_maxmsg = 5"
fi