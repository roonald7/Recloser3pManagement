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

struct LineInfo {
  LineRecord line;
  std::vector<TranslationRecord> translations;
  std::vector<ModelInfo> models;
};

struct FeatureInfo {
  int id;
  std::string feature_key;
  std::vector<TranslationRecord> translations;
  std::vector<DeviceManager::ComponentOptionRecord> options;
};

struct ServiceNodeInfo {
  int id;
  std::string description_key;
  std::vector<TranslationRecord> translations;
  std::vector<FeatureInfo> features;
  std::vector<ServiceNodeInfo> children;
};

class ServiceHelpers {
public:
  static std::vector<LineInfo> getLineInformation(DeviceManager *manager);

  static std::vector<ServiceNodeInfo> getServiceTree(DeviceManager *manager,
                                                     int lineId, int modelId,
                                                     int firmwareId);

  static std::vector<ServiceNodeInfo> getServiceTree(DeviceManager *manager,
                                                     int dmfId);

  static std::vector<ServiceNodeInfo>
  mergeServiceTrees(const std::vector<ServiceNodeInfo> &tree1,
                    const std::vector<ServiceNodeInfo> &tree2);

private:
  static ServiceNodeInfo buildServiceNode(DeviceManager *manager, int serviceId,
                                          int dmfId);
};
