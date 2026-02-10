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
