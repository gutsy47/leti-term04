Программа ожидает от пользователя консольного аргумента.
Аргумент указывает способ инициализации неименованного канала.

Значения аргументов:
- `pipe`  - запись и чтение выполняются с блокировками;
- `pipe2` - запись и чтение выполняются без блокировок (только Linux);
- `fcntl` - запись и чтение выполняются без блокировок (UNIX-системы).

Пример использования:
```bash
./main pipe2
```