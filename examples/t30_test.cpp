#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "CYdLidar.h"
#include "core/common/ydlidar_help.h"

using namespace std;
using namespace ydlidar;
using namespace ydlidar::core::common;

#if defined(_MSC_VER)
#pragma comment(lib, "ydlidar_sdk.lib")
#endif


int main(int argc, char *argv[])
{
  printLogo();
  os_init();

  std::string port;
  std::map<std::string, std::string> ports =
    ydlidar::lidarPortList();
  std::map<std::string, std::string>::iterator it;

  if (ports.size() == 1) {
    port = ports.begin()->second;
  } else {
    int id = 0;

    for (it = ports.begin(); it != ports.end(); it++) {
      printf("[%d] %s %s\n", id, it->first.c_str(), it->second.c_str());
      id++;
    }

    if (ports.empty()) {
      printf("Not Lidar was detected. Please enter the lidar serial port:");
      std::cin >> port;
    } else {
      while (ydlidar::os_isOk()) {
        printf("Please select the lidar port:");
        std::string number;
        std::cin >> number;

        if ((size_t)atoi(number.c_str()) >= ports.size()) {
          continue;
        }

        it = ports.begin();
        id = atoi(number.c_str());

        while (id) {
          id--;
          it++;
        }

        port = it->second;
        break;
      }
    }
  }

  // T30 specific settings according to Dataset.md
  int baudrate = 8000;  // T30 uses 8000 baudrate
  bool isSingleChannel = true;  // T30 is single channel
  float frequency = 10.0;  // 5-35 Hz supported

  CYdLidar laser;
  //////////////////////string property/////////////////
  /// lidar port
  laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
  /// ignore array
  std::string ignore_array;
  ignore_array.clear();
  laser.setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(),
                    ignore_array.size());

  //////////////////////int property/////////////////
  /// lidar baudrate
  laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
  // T30 is TYPE_TRIANGLE
  int optval = TYPE_TRIANGLE;
  laser.setlidaropt(LidarPropLidarType, &optval, sizeof(int));
  /// device type
  optval = YDLIDAR_TYPE_SERIAL;
  laser.setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
  /// sample rate (T30 uses 20K sample rate)
  optval = 20;
  laser.setlidaropt(LidarPropSampleRate, &optval, sizeof(int));
  /// abnormal count
  optval = 4;
  laser.setlidaropt(LidarPropAbnormalCheckCount, &optval, sizeof(int));
  /// Intenstiy bit count
  optval = 8;
  laser.setlidaropt(LidarPropIntenstiyBit, &optval, sizeof(int));

  //////////////////////bool property/////////////////
  /// fixed angle resolution
  bool b_optvalue = true;
  laser.setlidaropt(LidarPropFixedResolution, &b_optvalue, sizeof(bool));
  b_optvalue = false;
  /// rotate 180
  laser.setlidaropt(LidarPropReversion, &b_optvalue, sizeof(bool));
  /// Counterclockwise
  laser.setlidaropt(LidarPropInverted, &b_optvalue, sizeof(bool));
  b_optvalue = true;
  laser.setlidaropt(LidarPropAutoReconnect, &b_optvalue, sizeof(bool));
  /// one-way communication (T30 is single channel)
  laser.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));
  /// intensity
  b_optvalue = true;
  laser.setlidaropt(LidarPropIntenstiy, &b_optvalue, sizeof(bool));
  /// Motor DTR (T30 needs DTR control)
  b_optvalue = true;
  laser.setlidaropt(LidarPropSupportMotorDtrCtrl, &b_optvalue, sizeof(bool));
  /// HeartBeat
  b_optvalue = false;
  laser.setlidaropt(LidarPropSupportHeartBeat, &b_optvalue, sizeof(bool));

  //////////////////////float property/////////////////
  /// unit: °
  float f_optvalue = 180.0f;
  laser.setlidaropt(LidarPropMaxAngle, &f_optvalue, sizeof(float));
  f_optvalue = -180.0f;
  laser.setlidaropt(LidarPropMinAngle, &f_optvalue, sizeof(float));
  /// unit: m (T30 range is 0.05~30m)
  f_optvalue = 30.f;
  laser.setlidaropt(LidarPropMaxRange, &f_optvalue, sizeof(float));
  f_optvalue = 0.05f;
  laser.setlidaropt(LidarPropMinRange, &f_optvalue, sizeof(float));
  /// unit: Hz
  laser.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));

  //禁用阳光玻璃过滤
  laser.enableGlassNoise(false);
  laser.enableSunNoise(false);

  //启用调试模式查看详细信息
  laser.setEnableDebug(true);

  info("T30 Configuration:");
  info("  Baudrate: %d", baudrate);
  info("  Single Channel: %s", isSingleChannel ? "true" : "false");
  info("  Scan Frequency: %.1f Hz", frequency);

  bool ret = laser.initialize();
  if (!ret)
  {
    error("Fail to initialize %s", laser.DescribeError());
    return -1;
  }

  //启动扫描
  ret = laser.turnOn();
  if (!ret)
  {
    error("Fail to start %s", laser.DescribeError());
    return -1;
  }

  LaserScan scan;
  while (ydlidar::os_isOk())
  {
    if (laser.doProcessSimple(scan))
    {
      info("[%u] points [%.02f(%.02f)]Hz",
              (unsigned int)scan.points.size(),
              scan.scanFreq,
              1.0 / scan.config.scan_time);
      // Uncomment to see individual points:
      // for (size_t i = 0; i < scan.points.size(); ++i)
      // {
      //   const LaserPoint &p = scan.points.at(i);
      //   info("%d a %.01f r %.04f i %.0f",
      //     i, p.angle * 180.0 / M_PI, p.range, p.intensity);
      // }
    }
    else
    {
        error("Failed to get Lidar Data");
    }
  }

  laser.turnOff();
  laser.disconnecting();

  return 0;
}
