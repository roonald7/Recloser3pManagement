#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <utility>
#include <vector>

struct LanguageRecord {
  std::string code;
  std::string name;
};

struct TranslationRecord {
  std::string description_key;
  std::string language_code;
  std::string value;
};

struct DeviceRecord {
  int id;
  std::string description_key;
};

struct ModelRecord {
  int id;
  std::string description_key;
};

struct DeviceModelRecord {
  int id;
  int device_id;
  int model_id;
};

struct FirmwareVersionRecord {
  int id;
  std::string description_key;
};

struct DeviceModelFirmwareRecord {
  int id;
  int device_model_id;
  int firmware_id;
};

struct ServiceRecord {
  int id;
  std::string description_key;
  int parent_id; // 0 if parent
};

struct FeatureRecord {
  int id;
  std::string description_key;
  int service_device_model_firmware_id;
  int parent_feature_id;
};

struct PhysicalDeviceRecord {
  int64_t id;
  std::string name;
  int device_id;
  int model_id;
  int firmware_version_id;
  std::string identifier;
  std::string description;
  std::string comment;
  bool is_template;
};

struct PhysicalDeviceValueRecord {
  int id;
  int physical_device_id;
  int feature_id;
  std::string value;
};

class DeviceManager {
public:
  DeviceManager(const std::string &dbPath);
  ~DeviceManager();

  bool initialize();
  bool migrate();

  // Translation methods
  bool addLanguage(const std::string &code, const std::string &name);
  bool addDescriptionKey(const std::string &key);
  int addTranslation(const std::string &key, const std::string &langCode,
                     const std::string &value);
  bool addKeyWithTranslations(
      const std::string &key,
      const std::vector<std::pair<std::string, std::string>> &translations);
  std::string getTranslation(const std::string &key,
                             const std::string &langCode);
  std::vector<TranslationRecord> getTranslationsForKey(const std::string &key);
  std::vector<LanguageRecord> getAllLanguages();

  // Device methods
  int addDevice(const std::string &key);
  bool updateDevice(int id, const std::string &key);
  bool deleteDevice(int id);
  std::vector<DeviceRecord> getAllDevices();
  std::optional<DeviceRecord> getDeviceById(int id);

  // Model methods
  int addModel(const std::string &key);
  bool updateModel(int id, const std::string &key);
  bool deleteModel(int id);
  std::vector<ModelRecord> getAllModels();
  std::optional<ModelRecord> getModelById(int id);

  // DeviceModel methods
  int addDeviceModel(int deviceId, int modelId);
  bool deleteDeviceModel(int id);
  std::vector<DeviceModelRecord> getDeviceModelsForDevice(int deviceId);
  std::optional<DeviceModelRecord> getDeviceModelById(int id);
  int getDeviceModelId(int deviceId, int modelId);

  // Firmware methods
  int addFirmwareVersion(const std::string &descKey);
  int linkFirmwareToModel(int firmwareId, int deviceModelId);
  bool deleteFirmwareVersion(int id);
  std::vector<FirmwareVersionRecord>
  getFirmwareVersionsForDeviceModel(int deviceModelId);
  std::optional<FirmwareVersionRecord> getFirmwareVersionById(int id);
  int getDeviceModelFirmwareId(int deviceModelId, int firmwareId);

  // Service methods
  int addService(const std::string &descKey, int parentId = 0);
  bool updateService(int id, const std::string &descKey, int parentId = 0);
  bool deleteService(int id);
  int linkServiceToDeviceModelFirmware(int serviceId,
                                       int deviceModelFirmwareId);
  bool unlinkServiceFromDeviceModelFirmware(int serviceId,
                                            int deviceModelFirmwareId);
  int getServiceDeviceModelFirmwareId(int serviceId, int deviceModelFirmwareId);
  std::vector<ServiceRecord> getAllServices();
  std::vector<ServiceRecord>
  getServicesByParentAndDeviceModelFirmware(int parentId,
                                            int deviceModelFirmwareId);
  std::optional<ServiceRecord> getServiceById(int id);

  // Feature methods
  int addFeature(const std::string &descKey, int serviceDeviceModelFirmwareId,
                 int parentFeatureId = 0);
  bool updateFeature(int id, const std::string &descKey,
                     int serviceDeviceModelFirmwareId, int parentFeatureId = 0);
  bool deleteFeature(int id);
  std::vector<FeatureRecord>
  getFeaturesByServiceDeviceModelFirmware(int serviceDeviceModelFirmwareId);
  std::optional<FeatureRecord> getFeatureById(int id);

  // Component methods
  int linkFeatureToComponent(int featureId, const std::string &componentType);
  int addComponentLimit(int featureComponentId, const std::string &limitKey,
                        const std::string &value);

  struct ComponentLimitRecord {
    std::string key;
    std::string value;
  };

  struct ComponentOptionRecord {
    std::string value;
    std::string description_key;
    std::vector<TranslationRecord> translations;
  };

  struct FeatureComponentRecord {
    int feature_id;
    int feature_component_id; // Added to help linking options
    int parent_feature_id;
    std::string feature_key;
    std::vector<TranslationRecord> translations;
    std::string component_type;
    std::vector<ComponentLimitRecord> limits;
    std::vector<ComponentOptionRecord> options;
    std::vector<FeatureComponentRecord> children;
  };

  struct ServiceLayoutRecord {
    int service_id;
    std::string description_key;
    std::vector<TranslationRecord> translations;
    std::vector<FeatureComponentRecord> features;
    std::vector<ServiceLayoutRecord> children;
  };

  std::optional<ServiceLayoutRecord>
  getScreenLayout(int serviceDeviceModelFirmwareId);

  int addComponentOption(int featureComponentId, const std::string &value,
                         const std::string &descKey);
  std::vector<ComponentOptionRecord>
  getComponentOptions(int featureComponentId);

  // PhysicalDevice methods
  int64_t addPhysicalDevice(const PhysicalDeviceRecord &record);
  bool updatePhysicalDevice(const PhysicalDeviceRecord &record);
  bool deletePhysicalDevice(int64_t id);
  std::vector<PhysicalDeviceRecord> getAllPhysicalDevices();
  std::optional<PhysicalDeviceRecord> getPhysicalDeviceById(int64_t id);

  // PhysicalDeviceValue methods
  bool setPhysicalDeviceValue(int64_t physicalDeviceId, int featureId,
                              const std::string &value);
  std::string getPhysicalDeviceValue(int64_t physicalDeviceId, int featureId);
  std::vector<PhysicalDeviceValueRecord>
  getValuesForPhysicalDevice(int64_t physicalDeviceId);

  // Population method

  bool populateSampleLayoutData();

private:
  std::string dbPath;
  sqlite3 *db;

  bool runSchema();
  int getCurrentVersion();
};
