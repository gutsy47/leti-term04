#!/bin/bash

echo "Текущий PID: $$"
echo "Попытка использовать 'unshare --pid --fork /bin/bash'"
sudo unshare --pid --fork /bin/bash -c 'echo -e "Fork PID: $BASHPID\nЗакрываем изолированный процесс..."; exit'
echo "Текущий PID остался $$"