#include "ServiceHelpers.hpp"
#include <vector>

std::vector<DeviceInfo>
ServiceHelpers::getDeviceInformation(DeviceManager *manager) {
  std::vector<DeviceInfo> result;

  auto devices = manager->getAllDevices();
  for (const auto &device : devices) {
    DeviceInfo di;
    di.device = device;
    di.translations = manager->getTranslationsForKey(device.description_key);

    auto deviceModels = manager->getDeviceModelsForDevice(device.id);
    for (const auto &dm : deviceModels) {
      auto modelOpt = manager->getModelById(dm.model_id);
      if (!modelOpt)
        continue;

      ModelInfo mi;
      mi.model = *modelOpt;
      mi.translations =
          manager->getTranslationsForKey(modelOpt->description_key);
      auto fws = manager->getFirmwareVersionsForDeviceModel(dm.id);
      for (const auto &fw : fws) {
        FirmwareInfo fi;
        fi.firmware = fw;
        fi.translations = manager->getTranslationsForKey(fw.description_key);
        mi.firmwares.push_back(fi);
      }

      di.models.push_back(mi);
    }

    result.push_back(di);
  }

  return result;
}

std::vector<ServiceNodeInfo>
ServiceHelpers::getServiceTree(DeviceManager *manager, int deviceId,
                               int modelId, int firmwareId) {
  std::vector<ServiceNodeInfo> result;

  // 1. Get DeviceModel ID
  int deviceModelId = manager->getDeviceModelId(deviceId, modelId);
  if (deviceModelId <= 0)
    return result;

  // 2. Get DeviceModelFirmware ID
  int dmfId = manager->getDeviceModelFirmwareId(deviceModelId, firmwareId);
  if (dmfId <= 0)
    return result;

  // 3. Get top-level services
  auto topLevelServices =
      manager->getServicesByParentAndDeviceModelFirmware(0, dmfId);

  for (const auto &service : topLevelServices) {
    result.push_back(buildServiceNode(manager, service.id, dmfId));
  }

  return result;
}

ServiceNodeInfo ServiceHelpers::buildServiceNode(DeviceManager *manager,
                                                 int serviceId, int dmfId) {
  ServiceNodeInfo node;
  auto serviceOpt = manager->getServiceById(serviceId);
  if (!serviceOpt)
    return node;

  int sdfId = manager->getServiceDeviceModelFirmwareId(serviceId, dmfId);
  node.id = sdfId;
  node.description_key = serviceOpt->description_key;
  node.translations =
      manager->getTranslationsForKey(serviceOpt->description_key);

  if (sdfId > 0) {
    auto features = manager->getFeaturesByServiceDeviceModelFirmware(sdfId);
    for (const auto &feat : features) {
      FeatureInfo fi;
      fi.record = feat;
      fi.translations = manager->getTranslationsForKey(feat.description_key);
      node.features.push_back(fi);
    }
  }

  auto children =
      manager->getServicesByParentAndDeviceModelFirmware(serviceId, dmfId);
  for (const auto &child : children) {
    node.children.push_back(buildServiceNode(manager, child.id, dmfId));
  }

  return node;
}
