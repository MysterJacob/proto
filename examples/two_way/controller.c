#include <asm-generic/ioctls.h>
#include <stdint.h>
#include <stdio.h>
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
    case HumidityPacket_ID: {
      HumidityPacket *hp = packetData;
      data.humidity = hp->Humidity;
    } break;
    case TemperaturePacket_ID: {
      TemperaturePacket *tp = packetData;
      data.temperature = tp->Temperature;
    } break;
  }
}
void errorHandler(const protoErrorCode errorCode)
{
  // ERROR!
  fprintf(stderr, "Controller: Error %d while receiving\n", errorCode);
  _exit(0x100 | errorCode);
}
int readData(FILE *stream)
{
  int b;
  if(ioctl(STDIN_FILENO, FIONREAD, &b) == -1) {
    return 0;
  }
  while(b--) {
    processByte(getc(stream));
  }
  return 1;
}
void loop()
{
  int diffHumidity = 128 + (data.humidity - (1 << 14)) / (1 << 6);
  int diffTemperature = 128 - (data.temperature - (1 << 14)) / (1 << 6);

  data.fanRpm = diffHumidity;
  data.heaterValue = diffTemperature;
  fprintf(
      stderr,
      "Controller: Temperature:%d Heater Value: %d Humidity:%d Fan RPM: %d\n",
      data.temperature, data.heaterValue, data.humidity, data.fanRpm);
}
void sendData(FILE *stream)
{
  size_t size;

  FanPacket tp = {data.fanRpm};
  void *packet = generatePacket(FanPacket_ID, (void *)&tp, &size);
  fwrite(packet, sizeof(byte), size, stream);

  HeaterPacket hp = {data.heaterValue};
  packet = generatePacket(HeaterPacket_ID, (void *)&hp, &size);
  fwrite(packet, sizeof(byte), size, stream);

  fflush(stream);
}
int main()
{
  setErrorCallback(errorHandler);
  setPacketCallback(packetHandler);
  FILE *streamin = freopen(NULL, "rb", stdin);
  FILE *streamout = freopen(NULL, "wb", stdout);

  while(1) {
    readData(streamin);
    loop();
    sendData(streamout);
    sleep(1);
  }
}
