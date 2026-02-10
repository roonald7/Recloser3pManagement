#pragma once
#include "DeviceManager.hpp"
#include <vector>

struct FirmwareInfo {
  FirmwareVersionRecord firmware;
  std::vector<TranslationRecord> translations;
};

struct ModelInfo {
  ModelRecord model;
  std::vector<TranslationRecord> translations;
  std::vector<FirmwareInfo> firmwares;
};

struct DeviceInfo {
  DeviceRecord device;
  std::vector<TranslationRecord> translations;
  std::vector<ModelInfo> models;
};

class ServiceHelpers {
public:
  static std::vector<DeviceInfo> getDeviceInformation(DeviceManager *manager);
};
