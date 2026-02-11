#include "DeviceManager.hpp"
#include "DatabaseSchema.hpp"
#include <iostream>

DeviceManager::DeviceManager(const std::string &dbPath)
    : dbPath(dbPath), db(nullptr) {}

DeviceManager::~DeviceManager() {
  if (db) {
    sqlite3_close(db);
  }
}

bool DeviceManager::initialize() {
  int rc = sqlite3_open(dbPath.c_str(), &db);
  if (rc != SQLITE_OK) {
    std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
    return false;
  }
  // Enable foreign keys
  sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

  int currentVersion = getCurrentVersion();
  if (currentVersion == 0) {
    return runSchema();
  }

  return migrate();
}

bool DeviceManager::migrate() {
  int currentVersion = getCurrentVersion();
  std::cout << "Current database version: " << currentVersion << std::endl;

  for (auto const &[version, queries] : Schema::MIGRATIONS_SQL) {
    if (version > currentVersion) {
      std::cout << "Applying migration to version " << version << "..."
                << std::endl;
      for (const auto &sql : queries) {
        char *zErrMsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg) !=
            SQLITE_OK) {
          std::cerr << "Migration failed at version " << version << ": "
                    << zErrMsg << std::endl;
          sqlite3_free(zErrMsg);
          return false;
        }
      }
      // Update migration table
      std::string updateSql = "INSERT INTO Migrations (version) VALUES (" +
                              std::to_string(version) + ");";
      sqlite3_exec(db, updateSql.c_str(), nullptr, nullptr, nullptr);
    }
  }
  return true;
}

int DeviceManager::getCurrentVersion() {
  const char *sql = "SELECT MAX(version) FROM Migrations;";
  sqlite3_stmt *stmt;
  int version = 0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }
  return version;
}

bool DeviceManager::runSchema() {
  for (const auto &sql : Schema::INITIALIZATION_SQL) {
    char *zErrMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
      std::cerr << "SQL error during schema initialization: " << zErrMsg
                << std::endl;
      sqlite3_free(zErrMsg);
      return false;
    }
  }
  return true;
}

bool DeviceManager::addLanguage(const std::string &code,
                                const std::string &name) {
  const char *sql =
      "INSERT OR IGNORE INTO Languages (code, name) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<LanguageRecord> DeviceManager::getAllLanguages() {
  std::vector<LanguageRecord> languages;
  const char *sql = "SELECT code, name FROM Languages;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      LanguageRecord lang;
      lang.code = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      lang.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      languages.push_back(lang);
    }
    sqlite3_finalize(stmt);
  }
  return languages;
}

