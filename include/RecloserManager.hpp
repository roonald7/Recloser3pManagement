#pragma once

#include "sqlite3.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct TranslationRecord {
  std::string description_key;
  std::string language_code;
  std::string value;
};

struct RecloserRecord {
  int id;
  std::string description_key;
  std::string model;
};

struct FirmwareVersionRecord {
  int id;
  std::string version;
  int recloser_id;
};

struct ServiceRecord {
  int id;
  std::string service_key;
  std::string description_key;
  int parent_id; // 0 if parent
  int firmware_id;
};

struct FeatureRecord {
  int id;
  std::string description_key;
  int service_id;
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
  std::string getTranslation(const std::string &key,
                             const std::string &langCode);
  std::vector<TranslationRecord> getTranslationsForKey(const std::string &key);

  // Recloser methods
  bool addRecloser(const std::string &key, const std::string &model);
  bool updateRecloser(int id, const std::string &key, const std::string &model);
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
  bool addService(const std::string &serviceKey, const std::string &descKey,
                  int firmwareId, int parentId = 0);
  bool updateService(int id, const std::string &serviceKey,
                     const std::string &descKey, int firmwareId,
                     int parentId = 0);
  bool deleteService(int id);
  std::vector<ServiceRecord> getAllServices();
  std::vector<ServiceRecord> getServicesByParentAndFirmware(int parentId,
                                                            int firmwareId);
  std::optional<ServiceRecord> getServiceById(int id);

  // Feature methods
  bool addFeature(const std::string &descKey, int serviceId);
  bool updateFeature(int id, const std::string &descKey, int serviceId);
  bool deleteFeature(int id);
  std::vector<FeatureRecord> getFeaturesByService(int serviceId);
  std::optional<FeatureRecord> getFeatureById(int id);

  // Layout methods
  struct LayoutLimitRecord {
    std::string key;
    std::string value;
  };

  struct FeatureLayoutRecord {
    int feature_id;
    std::string feature_key;
    std::vector<TranslationRecord> translations;
    std::string component_type;
    std::string component_key;
    std::vector<LayoutLimitRecord> limits;
  };

  struct ServiceLayoutRecord {
    int service_id;
    std::string service_key;
    std::string description_key;
    std::vector<TranslationRecord> translations;
    std::vector<FeatureLayoutRecord> features;
    std::vector<ServiceLayoutRecord> children;
  };

  std::optional<ServiceLayoutRecord> getScreenLayout(int serviceId);

  // Population method
  bool populateSampleLayoutData();

private:
  std::string dbPath;
  sqlite3 *db;

  bool runSchema();
  int getCurrentVersion();
};
