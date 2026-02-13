#include "ServiceHelpers.hpp"
#include <map>
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
  (void)deviceId; // Suppress unused parameter warning

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
    auto screenFeatures =
        manager->getScreenFeaturesByServiceModelFirmware(smfId);
    for (const auto &sf : screenFeatures) {
      if (sf.feature_id > 0) {
        FeatureInfo fi;
        fi.id = sf.feature_id;
        fi.feature_key = sf.description_key;
        fi.translations = manager->getTranslationsForKey(sf.description_key);
        node.features.push_back(fi);
      }
    }
  }

  auto children =
      manager->getServicesByParentAndModelFirmware(serviceId, dmfId);
  for (const auto &child : children) {
    node.children.push_back(buildServiceNode(manager, child.id, dmfId));
  }

  return node;
}

std::vector<ServiceNodeInfo>
ServiceHelpers::mergeServiceTrees(const std::vector<ServiceNodeInfo> &tree1,
                                  const std::vector<ServiceNodeInfo> &tree2) {
  std::vector<ServiceNodeInfo> merged;
  std::map<std::string, ServiceNodeInfo> combined;

  auto mergeNodes = [&](auto self, ServiceNodeInfo &base,
                        const ServiceNodeInfo &other) -> void {
    // Merge features
    for (const auto &feat : other.features) {
      bool exists = false;
      for (const auto &baseFeat : base.features) {
        if (baseFeat.feature_key == feat.feature_key) {
          exists = true;
          break;
        }
      }
      if (!exists) {
        base.features.push_back(feat);
      }
    }

    // Merge children
    for (const auto &otherChild : other.children) {
      bool childExists = false;
      for (auto &baseChild : base.children) {
        if (baseChild.description_key == otherChild.description_key) {
          self(self, baseChild, otherChild);
          childExists = true;
          break;
        }
      }
      if (!childExists) {
        base.children.push_back(otherChild);
      }
    }
  };

  // Add all from tree1
  for (const auto &node : tree1) {
    combined[node.description_key] = node;
  }

  // Merge tree2 into combined
  for (const auto &node : tree2) {
    if (combined.count(node.description_key)) {
      mergeNodes(mergeNodes, combined[node.description_key], node);
    } else {
      combined[node.description_key] = node;
    }
  }

  // Convert map back to vector
  for (auto const &[key, val] : combined) {
    merged.push_back(val);
  }

  return merged;
}
