#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lib/proto.h"
struct {
  uint8_t fanRpm;
  uint8_t heaterValue;
  uint16_t temperature;
  uint16_t humidity;
} data;

void packetHandler(const PacketHeader header, void *packetData)
{
  switch(header.id) {
    case FanPacket_ID: {
      FanPacket *fp = packetData;
      data.fanRpm = fp->SetRPM;
    } break;
    case HeaterPacket_ID: {
      HeaterPacket *fp = packetData;
      data.heaterValue = fp->SetValue;
    } break;
  }
}
void errorHandler(const protoErrorCode errorCode)
{
  // ERROR!
  fprintf(stderr, "Heater: Error %d while receiving\n", errorCode);
  _exit(0x100 | errorCode);
}
void readData(FILE *stream)
{
  int b;
  if(ioctl(STDIN_FILENO, FIONREAD, &b) == -1) {
    fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
    return;
  }
  while(b--) {
    processByte(getc(stream));
  }
}
void loop()
{
  data.humidity -= data.fanRpm - 128 + (data.temperature >> 13);
  data.temperature += data.heaterValue - 128;

  if(data.humidity > 35000) {
    fputs("Too humid!\n", stderr);
    _exit(1);
  }

  if(data.humidity < 25000) {
    fputs("Too dry!\n", stderr);
    _exit(2);
  }

  if(data.temperature > 35000) {
    fputs("Too hot!\n", stderr);
    _exit(3);
  }

  if(data.temperature < 25000) {
    fputs("Too cold!\n", stderr);
    _exit(4);
  }

  fprintf(stderr,
          "Heater: Temperature:%d Heater Value: %d Humidity:%d Fan RPM:%d\n",
          data.temperature, data.heaterValue, data.humidity, data.fanRpm);
}
void sendData(FILE *stream)
{
  size_t size;

  TemperaturePacket tp = {data.temperature};
  void *packet = generatePacket(TemperaturePacket_ID, (void *)&tp, &size);
  fwrite(packet, sizeof(byte), size, stream);

  HumidityPacket hp = {data.humidity};
  packet = generatePacket(HumidityPacket_ID, (void *)&hp, &size);
  fwrite(packet, sizeof(byte), size, stream);

  fflush(stream);
}
int main()
{
  setErrorCallback(errorHandler);
  setPacketCallback(packetHandler);

  data.temperature = (2 << 14);
  data.humidity = (2 << 14);
  FILE *streamin = freopen(NULL, "rb", stdin);
  FILE *streamout = freopen(NULL, "wb", stdout);

  while(1) {
    loop();
    sendData(streamout);
    readData(streamin);
    sleep(1);
  }
}
