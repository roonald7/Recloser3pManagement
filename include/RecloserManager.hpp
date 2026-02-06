#pragma once

#include "sqlite3.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct TranslationRecord {
  std::string description_key;
  std::string language_code;
  std::string value;
};

struct RecloserRecord {
  int id;
  std::string description_key;
};

struct FirmwareVersionRecord {
  int id;
  std::string version;
  int recloser_id;
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
};

class RecloserManager {
public:
  RecloserManager(const std::string &dbPath);
  ~RecloserManager();

  bool initialize();
  bool migrate();

  // Translation methods
  bool addLanguage(const std::string &code, const std::string &name);
  bool addDescriptionKey(const std::string &key);
  bool addTranslation(const std::string &key, const std::string &langCode,
                      const std::string &value);
  bool addKeyWithTranslations(
      const std::string &key,
      const std::vector<std::pair<std::string, std::string>> &translations);
  std::string getTranslation(const std::string &key,
                             const std::string &langCode);
  std::vector<TranslationRecord> getTranslationsForKey(const std::string &key);

  // Recloser methods
  bool addRecloser(const std::string &key);
  bool updateRecloser(int id, const std::string &key);
  bool deleteRecloser(int id);
  std::vector<RecloserRecord> getAllReclosers();
  std::optional<RecloserRecord> getRecloserById(int id);

  // Firmware methods
  bool addFirmwareVersion(const std::string &version, int recloserId);
  bool updateFirmwareVersion(int id, const std::string &version,
                             int recloserId);
  bool deleteFirmwareVersion(int id);
  std::vector<FirmwareVersionRecord>
  getFirmwareVersionsForRecloser(int recloserId);
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
  int addFeature(const std::string &descKey, int serviceFirmwareId);
  bool updateFeature(int id, const std::string &descKey, int serviceFirmwareId);
  bool deleteFeature(int id);
  std::vector<FeatureRecord>
  getFeaturesByServiceFirmware(int serviceFirmwareId);
  std::optional<FeatureRecord> getFeatureById(int id);

  // Component methods
  int linkFeatureToComponent(int featureId, const std::string &componentType);
  bool addComponentLimit(int featureComponentId, const std::string &limitKey,
                         const std::string &value);

  struct ComponentLimitRecord {
    std::string key;
    std::string value;
  };

  struct FeatureComponentRecord {
    int feature_id;
    std::string feature_key;
    std::vector<TranslationRecord> translations;
    std::string component_type;
    std::vector<ComponentLimitRecord> limits;
  };

  struct ServiceLayoutRecord {
    int service_id;
    std::string description_key;
    std::vector<TranslationRecord> translations;
    std::vector<FeatureComponentRecord> features;
    std::vector<ServiceLayoutRecord> children;
  };

  std::optional<ServiceLayoutRecord> getScreenLayout(int serviceFirmwareId);

  // Population method
  bool populateSampleLayoutData();

private:
  std::string dbPath;
  sqlite3 *db;

  bool runSchema();
  int getCurrentVersion();
};
