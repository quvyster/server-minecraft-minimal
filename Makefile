# Makefile для Minecraft Server
# Оптимизирован для слабого железа (2GB RAM)

CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -flto \
         -ffast-math -ftree-vectorize \
         -fomit-frame-pointer -finline-functions \
         -DNDEBUG \
         -I./include

LDFLAGS = -lm -lpthread \
          -Wl,-O3 \
          -Wl,--sort-common \
          -Wl,--as-needed

# Источники
SOURCES = src/main.c \
          src/server.c \
          src/player.c \
          src/chunk.c \
          src/protocol.c \
          src/utils.c

# Объекты
OBJECTS = $(SOURCES:.c=.o)

# Выходной файл
OUTPUT = build/server

# Targets
all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	@mkdir -p build
	@echo "[LD] Линковка $@..."
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "[OK] Сервер создан: $@"
	@echo "Размер: $$(du -h $@ | cut -f1)"

%.o: %.c
	@echo "[CC] Компилирование $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	@echo "[CLEAN] Удаление файлов сборки..."
	@rm -f $(OBJECTS)
	@rm -f $(OUTPUT)
	@echo "[OK] Очищено"

# Сборка с дебагом
debug: CFLAGS = -Wall -Wextra -g -O0 -DDEBUG_LOG=1 -DDEBUG_MEMORY=1 -I./include
debug: clean $(OUTPUT)
	@echo "[OK] Debug сборка завершена"

# Запуск сервера
run: $(OUTPUT)
	@echo "[RUN] Запуск сервера..."
	@./$(OUTPUT)

# Запуск с дебагом
debug-run: debug
	@echo "[RUN] Запуск сервера в режиме дебага..."
	@./$(OUTPUT)

# Профилирование
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean $(OUTPUT)
	@echo "[OK] Профилирующая сборка завершена"

# Статистика
stats:
	@echo "[STATS] Статистика проекта:"
	@wc -l $(SOURCES) include/*.h
	@echo ""
	@echo "Опции компилятора:"
	@echo "  - O3: максимальная оптимизация"
	@echo "  - march=native: оптимизация под текущий процессор"
	@echo "  - flto: Link-Time Optimization"
	@echo "  - ffast-math: быстрая математика"
	@echo "  - ftree-vectorize: векторизация кода"
	@echo "  - fomit-frame-pointer: убрать frame pointer"

# Справка
help:
	@echo "Доступные команды:"
	@echo "  make              - собрать сервер"
	@echo "  make clean        - удалить скомпилированные файлы"
	@echo "  make run          - собрать и запустить сервер"
	@echo "  make debug        - собрать с информацией для дебага"
	@echo "  make debug-run    - собрать и запустить с дебагом"
	@echo "  make profile      - собрать с профилированием"
	@echo "  make stats        - показать статистику проекта"
	@echo "  make help         - показать эту справку"

.PHONY: all clean debug debug-run run profile stats help
