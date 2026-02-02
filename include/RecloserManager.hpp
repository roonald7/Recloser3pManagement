#pragma once

#include <string>
#include <vector>
#include <memory>
#include "sqlite3.h"

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
    RecloserManager(const std::string& dbPath);
    ~RecloserManager();

    bool initialize();
    
    // Translation methods
    bool addLanguage(const std::string& code, const std::string& name);
    bool addDescriptionKey(const std::string& key);
    bool addTranslation(const std::string& key, const std::string& langCode, const std::string& value);
    std::string getTranslation(const std::string& key, const std::string& langCode);

    // Recloser methods
    bool addRecloser(const std::string& key, const std::string& model);
    std::vector<RecloserRecord> getAllReclosers();

    // Firmware methods
    bool addFirmwareVersion(const std::string& version, int recloserId);
    std::vector<FirmwareVersionRecord> getFirmwareVersionsForRecloser(int recloserId);

    // Service methods
    bool addService(const std::string& serviceKey, const std::string& descKey, int firmwareId, int parentId = 0);
    std::vector<ServiceRecord> getAllServices();
    std::vector<ServiceRecord> getServicesByParentAndFirmware(int parentId, int firmwareId);

    // Feature methods
    bool addFeature(const std::string& descKey, int serviceId);
    std::vector<FeatureRecord> getFeaturesByService(int serviceId);

private:
    std::string dbPath;
    sqlite3* db;

    bool runSchema();
};
