#!/bin/bash

LOG_FILE="perf_monitor_$(date +%Y%m%d_%H%M%S).log"
INTERVAL=2  # 每2秒采样一次

#echo "时间,CPU温度(℃),CPU使用率(%),内存使用(MB),磁盘读(MB/s),磁盘写(MB/s),LoadAvg(1m)" > $LOG_FILE

while true; do
  TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

  # 1️⃣ CPU 温度
  RAW_TEMP=$(adb shell cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)
  TEMP_C=$(echo "scale=1; $RAW_TEMP / 1000" | bc)

  # 2️⃣ CPU 使用率
  CPU_LINE=$(adb shell top -n 1 | grep -m 1 '%cpu')
  USER=$(echo "$CPU_LINE" | grep -oE '[0-9]+%user' | grep -oE '[0-9]+')
  SYS=$(echo "$CPU_LINE" | grep -oE '[0-9]+%sys' | grep -oE '[0-9]+')
  CPU_USAGE=$((USER + SYS))

  # 3️⃣ 内存使用
  MEM_INFO=$(adb shell cat /proc/meminfo)
  MEM_TOTAL=$(echo "$MEM_INFO" | grep MemTotal | awk '{print $2}')
  MEM_FREE=$(echo "$MEM_INFO" | grep MemAvailable | awk '{print $2}')
  MEM_USED_MB=$(( (MEM_TOTAL - MEM_FREE) / 1024 ))

  # 4️⃣ 磁盘读写
  READ_MB="N/A"
  WRITE_MB="N/A"

  # 5️⃣ Load average
  LOAD_AVG=$(adb shell cat /proc/loadavg | awk '{print $1}')

  # 6️⃣ 输出到控制台
  echo -e "\r[$TIMESTAMP] 温度:${TEMP_C}℃,占用内存:${MEM_USED_MB}MB,占用CPU:${CPU_USAGE}%,磁盘读:${READ_MB}MB/s,磁盘写:${WRITE_MB}MB/s,平均负载: ${LOAD_AVG} \c"

  # 7️⃣ 写入日志
  #echo "$TIMESTAMP,$TEMP_C,$CPU_USAGE,$MEM_USED_MB,$READ_MB,$WRITE_MB,$LOAD_AVG" >> $LOG_FILE

  sleep $INTERVAL
done