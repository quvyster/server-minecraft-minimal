# Windows Compilation Guide (MSYS2)

## Установка MSYS2

### Шаг 1: Загрузите MSYS2

Перейдите на https://www.msys2.org/ и загрузите установщик

### Шаг 2: Установите MSYS2

1. Запустите установщик
2. Установите в папку по умолчанию: `C:\msys64`
3. Оставьте галочку "Run MSYS2 now"

### Шаг 3: Установите компилятор

В окне MSYS2 MINGW64 выполните:

```bash
# Обновить менеджер пакетов
pacman -Syu

# Установить компилятор и необходимые инструменты
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

На вопрос "Proceed with installation?" ответьте: `y`

## Компиляция проекта

### Способ 1: Через batch файл (рекомендуется)

```bash
# В командной строке Windows (cmd.exe)
cd c:\Users\quvys\Desktop\server-minecraft-minimal
build.bat
```

### Способ 2: Через MSYS2 терминал

```bash
# Откройте MSYS2 MINGW64
cd /c/Users/quvys/Desktop/server-minecraft-minimal
make clean
make -j4
```

### Способ 3: WSL (Windows Subsystem for Linux)

```bash
# Если установлен WSL2 с Ubuntu
wsl
cd /mnt/c/Users/quvys/Desktop/server-minecraft-minimal
sudo apt-get install build-essential
make
```

## Проверка компиляции

После успешной компиляции вы должны увидеть:

```
[CC] Компилирование src/main.c...
[CC] Компилирование src/server.c...
[CC] Компилирование src/player.c...
[CC] Компилирование src/chunk.c...
[CC] Компилирование src/protocol.c...
[LD] Линковка build/server...
[OK] Сервер создан: build/server
Размер: 512KB
```

Исполняемый файл будет: `build\server.exe`

## Запуск сервера

### Из Command Prompt:

```bash
cd c:\Users\quvys\Desktop\server-minecraft-minimal
build\server.exe
```

### Из PowerShell:

```powershell
cd c:\Users\quvys\Desktop\server-minecraft-minimal
.\build\server.exe
```

### Из MSYS2:

```bash
cd /c/Users/quvys/Desktop/server-minecraft-minimal
./build/server
```

## Трубление при компиляции

### Ошибка: `gcc: command not found`

**Решение:**
1. Убедитесь что установлен GCC: `pacman -S mingw-w64-x86_64-gcc`
2. Перезагрузите MSYS2
3. Проверьте версию: `gcc --version`

### Ошибка: `make: command not found`

**Решение:**
```bash
pacman -S mingw-w64-x86_64-make
```

### Ошибка: `cannot open file 'XXX.c'`

**Решение:**
- Убедитесь что вы в правильной папке
- Используйте полный путь: `C:\Users\quvys\Desktop\server-minecraft-minimal`
- Путь не должен содержать специальные символы

### Ошибка при линковке (`undefined reference`)

**Решение:**
- Убедитесь что все .c файлы скомпилированы
- Проверьте что все .h файлы включены правильно
- Пересоберите: `make clean && make`

## Оптимизация под Windows

В `Makefile` можно добавить флаги для Windows:

```makefile
# Для Windows добавьте:
CFLAGS += -mwindows -mconsole

# Для динамической линковки:
LDFLAGS += -static-libgcc -static-libstdc++
```

Затем пересоберите:
```bash
make clean && make
```

## Создание service на Windows

Если хотите запускать как сервис:

### Используя NSSM (Non-Sucking Service Manager):

```bash
# Загрузить с https://nssm.cc/
nssm install MinecraftServer C:\Users\quvys\Desktop\server-minecraft-minimal\build\server.exe
nssm start MinecraftServer
```

### Или через Task Scheduler:

1. Откройте Task Scheduler (Планировщик заданий)
2. Create Basic Task
3. Trigger: At log on
4. Action: Start a program
5. Program: `C:\Users\quvys\Desktop\server-minecraft-minimal\build\server.exe`

## Проверка в Windows

### PowerShell:

```powershell
# Проверить что сервер слушает на порту 25565
netstat -ano | findstr :25565

# Если видите строку типа:
# TCP    0.0.0.0:25565    0.0.0.0:0    LISTENING    12345
# То сервер работает!
```

### Command Prompt:

```bash
# Посмотреть использование памяти
tasklist | findstr server

# Посмотреть активные сокеты
netstat -ab | findstr :25565
```

## Тестирование подключения

```bash
# Пингуем сервер
ping localhost

# Проверяем что порт открыт
netstat -ano | findstr 25565

# Используя PowerShell
Test-NetConnection -ComputerName localhost -Port 25565
```

## Оптимизация производительности на Windows

### 1. Увеличить приоритет процесса

```powershell
# В PowerShell (от администратора)
$proc = Get-Process server
$proc.ProcessorAffinity = [Convert]::ToInt32("00001111",2)  # использовать ядра 0-3
$proc.PriorityClass = [Diagnostics.ProcessPriorityClass]::High
```

### 2. Отключить спящий режим

```bash
powercfg /change monitor-timeout-ac 0
powercfg /change disk-timeout-ac 0
powercfg /change standby-timeout-ac 0
```

### 3. Проверить виртуализацию памяти

Убедитесь что Windows может использовать весь объём RAM для сервера.

## Запуск с логированием

Для сохранения логов в файл:

```bash
# Command Prompt
cd c:\Users\quvys\Desktop\server-minecraft-minimal
build\server.exe > server.log 2>&1

# PowerShell
& '.\build\server.exe' | Tee-Object -FilePath server.log
```

## Релиз для распространения

Если хотите создать exe который запускается везде без MSYS2:

```bash
# Скомпилировать с статической линковкой
gcc -static -O3 src/*.c -o build/server.exe

# Размер будет ~3MB (вместо 512KB без статики)
# Зато не нужны DLL
```

## Антивирус

Если Windows Defender блокирует исполняемый файл:

1. Откройте Windows Security
2. Перейдите в "Virus & threat protection"
3. "Manage settings"
4. "Add exclusions"
5. Добавьте папку проекта

---

**Готово!** Теперь вы можете развивать сервер на Windows 🎉
