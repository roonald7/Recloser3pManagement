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

struct LineRecord {
  int id;
  std::string description_key;
};

struct ModelRecord {
  int id;
  std::string description_key;
  int line_id;
};

struct FirmwareVersionRecord {
  int id;
  std::string description_key;
};

struct ModelFirmwareRecord {
  int id;
  int model_id;
  int firmware_id;
};

struct ServiceRecord {
  int id;
  std::string description_key;
  int parent_id; // 0 if parent
};

struct FeatureRecord {
  int id;
  std::string key;
};

struct ScreenFeatureRecord {
  int id;
  int service_model_firmware_id;
  int feature_id;
  std::string description_key;
  int parent_screen_feature_id;
};

struct Device {
  int64_t id;
  std::string name;
  int model_firmware_id;
  std::string identifier;
  std::string description;
  std::string comment;
  bool is_template;
};

struct DeviceRecord {
  int64_t id;
  int64_t device_id;
  std::string created_at;
};

struct DeviceValueRecord {
  int id;
  int64_t device_record_id;
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

  // Line methods
  int addLine(const std::string &key);
  bool updateLine(int id, const std::string &key);
  bool deleteLine(int id);
  std::vector<LineRecord> getAllLines();
  std::optional<LineRecord> getLineById(int id);

  // Model methods
  int addModel(const std::string &key, int lineId);
  bool updateModel(int id, const std::string &key, int lineId);
  bool deleteModel(int id);
  std::vector<ModelRecord> getAllModels();
  std::vector<ModelRecord> getModelsForLine(int lineId);
  std::optional<ModelRecord> getModelById(int id);

  // Firmware methods
  int addFirmwareVersion(const std::string &descKey);
  int linkFirmwareToModel(int firmwareId, int modelId);
  bool deleteFirmwareVersion(int id);
  std::vector<FirmwareVersionRecord> getFirmwareVersionsForModel(int modelId);
  std::optional<FirmwareVersionRecord> getFirmwareVersionById(int id);
  int getModelFirmwareId(int modelId, int firmwareId);
  std::optional<ModelFirmwareRecord> getModelFirmwareById(int id);

  // Service methods
  int addService(const std::string &descKey, int parentId = 0);
  bool updateService(int id, const std::string &descKey, int parentId = 0);
  bool deleteService(int id);
  int linkServiceToModelFirmware(int serviceId, int modelFirmwareId);
  bool unlinkServiceFromModelFirmware(int serviceId, int modelFirmwareId);
  int getServiceModelFirmwareId(int serviceId, int modelFirmwareId);
  std::vector<ServiceRecord> getAllServices();
  std::vector<ServiceRecord>
  getServicesByParentAndModelFirmware(int parentId, int modelFirmwareId);
  std::optional<ServiceRecord> getServiceById(int id);

  // Feature methods
  int addFeature(const std::string &key);
  bool updateFeature(int id, const std::string &key);
  bool deleteFeature(int id);
  std::vector<FeatureRecord> getAllFeatures();
  std::optional<FeatureRecord> getFeatureById(int id);

  // Screen Feature methods
  int addScreenFeature(int serviceModelFirmwareId, int featureId,
                       const std::string &descKey,
                       int parentScreenFeatureId = 0);
  bool updateScreenFeature(int id, int serviceModelFirmwareId, int featureId,
                           const std::string &descKey,
                           int parentScreenFeatureId = 0);
  bool deleteScreenFeature(int id);
  std::vector<ScreenFeatureRecord>
  getScreenFeaturesByServiceModelFirmware(int serviceModelFirmwareId);
  std::optional<ScreenFeatureRecord> getScreenFeatureById(int id);

  // Component methods
  int linkScreenFeatureToComponent(int screenFeatureId,
                                   const std::string &componentType);
  int addComponentLimit(int featureComponentId, const std::string &limitKey,
                        const std::string &value);
  int getFeatureComponentId(int screenFeatureId);

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
  getScreenLayout(int serviceModelFirmwareId);

  int addComponentOption(int featureComponentId, const std::string &value,
                         const std::string &descKey);
  std::vector<ComponentOptionRecord>
  getComponentOptions(int featureComponentId);

  // Device methods
  int64_t addDevice(const Device &record);
  bool updateDevice(const Device &record);
  bool deleteDevice(int64_t id);
  std::vector<Device> getAllDevices();
  std::vector<Device> getDevicesByLine(int lineId);
  std::optional<Device> getDeviceById(int64_t id);

  // DeviceRecord methods
  int64_t addDeviceRecord(int64_t deviceId);
  bool deleteDeviceRecord(int64_t id);
  std::vector<DeviceRecord> getRecordsByDevice(int64_t deviceId);
  std::optional<DeviceRecord> getDeviceRecordById(int64_t id);

  // DeviceValue methods
  bool setRecordValue(int64_t device_record_id, int feature_id,
                      const std::string &value);
  std::string getRecordValue(int64_t device_record_id, int feature_id);
  std::vector<DeviceValueRecord> getValuesForRecord(int64_t device_record_id);

  // Legacy/Helper (might need update to uses latest record)
  bool setDeviceValue(int64_t deviceId, int featureId,
                      const std::string &value);
  std::vector<DeviceValueRecord> getValuesForDevice(int64_t deviceId);

  // Population method

private:
  std::string dbPath;
  sqlite3 *db;

  bool runSchema();
  int getCurrentVersion();
};
