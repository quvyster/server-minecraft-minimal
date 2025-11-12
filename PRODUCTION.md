# 🚀 Production Deployment Guide

## Развёртывание на боевом сервере

### Требования

| Параметр | Минимум | Рекомендуется | Премиум |
|----------|---------|-------------|---------|
| **CPU** | 2 ядра | 4 ядра | 8+ ядер |
| **RAM** | 1GB | 2GB | 4-8GB |
| **Сеть** | 10 Mbps | 50 Mbps | 100+ Mbps |
| **Диск** | 10GB | 50GB SSD | 100GB NVMe |
| **ОС** | Linux | Linux | Linux |

### Рекомендуемые провайдеры

- **Hetzner** (дешево и надёжно)
- **DigitalOcean** (простой интерфейс)
- **Linode** (стабильно)
- **Vultr** (хорошая сеть)

## Установка на Linux сервер

### 1. Подключиться к серверу

```bash
ssh root@ВАШ_IP_АДРЕС
```

### 2. Установить зависимости

```bash
# Обновить систему
apt-get update
apt-get upgrade -y

# Установить необходимое
apt-get install -y build-essential git curl wget
```

### 3. Клонировать репозиторий

```bash
cd /opt
git clone <URL_РЕПОЗИТОРИЯ> minecraft-server
cd minecraft-server
```

### 4. Собрать сервер

```bash
make clean
make -j$(nproc)
```

### 5. Создать пользователя для сервера

```bash
useradd -m -s /bin/bash minecraft
chown -R minecraft:minecraft /opt/minecraft-server
```

### 6. Настроить firewall

```bash
# Если используется UFW
ufw allow 22/tcp      # SSH
ufw allow 25565/tcp   # Minecraft
ufw allow 25565/udp   # Minecraft UDP (если добавите)
ufw enable
```

### 7. Создать systemd сервис

```bash
sudo tee /etc/systemd/system/minecraft-server.service > /dev/null <<EOF
[Unit]
Description=Optimized Minecraft Server
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=minecraft
WorkingDirectory=/opt/minecraft-server
ExecStart=/opt/minecraft-server/build/server
Restart=always
RestartSec=5

# Ограничение ресурсов
MemoryLimit=2G
CPUQuota=80%

# Логирование
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Активировать сервис
systemctl daemon-reload
systemctl enable minecraft-server
systemctl start minecraft-server

# Проверить статус
systemctl status minecraft-server
```

### 8. Проверить логи

```bash
# Реал-тайм
journalctl -u minecraft-server -f

# Последние 100 строк
journalctl -u minecraft-server -n 100
```

## Мониторинг сервера

### Установить Prometheus + Grafana

```bash
# Установить Node Exporter
wget https://github.com/prometheus/node_exporter/releases/download/v1.0.1/node_exporter-1.0.1.linux-amd64.tar.gz
tar xvfz node_exporter-1.0.1.linux-amd64.tar.gz
sudo mv node_exporter-1.0.1.linux-amd64/node_exporter /usr/local/bin/

# Создать systemd для Node Exporter
sudo tee /etc/systemd/system/node_exporter.service > /dev/null <<EOF
[Unit]
Description=Node Exporter
After=network.target

[Service]
User=prometheus
ExecStart=/usr/local/bin/node_exporter
Restart=always

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable node_exporter
systemctl start node_exporter
```

### Мониторить вручную

```bash
# CPU и RAM каждые 5 секунд
watch -n 5 'ps aux | grep minecraft-server | grep -v grep'

# Использование памяти
free -h

# Процесс
top -p $(pgrep -f minecraft-server)

# Сетевые соединения
netstat -tn | grep 25565

# Активные игроки
lsof -i :25565
```

## Резервное копирование

### Автоматическое резервное копирование

```bash
#!/bin/bash
# backup.sh

BACKUP_DIR="/backups/minecraft"
SERVER_DIR="/opt/minecraft-server"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Остановить сервер
systemctl stop minecraft-server

# Скопировать данные
tar -czf $BACKUP_DIR/minecraft_$DATE.tar.gz $SERVER_DIR/world/

# Запустить сервер
systemctl start minecraft-server

# Удалить старые резервные копии (старше 7 дней)
find $BACKUP_DIR -name "minecraft_*.tar.gz" -mtime +7 -delete

echo "Резервная копия создана: minecraft_$DATE.tar.gz"
```

Добавить в cron:

```bash
# Резервная копия каждый день в 3:00
0 3 * * * /opt/minecraft-server/backup.sh
```

### Ручное резервное копирование

```bash
# На сервере
tar -czf world_$(date +%Y%m%d).tar.gz world/

# Скопировать на локальную машину
scp root@ВАШ_IP:/opt/minecraft-server/world_*.tar.gz ~/backups/
```

## Оптимизация для production

### 1. Увеличить открытые файлы

```bash
# /etc/security/limits.conf
minecraft soft nofile 65535
minecraft hard nofile 65535
```

### 2. Оптимизация ядра

```bash
# /etc/sysctl.conf
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65000
```

Применить:
```bash
sysctl -p
```

### 3. Отключить swap для стабильности

```bash
swapoff -a
# Отредактировать /etc/fstab и закомментировать swap строку
```

### 4. Переконфигурировать параметры для production

```c
// в include/globals.h для production
#define MAX_PLAYERS 1000
#define RENDER_DISTANCE 6
#define SAVE_INTERVAL 30000  // сохранять каждые 30 сек
#define ASYNC_SAVE 1
#define DEBUG_LOG 0          // выключить дебаг логи
```

Пересобрать:
```bash
make clean && make -j$(nproc)
systemctl restart minecraft-server
```

## Масштабирование на 1000+ игроков

### Вариант 1: Несколько инстансов (Proxy)

```
                    ┌─────────────┐
                    │   LB/Proxy  │
                    │  (Nginx)    │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
    ┌───▼──────┐      ┌────▼─────┐      ┌───▼──────┐
    │Instance 1│      │Instance 2 │      │Instance 3│
    │Port:2556│       │Port:2566  │      │Port:2576 │
    │Players:333│       │Players:333│      │Players:334│
    └──────────┘      └─────────┘      └──────────┘
        World_1          World_2          World_3
```

### Вариант 2: Одна большая машина

```
Максимальное использование одного сервера:
- 8 ядер CPU
- 16GB RAM
- 100 Mbps сеть
= ~5000 игроков (при оптимальной конфигурации)
```

## Обновление сервера

```bash
cd /opt/minecraft-server

# Получить обновления
git pull origin main

# Остановить сервер
systemctl stop minecraft-server

# Пересобрать
make clean && make -j$(nproc)

# Запустить
systemctl start minecraft-server

# Проверить логи
systemctl status minecraft-server
```

## Откат при проблемах

```bash
# Если всё сломалось
git reset --hard HEAD~1
make clean && make
systemctl restart minecraft-server
```

## Security

### Firewall правила (UFW)

```bash
# Разрешить только нужные порты
ufw allow from 203.0.113.0/24 to any port 22
ufw allow 25565/tcp
ufw deny incoming
ufw allow outgoing
ufw enable
```

### Fail2Ban (против брутфорса)

```bash
apt-get install fail2ban

# Создать фильтр для Minecraft
sudo tee /etc/fail2ban/filter.d/minecraft.conf > /dev/null <<EOF
[Definition]
failregex = \[NETWORK\] Invalid packet from <HOST>
ignoreregex =
EOF

# Создать jail
sudo tee /etc/fail2ban/jail.d/minecraft.conf > /dev/null <<EOF
[minecraft]
enabled = true
port = 25565
filter = minecraft
maxretry = 10
findtime = 3600
bantime = 3600
EOF

systemctl restart fail2ban
```

## Обновления и патчи

```bash
# Автоматические обновления безопасности
apt-get install unattended-upgrades
dpkg-reconfigure -plow unattended-upgrades
```

## Проверка здоровья сервера

```bash
#!/bin/bash
# healthcheck.sh

# Проверка процесса
if ! pgrep -f minecraft-server > /dev/null; then
    echo "CRITICAL: Сервер не запущен!"
    systemctl restart minecraft-server
    exit 1
fi

# Проверка памяти
MEMORY=$(ps aux | grep minecraft-server | awk '{print $6}' | tail -1)
if [ $MEMORY -gt 1900000 ]; then  # > 1.9GB
    echo "WARNING: Высокое использование памяти: ${MEMORY}KB"
fi

echo "OK: Сервер здоров"
```

Добавить в cron каждые 5 минут:
```bash
*/5 * * * * /opt/minecraft-server/healthcheck.sh
```

---

**Готово к боевому использованию!** 🎮🚀

Для поддержки и помощи открывайте issues на GitHub.
