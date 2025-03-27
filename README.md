# Лабораторная работа 4: Система сбора температурных данных

Кроссплатформенная служба для мониторинга температурных показателей через последовательный порт с поддержкой временных интервалов и сохранением данных.

## Сборка и запуск

```bash
# Сборка проекта
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Запуск основной службы (требуются права на доступ к COM-порту)
./src/main -c config.json

# Запуск симулятора температурных данных
./src/tools/simulator /dev/ttyUSB0 9600
```

## Основные особенности
- Чтение данных с последовательного порта (COM-порты/терминальные устройства)
- Трехуровневое хранение данных:
  - Сырые показания (current.log)
  - Часовые средние (hourly.log)
  - Дневные средние (daily.log)
- Механизм очистки устаревших данных:
  - Сырые данные - 1 день
  - Часовые средние - 30 дней
  - Дневные средние - 1 год
- Кроссплатформенная реализация (Windows/Linux)
- Конфигурация через JSON-файл

## Пример конфигурации (config.json)
```json
{
    "serial_port": "/dev/ttyUSB0",
    "baud_rate": 9600,
    "mesure_delay": 100,
    "temperature_path": "/var/log/temperature/current.log",
    "hourly_path": "/var/log/temperature/hourly.log",
    "daily_path": "/var/log/temperature/daily.log"
}
```

## Ожидаемое поведение
```
[INFO] Starting service with measurement interval 100 milliseconds
[INFO] Reading: 24.5
[INFO] Got 1428 readings from file /var/log/temperature/current.log
[INFO] Sent temperature: 24.5C (Simulator)
```

## Проверка работы
1. Убедитесь в создании файлов данных:
```
/var/log/temperature/
├── current.log
├── hourly.log
└── daily.log
```

2. Формат записей в логах:
```
2024-03-15T14:30:00Z 24.5
2024-03-15T15:00:00Z 24.7
```