bool DeviceManager::addDescriptionKey(const std::string &key) {
  const char *sql = "INSERT OR IGNORE INTO Descriptions (key) VALUES (?);";
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

int DeviceManager::addTranslation(const std::string &key,
                                  const std::string &langCode,
                                  const std::string &value) {
  const char *sql = "INSERT OR REPLACE INTO Translations (description_key, "
                    "language_code, value) VALUES (?, ?, ?);";
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, langCode.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

bool DeviceManager::addKeyWithTranslations(
    const std::string &key,
    const std::vector<std::pair<std::string, std::string>> &translations) {
  if (!addDescriptionKey(key)) {
    return false;
  }

  bool success = true;
  for (const auto &t : translations) {
    if (!addTranslation(key, t.first, t.second)) {
      success = false;
    }
  }
  return success;
}

std::string DeviceManager::getTranslation(const std::string &key,
                                          const std::string &langCode) {
  const char *sql = "SELECT value FROM Translations WHERE description_key = ? "
                    "AND language_code = ?;";
  sqlite3_stmt *stmt;
  std::string result = "";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, langCode.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      result = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

std::vector<TranslationRecord>
DeviceManager::getTranslationsForKey(const std::string &key) {
  std::vector<TranslationRecord> results;
  const char *sql = "SELECT description_key, language_code, value FROM "
                    "Translations WHERE description_key = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      TranslationRecord rec;
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      rec.language_code =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.value = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      results.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return results;
}

int DeviceManager::addDevice(const std::string &key) {
  const char *sql = "INSERT INTO Devices (description_key) VALUES (?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

bool DeviceManager::updateDevice(int id, const std::string &key) {
  const char *sql = "UPDATE Devices SET description_key = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool DeviceManager::deleteDevice(int id) {
  const char *sql = "DELETE FROM Devices WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<DeviceRecord> DeviceManager::getAllDevices() {
  std::vector<DeviceRecord> records;
  const char *sql = "SELECT id, description_key FROM Devices;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      DeviceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<DeviceRecord> DeviceManager::getDeviceById(int id) {
  const char *sql = "SELECT id, description_key FROM Devices WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<DeviceRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      DeviceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

int DeviceManager::addModel(const std::string &key, int deviceId) {
  const char *sql =
      "INSERT INTO Models (description_key, device_id) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, deviceId);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

bool DeviceManager::updateModel(int id, const std::string &key, int deviceId) {
  const char *sql =
      "UPDATE Models SET description_key = ?, device_id = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, deviceId);
  sqlite3_bind_int(stmt, 3, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool DeviceManager::deleteModel(int id) {
  const char *sql = "DELETE FROM Models WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<ModelRecord> DeviceManager::getAllModels() {
  std::vector<ModelRecord> records;
  const char *sql = "SELECT id, description_key, device_id FROM Models;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ModelRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.device_id = sqlite3_column_int(stmt, 2);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::vector<ModelRecord> DeviceManager::getModelsForDevice(int deviceId) {
  std::vector<ModelRecord> records;
  const char *sql =
      "SELECT id, description_key, device_id FROM Models WHERE device_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, deviceId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ModelRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.device_id = sqlite3_column_int(stmt, 2);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<ModelRecord> DeviceManager::getModelById(int id) {
  const char *sql =
      "SELECT id, description_key, device_id FROM Models WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<ModelRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      ModelRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.device_id = sqlite3_column_int(stmt, 2);
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

int DeviceManager::addFirmwareVersion(const std::string &descKey) {
  addDescriptionKey(descKey);
  const char *sql =
      "INSERT INTO FirmwareVersions (description_key) VALUES (?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  } else {
    // If it exists, find the ID
    const char *findSql =
        "SELECT id FROM FirmwareVersions WHERE description_key = ?;";
    if (sqlite3_prepare_v2(db, findSql, -1, &stmt, nullptr) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return id;
      }
      sqlite3_finalize(stmt);
    }
  }
  return 0;
}

int DeviceManager::linkFirmwareToModel(int firmwareId, int modelId) {
  const char *sql = "INSERT OR IGNORE INTO ModelFirmware (model_id, "
                    "firmware_id) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_int(stmt, 1, modelId);
  sqlite3_bind_int(stmt, 2, firmwareId);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  } else {
    // If it exists, find the ID
    const char *findSql = "SELECT id FROM ModelFirmware WHERE model_id = ? AND "
                          "firmware_id = ?;";
    if (sqlite3_prepare_v2(db, findSql, -1, &stmt, nullptr) == SQLITE_OK) {
      sqlite3_bind_int(stmt, 1, modelId);
      sqlite3_bind_int(stmt, 2, firmwareId);
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return id;
      }
      sqlite3_finalize(stmt);
    }
  }
  return 0;
}

bool DeviceManager::deleteFirmwareVersion(int id) {
  const char *sql = "DELETE FROM FirmwareVersions WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<FirmwareVersionRecord>
DeviceManager::getFirmwareVersionsForModel(int modelId) {
  std::vector<FirmwareVersionRecord> records;
  const char *sql = "SELECT f.id, f.description_key FROM FirmwareVersions f "
                    "JOIN ModelFirmware dmf ON f.id = dmf.firmware_id "
                    "WHERE dmf.model_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, modelId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      FirmwareVersionRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<FirmwareVersionRecord>
DeviceManager::getFirmwareVersionById(int id) {
  const char *sql =
      "SELECT id, description_key FROM FirmwareVersions WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<FirmwareVersionRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      FirmwareVersionRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

int DeviceManager::getModelFirmwareId(int modelId, int firmwareId) {
  const char *sql = "SELECT id FROM ModelFirmware WHERE model_id = ? AND "
                    "firmware_id = ?;";
  sqlite3_stmt *stmt = nullptr;
  int id = 0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, modelId);
    sqlite3_bind_int(stmt, 2, firmwareId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      id = sqlite3_column_int(stmt, 0);
    }
  }
  sqlite3_finalize(stmt);
  return id;
}

int DeviceManager::addService(const std::string &descKey, int parentId) {
  const char *sql =
      "INSERT INTO Services (description_key, parent_id) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
  if (parentId > 0) {
    sqlite3_bind_int(stmt, 2, parentId);
  } else {
    sqlite3_bind_null(stmt, 2);
  }

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

bool DeviceManager::updateService(int id, const std::string &descKey,
                                  int parentId) {
  const char *sql = "UPDATE Services SET description_key = ?, "
                    "parent_id = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
  if (parentId > 0) {
    sqlite3_bind_int(stmt, 2, parentId);
  } else {
    sqlite3_bind_null(stmt, 2);
  }
  sqlite3_bind_int(stmt, 3, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

// Junction table methods removed - services now linked via Features table
// directly

bool DeviceManager::deleteService(int id) {
  const char *sql = "DELETE FROM Services WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<ServiceRecord> DeviceManager::getAllServices() {
  std::vector<ServiceRecord> records;
  const char *sql = "SELECT id, description_key, "
                    "IFNULL(parent_id, 0) FROM Services;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ServiceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.parent_id = sqlite3_column_int(stmt, 2);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::vector<ServiceRecord>
DeviceManager::getServicesByParentAndModelFirmware(int parentId,
                                                   int modelFirmwareId) {
  std::vector<ServiceRecord> records;
  const char *sql;
  if (parentId > 0) {
    sql = "SELECT DISTINCT s.id, s.description_key, s.parent_id "
          "FROM Services s "
          "JOIN Features f ON s.id = f.service_id "
          "WHERE s.parent_id = ? AND f.model_firmware_id = ?;";
  } else {
    sql = "SELECT DISTINCT s.id, s.description_key, s.parent_id "
          "FROM Services s "
          "JOIN Features f ON s.id = f.service_id "
          "WHERE s.parent_id IS NULL AND f.model_firmware_id = ?;";
  }

  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    if (parentId > 0) {
      sqlite3_bind_int(stmt, 1, parentId);
      sqlite3_bind_int(stmt, 2, modelFirmwareId);
    } else {
      sqlite3_bind_int(stmt, 1, modelFirmwareId);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ServiceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.parent_id = sqlite3_column_int(stmt, 2);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<ServiceRecord> DeviceManager::getServiceById(int id) {
  const char *sql = "SELECT id, description_key, IFNULL(parent_id, "
                    "0) FROM Services WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<ServiceRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      ServiceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.parent_id = sqlite3_column_int(stmt, 2);
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

int DeviceManager::addFeature(const std::string &descKey, int serviceId,
                              int modelFirmwareId, int parentFeatureId) {
  const char *sql = "INSERT INTO Features (description_key, "
                    "service_id, model_firmware_id, "
                    "parent_feature_id) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, serviceId);
  sqlite3_bind_int(stmt, 3, modelFirmwareId);
  if (parentFeatureId > 0)
    sqlite3_bind_int(stmt, 4, parentFeatureId);
  else
    sqlite3_bind_null(stmt, 4);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

bool DeviceManager::updateFeature(int id, const std::string &descKey,
                                  int serviceId, int modelFirmwareId,
                                  int parentFeatureId) {
  const char *sql = "UPDATE Features SET description_key = ?, "
                    "service_id = ?, model_firmware_id = ?, parent_feature_id "
                    "= ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, serviceId);
  sqlite3_bind_int(stmt, 3, modelFirmwareId);
  if (parentFeatureId > 0)
    sqlite3_bind_int(stmt, 4, parentFeatureId);
  else
    sqlite3_bind_null(stmt, 4);
  sqlite3_bind_int(stmt, 5, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool DeviceManager::deleteFeature(int id) {
  const char *sql = "DELETE FROM Features WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<FeatureRecord>
DeviceManager::getFeaturesByServiceAndModelFirmware(int serviceId,
                                                    int modelFirmwareId) {
  std::vector<FeatureRecord> records;
  const char *sql =
      "SELECT id, description_key, service_id, model_firmware_id, "
      "IFNULL(parent_feature_id, 0) FROM "
      "Features WHERE service_id = ? AND model_firmware_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, serviceId);
    sqlite3_bind_int(stmt, 2, modelFirmwareId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      FeatureRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.service_id = sqlite3_column_int(stmt, 2);
      rec.model_firmware_id = sqlite3_column_int(stmt, 3);
      rec.parent_feature_id = sqlite3_column_int(stmt, 4);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

int DeviceManager::linkFeatureToComponent(int featureId,
                                          const std::string &componentType) {
  const char *sql = "INSERT INTO FeatureComponent (feature_id, component_id) "
                    "SELECT ?, id FROM Component WHERE type = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_int(stmt, 1, featureId);
  sqlite3_bind_text(stmt, 2, componentType.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

int DeviceManager::addComponentLimit(int featureComponentId,
                                     const std::string &limitKey,
                                     const std::string &value) {
  const char *sql =
      "INSERT INTO FeatureComponentLimits (feature_component_id, limit_id, "
      "value) SELECT ?, id, ? FROM Limits WHERE key = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_int(stmt, 1, featureComponentId);
  sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, limitKey.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc == SQLITE_DONE) {
    return static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  return 0;
}

std::optional<FeatureRecord> DeviceManager::getFeatureById(int id) {
  const char *sql =
      "SELECT id, description_key, service_id, model_firmware_id, "
      "IFNULL(parent_feature_id, 0) FROM Features WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<FeatureRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      FeatureRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.service_id = sqlite3_column_int(stmt, 2);
      rec.model_firmware_id = sqlite3_column_int(stmt, 3);
      rec.parent_feature_id = sqlite3_column_int(stmt, 4);
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

std::optional<DeviceManager::ServiceLayoutRecord>
DeviceManager::getScreenLayout(int serviceId, int modelFirmwareId) {
  // Get service details directly
  const char *serviceSql = "SELECT id, description_key "
                           "FROM Services "
                           "WHERE id = ?;";

  sqlite3_stmt *serviceStmt;
  ServiceLayoutRecord layout;
  bool found = false;

  if (sqlite3_prepare_v2(db, serviceSql, -1, &serviceStmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(serviceStmt, 1, serviceId);
    if (sqlite3_step(serviceStmt) == SQLITE_ROW) {
      layout.service_id = sqlite3_column_int(serviceStmt, 0);
      layout.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(serviceStmt, 1));
      found = true;
    }
  }
  sqlite3_finalize(serviceStmt);

  if (!found)
    return std::nullopt;

  layout.translations = getTranslationsForKey(layout.description_key);

  // Get features for this service-firmware combination
  std::map<int, FeatureComponentRecord> featureMap;

  // Get ALL features for this service-firmware combination first
  const char *layoutSql =
      "SELECT f.id, f.description_key, c.type, fc.id, "
      "IFNULL(f.parent_feature_id, 0) "
      "FROM Features f "
      "LEFT JOIN FeatureComponent fc ON f.id = fc.feature_id "
      "LEFT JOIN Component c ON fc.component_id = c.id "
      "WHERE f.service_id = ? AND f.model_firmware_id = ?;";

  sqlite3_stmt *layoutStmt;
  if (sqlite3_prepare_v2(db, layoutSql, -1, &layoutStmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(layoutStmt, 1, serviceId);
    sqlite3_bind_int(layoutStmt, 2, modelFirmwareId);

    while (sqlite3_step(layoutStmt) == SQLITE_ROW) {
      FeatureComponentRecord rec;
      rec.feature_id = sqlite3_column_int(layoutStmt, 0);
      rec.feature_key =
          reinterpret_cast<const char *>(sqlite3_column_text(layoutStmt, 1));
      rec.parent_feature_id = sqlite3_column_int(layoutStmt, 4);

      rec.translations = getTranslationsForKey(rec.feature_key);

      if (sqlite3_column_type(layoutStmt, 2) != SQLITE_NULL) {
        rec.component_type =
            reinterpret_cast<const char *>(sqlite3_column_text(layoutStmt, 2));

        int fcId = sqlite3_column_int(layoutStmt, 3);
        rec.feature_component_id = fcId;

        // Get limits for this component
        const char *limitSql = "SELECT l.key, fcl.value "
                               "FROM FeatureComponentLimits fcl "
                               "JOIN Limits l ON fcl.limit_id = l.id "
                               "WHERE fcl.feature_component_id = ?;";

        sqlite3_stmt *limitStmt;
        if (sqlite3_prepare_v2(db, limitSql, -1, &limitStmt, nullptr) ==
            SQLITE_OK) {
          sqlite3_bind_int(limitStmt, 1, fcId);
          while (sqlite3_step(limitStmt) == SQLITE_ROW) {
            ComponentLimitRecord lim;
            lim.key = reinterpret_cast<const char *>(
                sqlite3_column_text(limitStmt, 0));
            lim.value = reinterpret_cast<const char *>(
                sqlite3_column_text(limitStmt, 1));

            // Store ON/OFF labels as ComponentOptions instead of limits
            if (lim.key == "ON_LABEL") {
              ComponentOptionRecord onOpt;
              onOpt.value = "ON";
              onOpt.description_key = lim.value;
              onOpt.translations = getTranslationsForKey(lim.value);
              rec.options.push_back(onOpt);
            } else if (lim.key == "OFF_LABEL") {
              ComponentOptionRecord offOpt;
              offOpt.value = "OFF";
              offOpt.description_key = lim.value;
              offOpt.translations = getTranslationsForKey(lim.value);
              rec.options.push_back(offOpt);
            } else {
              // Regular limits (MIN_VALUE, MAX_VALUE, etc.)
              rec.limits.push_back(lim);
            }
          }
        }
        sqlite3_finalize(limitStmt);

        // Get regular options for this component (for COMBOBOX, etc.)
        auto comboOptions = getComponentOptions(fcId);
        rec.options.insert(rec.options.end(), comboOptions.begin(),
                           comboOptions.end());
      }
      featureMap[rec.feature_id] = rec;
    }
  }
  sqlite3_finalize(layoutStmt);

  // Note: The loop above adds ALL children to parents, but we only want to push
  // ROOTS to layout.features. However, the issue is that children are ALREADY
  // inside the map. When we push `rec` to children, we are pushing a COPY. We
  // need to pointer or second pass. Better approach:
  // 1. Populate Map.
  // 2. Iterate Map. If parent_id > 0, add to parent's children list.
  // 3. Iterate Map AGAIN. If parent_id == 0, add to layout.features.

  // Reset and redo correctly:
  layout.features.clear();

  // Link children
  for (auto &[id, rec] : featureMap) {
    if (rec.parent_feature_id > 0 && featureMap.count(rec.parent_feature_id)) {
      featureMap[rec.parent_feature_id].children.push_back(rec);
    }
  }

  // Add roots to final layout
  for (const auto &[id, rec] : featureMap) {
    if (rec.parent_feature_id == 0) {
      layout.features.push_back(rec);
    }
  }

  // Recursively get children layouts
  const char *childrenSql = "SELECT id FROM Services WHERE parent_id = ?;";
  sqlite3_stmt *childrenStmt;
  if (sqlite3_prepare_v2(db, childrenSql, -1, &childrenStmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(childrenStmt, 1, layout.service_id);
    while (sqlite3_step(childrenStmt) == SQLITE_ROW) {
      int childServiceId = sqlite3_column_int(childrenStmt, 0);
      auto childLayout = getScreenLayout(childServiceId, modelFirmwareId);
      if (childLayout) {
        layout.children.push_back(*childLayout);
      }
    }
    sqlite3_finalize(childrenStmt);
  }

  return layout;
}

bool DeviceManager::populateSampleLayoutData() {
  // Overcurrent Protection (feature_id=1) -> Integer
  int fc1 = linkFeatureToComponent(1, "Integer");
  if (fc1 > 0) {
    addComponentLimit(fc1, "MIN_VALUE", "0");
    addComponentLimit(fc1, "MAX_VALUE", "5000");
    addComponentLimit(fc1, "STEP", "1");
  }

  // Reclose Count Limit (feature_id=2) -> ComboBox
  int fc2 = linkFeatureToComponent(2, "ComboBox");
  if (fc2 > 0) {
    addComponentLimit(fc2, "MAX_CHAR", "2");
  }

  return true;
}

int DeviceManager::addComponentOption(int featureComponentId,
                                      const std::string &value,
                                      const std::string &descKey) {
  addDescriptionKey(descKey);
  const char *sql =
      "INSERT INTO ComponentOptions (feature_component_id, value, "
      "description_key) VALUES (?, ?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_int(stmt, 1, featureComponentId);
  sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, descKey.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  int id = 0;
  if (rc == SQLITE_DONE) {
    id = static_cast<int>(sqlite3_last_insert_rowid(db));
  }
  sqlite3_finalize(stmt);
  return id;
}

std::vector<DeviceManager::ComponentOptionRecord>
DeviceManager::getComponentOptions(int featureComponentId) {
  std::vector<ComponentOptionRecord> options;
  const char *sql = "SELECT value, description_key FROM ComponentOptions WHERE "
                    "feature_component_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, featureComponentId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ComponentOptionRecord opt;
      opt.value = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      opt.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      opt.translations = getTranslationsForKey(opt.description_key);
      options.push_back(opt);
    }
    sqlite3_finalize(stmt);
  }
  return options;
}

int64_t DeviceManager::addPhysicalDevice(const PhysicalDeviceRecord &record) {
  const char *sql = "INSERT INTO PhysicalDevices (name, device_id, model_id, "
                    "firmware_version_id, identifier, description, comment, "
                    "is_template) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_text(stmt, 1, record.name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, record.device_id);
  sqlite3_bind_int(stmt, 3, record.model_id);
  sqlite3_bind_int(stmt, 4, record.firmware_version_id);
  sqlite3_bind_text(stmt, 5, record.identifier.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 6, record.description.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 7, record.comment.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 8, record.is_template ? 1 : 0);

  int rc = sqlite3_step(stmt);
  int64_t id = 0;
  if (rc == SQLITE_DONE) {
    id = sqlite3_last_insert_rowid(db);
  }
  sqlite3_finalize(stmt);
  return id;
}

bool DeviceManager::updatePhysicalDevice(const PhysicalDeviceRecord &record) {
  const char *sql =
      "UPDATE PhysicalDevices SET name = ?, device_id = ?, "
      "model_id = ?, firmware_version_id = ?, identifier = ?, "
      "description = ?, comment = ?, is_template = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, record.name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, record.device_id);
  sqlite3_bind_int(stmt, 3, record.model_id);
  sqlite3_bind_int(stmt, 4, record.firmware_version_id);
  sqlite3_bind_text(stmt, 5, record.identifier.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 6, record.description.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 7, record.comment.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 8, record.is_template ? 1 : 0);
  sqlite3_bind_int64(stmt, 9, record.id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool DeviceManager::deletePhysicalDevice(int64_t id) {
  const char *sql = "DELETE FROM PhysicalDevices WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int64(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<PhysicalDeviceRecord> DeviceManager::getAllPhysicalDevices() {
  std::vector<PhysicalDeviceRecord> devices;
  const char *sql =
      "SELECT id, name, device_id, model_id, firmware_version_id, "
      "identifier, description, comment, is_template FROM "
      "PhysicalDevices;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      PhysicalDeviceRecord rec;
      rec.id = sqlite3_column_int64(stmt, 0);
      rec.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.device_id = sqlite3_column_int(stmt, 2);
      rec.model_id = sqlite3_column_int(stmt, 3);
      rec.firmware_version_id = sqlite3_column_int(stmt, 4);
      rec.identifier =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
      rec.description =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
      rec.comment =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
      rec.is_template = sqlite3_column_int(stmt, 8) != 0;
      devices.push_back(rec);
    }
    sqlite3_finalize(stmt);
  }
  return devices;
}

std::optional<PhysicalDeviceRecord>
DeviceManager::getPhysicalDeviceById(int64_t id) {
  const char *sql =
      "SELECT id, name, device_id, model_id, firmware_version_id, "
      "identifier, description, comment, is_template FROM "
      "PhysicalDevices WHERE id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      PhysicalDeviceRecord rec;
      rec.id = sqlite3_column_int64(stmt, 0);
      rec.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.device_id = sqlite3_column_int(stmt, 2);
      rec.model_id = sqlite3_column_int(stmt, 3);
      rec.firmware_version_id = sqlite3_column_int(stmt, 4);
      rec.identifier =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
      rec.description =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
      rec.comment =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
      rec.is_template = sqlite3_column_int(stmt, 8) != 0;
      sqlite3_finalize(stmt);
      return rec;
    }
    sqlite3_finalize(stmt);
  }
  return std::nullopt;
}

bool DeviceManager::setPhysicalDeviceValue(int64_t physicalDeviceId,
                                           int featureId,
                                           const std::string &value) {
  const char *sql = "INSERT INTO PhysicalDeviceValues (physical_device_id, "
                    "feature_id, value) VALUES (?, ?, ?) "
                    "ON CONFLICT(physical_device_id, feature_id) DO UPDATE SET "
                    "value = excluded.value;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int64(stmt, 1, physicalDeviceId);
  sqlite3_bind_int(stmt, 2, featureId);
  sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::string DeviceManager::getPhysicalDeviceValue(int64_t physicalDeviceId,
                                                  int featureId) {
  const char *sql = "SELECT value FROM PhysicalDeviceValues WHERE "
                    "physical_device_id = ? AND feature_id = ?;";
  sqlite3_stmt *stmt;
  std::string value = "";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int64(stmt, 1, physicalDeviceId);
    sqlite3_bind_int(stmt, 2, featureId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *val =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      if (val)
        value = val;
    }
    sqlite3_finalize(stmt);
  }
  return value;
}

std::vector<PhysicalDeviceValueRecord>
DeviceManager::getValuesForPhysicalDevice(int64_t physicalDeviceId) {
  std::vector<PhysicalDeviceValueRecord> values;
  const char *sql = "SELECT id, physical_device_id, feature_id, value FROM "
                    "PhysicalDeviceValues WHERE physical_device_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int64(stmt, 1, physicalDeviceId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      PhysicalDeviceValueRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.physical_device_id = static_cast<int>(sqlite3_column_int64(stmt, 1));
      rec.feature_id = sqlite3_column_int(stmt, 2);
      const char *val =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
      if (val)
        rec.value = val;
      values.push_back(rec);
    }
    sqlite3_finalize(stmt);
  }
  return values;
}
