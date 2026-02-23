#!/bin/bash

trap terminate SIGINT

mkfifo p1 p2
./controller.o 1>p1 0<p2 &
CONTROLLER_PID=$!
./heater.o 1>p2 0<p1 &
HEATER_PID=$!

terminate(){
  kill $CONTROLLER_PID $HEATER_PID
  rm p1 p2
  exit
}

(exec 30<p1 31<p2)
cat 1>/dev/null
