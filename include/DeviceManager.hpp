#pragma once

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
  std::string version;
  int device_model_id;
};

struct ServiceRecord {
  int id;
  std::string description_key;
  int parent_id; // 0 if parent
};

struct FeatureRecord {
  int id;
  std::string description_key;
  int service_firmware_id;
  int parent_feature_id;
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

  // Firmware methods
  int addFirmwareVersion(const std::string &version, int deviceModelId);
  bool updateFirmwareVersion(int id, const std::string &version,
                             int deviceModelId);
  bool deleteFirmwareVersion(int id);
  std::vector<FirmwareVersionRecord>
  getFirmwareVersionsForDeviceModel(int deviceModelId);
  std::optional<FirmwareVersionRecord> getFirmwareVersionById(int id);

  // Service methods
  int addService(const std::string &descKey, int parentId = 0);
  bool updateService(int id, const std::string &descKey, int parentId = 0);
  bool deleteService(int id);
  int linkServiceToFirmware(int serviceId, int firmwareId);
  bool unlinkServiceFromFirmware(int serviceId, int firmwareId);
  int getServiceFirmwareId(int serviceId, int firmwareId);
  std::vector<ServiceRecord> getAllServices();
  std::vector<ServiceRecord> getServicesByParentAndFirmware(int parentId,
                                                            int firmwareId);
  std::optional<ServiceRecord> getServiceById(int id);

  // Feature methods
  int addFeature(const std::string &descKey, int serviceFirmwareId,
                 int parentFeatureId = 0);
  bool updateFeature(int id, const std::string &descKey, int serviceFirmwareId,
                     int parentFeatureId = 0);
  bool deleteFeature(int id);
  std::vector<FeatureRecord>
  getFeaturesByServiceFirmware(int serviceFirmwareId);
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
    std::vector<TranslationRecord> on_translations;
    std::vector<TranslationRecord> off_translations;
    std::vector<FeatureComponentRecord> children;
  };

  struct ServiceLayoutRecord {
    int service_id;
    std::string description_key;
    std::vector<TranslationRecord> translations;
    std::vector<FeatureComponentRecord> features;
    std::vector<ServiceLayoutRecord> children;
  };

  std::optional<ServiceLayoutRecord> getScreenLayout(int serviceFirmwareId);

  int addComponentOption(int featureComponentId, const std::string &value,
                         const std::string &descKey);
  std::vector<ComponentOptionRecord>
  getComponentOptions(int featureComponentId);

  // Population method
  bool populateSampleLayoutData();

private:
  std::string dbPath;
  sqlite3 *db;

  bool runSchema();
  int getCurrentVersion();
};
