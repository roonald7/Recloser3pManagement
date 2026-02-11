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

    auto models = manager->getModelsForDevice(device.id);
    for (const auto &model : models) {
      ModelInfo mi;
      mi.model = model;
      mi.translations = manager->getTranslationsForKey(model.description_key);

      auto fws = manager->getFirmwareVersionsForModel(model.id);
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

  // 1. Get DeviceModelFirmware ID directly using modelId and firmwareId
  int dmfId = manager->getModelFirmwareId(modelId, firmwareId);
  if (dmfId <= 0)
    return result;

  // 2. Get top-level services
  auto topLevelServices =
      manager->getServicesByParentAndModelFirmware(0, dmfId);

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

  node.id = serviceId;
  node.description_key = serviceOpt->description_key;
  node.translations =
      manager->getTranslationsForKey(serviceOpt->description_key);

  // Get the junction ID for features retrieval
  int smfId = manager->getServiceModelFirmwareId(serviceId, dmfId);
  if (smfId > 0) {
    auto features = manager->getFeaturesByServiceModelFirmware(smfId);
    for (const auto &feat : features) {
      FeatureInfo fi;
      fi.record = feat;
      fi.translations = manager->getTranslationsForKey(feat.description_key);
      node.features.push_back(fi);
    }
  }

  auto children =
      manager->getServicesByParentAndModelFirmware(serviceId, dmfId);
  for (const auto &child : children) {
    node.children.push_back(buildServiceNode(manager, child.id, dmfId));
  }

  return node;
}
