#include "RecloserManager.hpp"
#include "DatabaseSchema.hpp"
#include <iostream>

RecloserManager::RecloserManager(const std::string &dbPath)
    : dbPath(dbPath), db(nullptr) {}

RecloserManager::~RecloserManager() {
  if (db) {
    sqlite3_close(db);
  }
}

bool RecloserManager::initialize() {
  int rc = sqlite3_open(dbPath.c_str(), &db);
  if (rc != SQLITE_OK) {
    std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
    return false;
  }
  // Enable foreign keys
  sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

  return runSchema();
}

bool RecloserManager::migrate() {
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

int RecloserManager::getCurrentVersion() {
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

bool RecloserManager::runSchema() {
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
  return migrate();
}

bool RecloserManager::addLanguage(const std::string &code,
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

bool RecloserManager::addDescriptionKey(const std::string &key) {
  const char *sql = "INSERT OR IGNORE INTO Descriptions (key) VALUES (?);";
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::addTranslation(const std::string &key,
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
  return rc == SQLITE_DONE;
}

std::string RecloserManager::getTranslation(const std::string &key,
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
RecloserManager::getTranslationsForKey(const std::string &key) {
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

bool RecloserManager::addRecloser(const std::string &key,
                                  const std::string &model) {
  const char *sql =
      "INSERT INTO Reclosers (description_key, model) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, model.c_str(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::updateRecloser(int id, const std::string &key,
                                     const std::string &model) {
  const char *sql =
      "UPDATE Reclosers SET description_key = ?, model = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, model.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::deleteRecloser(int id) {
  const char *sql = "DELETE FROM Reclosers WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<RecloserRecord> RecloserManager::getAllReclosers() {
  std::vector<RecloserRecord> records;
  const char *sql = "SELECT id, description_key, model FROM Reclosers;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      RecloserRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.model = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<RecloserRecord> RecloserManager::getRecloserById(int id) {
  const char *sql =
      "SELECT id, description_key, model FROM Reclosers WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<RecloserRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      RecloserRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.model = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

bool RecloserManager::addFirmwareVersion(const std::string &version,
                                         int recloserId) {
  const char *sql =
      "INSERT INTO FirmwareVersions (version, recloser_id) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, version.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, recloserId);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::updateFirmwareVersion(int id, const std::string &version,
                                            int recloserId) {
  const char *sql =
      "UPDATE FirmwareVersions SET version = ?, recloser_id = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, version.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, recloserId);
  sqlite3_bind_int(stmt, 3, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::deleteFirmwareVersion(int id) {
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
RecloserManager::getFirmwareVersionsForRecloser(int recloserId) {
  std::vector<FirmwareVersionRecord> records;
  const char *sql = "SELECT id, version, recloser_id FROM FirmwareVersions "
                    "WHERE recloser_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, recloserId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      FirmwareVersionRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.version =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.recloser_id = sqlite3_column_int(stmt, 2);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<FirmwareVersionRecord>
RecloserManager::getFirmwareVersionById(int id) {
  const char *sql =
      "SELECT id, version, recloser_id FROM FirmwareVersions WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<FirmwareVersionRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      FirmwareVersionRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.version =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.recloser_id = sqlite3_column_int(stmt, 2);
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

bool RecloserManager::addService(const std::string &serviceKey,
                                 const std::string &descKey, int firmwareId,
                                 int parentId) {
  const char *sql = "INSERT INTO Services (service_key, description_key, "
                    "parent_id, firmware_id) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, serviceKey.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, descKey.c_str(), -1, SQLITE_TRANSIENT);
  if (parentId > 0) {
    sqlite3_bind_int(stmt, 3, parentId);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  sqlite3_bind_int(stmt, 4, firmwareId);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::updateService(int id, const std::string &serviceKey,
                                    const std::string &descKey, int firmwareId,
                                    int parentId) {
  const char *sql = "UPDATE Services SET service_key = ?, description_key = ?, "
                    "parent_id = ?, firmware_id = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, serviceKey.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, descKey.c_str(), -1, SQLITE_TRANSIENT);
  if (parentId > 0) {
    sqlite3_bind_int(stmt, 3, parentId);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  sqlite3_bind_int(stmt, 4, firmwareId);
  sqlite3_bind_int(stmt, 5, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::deleteService(int id) {
  const char *sql = "DELETE FROM Services WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<ServiceRecord> RecloserManager::getAllServices() {
  std::vector<ServiceRecord> records;
  const char *sql = "SELECT id, service_key, description_key, "
                    "IFNULL(parent_id, 0), firmware_id FROM Services;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ServiceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.service_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      rec.parent_id = sqlite3_column_int(stmt, 3);
      rec.firmware_id = sqlite3_column_int(stmt, 4);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::vector<ServiceRecord>
RecloserManager::getServicesByParentAndFirmware(int parentId, int firmwareId) {
  std::vector<ServiceRecord> records;
  const char *sql;
  if (parentId > 0) {
    sql = "SELECT id, service_key, description_key, parent_id, firmware_id "
          "FROM Services WHERE parent_id = ? AND firmware_id = ?;";
  } else {
    sql = "SELECT id, service_key, description_key, parent_id, firmware_id "
          "FROM Services WHERE parent_id IS NULL AND firmware_id = ?;";
  }

  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    if (parentId > 0) {
      sqlite3_bind_int(stmt, 1, parentId);
      sqlite3_bind_int(stmt, 2, firmwareId);
    } else {
      sqlite3_bind_int(stmt, 1, firmwareId);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      ServiceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.service_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      rec.parent_id = sqlite3_column_int(stmt, 3);
      rec.firmware_id = sqlite3_column_int(stmt, 4);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<ServiceRecord> RecloserManager::getServiceById(int id) {
  const char *sql =
      "SELECT id, service_key, description_key, IFNULL(parent_id, "
      "0), firmware_id FROM Services WHERE id = ?;";
  sqlite3_stmt *stmt;
  std::optional<ServiceRecord> result = std::nullopt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      ServiceRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.service_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      rec.parent_id = sqlite3_column_int(stmt, 3);
      rec.firmware_id = sqlite3_column_int(stmt, 4);
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

bool RecloserManager::addFeature(const std::string &descKey, int serviceId) {
  const char *sql =
      "INSERT INTO Features (description_key, service_id) VALUES (?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, serviceId);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::updateFeature(int id, const std::string &descKey,
                                    int serviceId) {
  const char *sql =
      "UPDATE Features SET description_key = ?, service_id = ? WHERE id = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, descKey.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, serviceId);
  sqlite3_bind_int(stmt, 3, id);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool RecloserManager::deleteFeature(int id) {
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
RecloserManager::getFeaturesByService(int serviceId) {
  std::vector<FeatureRecord> records;
  const char *sql = "SELECT id, description_key, service_id FROM Features "
                    "WHERE service_id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, serviceId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      FeatureRecord rec;
      rec.id = sqlite3_column_int(stmt, 0);
      rec.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      rec.service_id = sqlite3_column_int(stmt, 2);
      records.push_back(rec);
    }
  }
  sqlite3_finalize(stmt);
  return records;
}

std::optional<FeatureRecord> RecloserManager::getFeatureById(int id) {
  const char *sql =
      "SELECT id, description_key, service_id FROM Features WHERE id = ?;";
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
      result = rec;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

std::optional<RecloserManager::ServiceLayoutRecord>
RecloserManager::getScreenLayout(int serviceId) {
  // Get service details
  const char *serviceSql =
      "SELECT id, service_key, description_key FROM Services "
      "WHERE id = ?;";

  sqlite3_stmt *serviceStmt;
  ServiceLayoutRecord layout;
  bool found = false;

  if (sqlite3_prepare_v2(db, serviceSql, -1, &serviceStmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(serviceStmt, 1, serviceId);
    if (sqlite3_step(serviceStmt) == SQLITE_ROW) {
      layout.service_id = sqlite3_column_int(serviceStmt, 0);
      layout.service_key =
          reinterpret_cast<const char *>(sqlite3_column_text(serviceStmt, 1));
      layout.description_key =
          reinterpret_cast<const char *>(sqlite3_column_text(serviceStmt, 2));
      found = true;
    }
  }
  sqlite3_finalize(serviceStmt);

  if (!found)
    return std::nullopt;

  layout.translations = getTranslationsForKey(layout.description_key);

  // Get features for this service
  const char *layoutSql =
      "SELECT f.id, f.description_key, c.type, c.key, fl.id "
      "FROM Features f "
      "LEFT JOIN FeatureLayout fl ON f.id = fl.feature_id "
      "LEFT JOIN Component c ON fl.component_id = c.id "
      "WHERE f.service_id = ?;";

  sqlite3_stmt *layoutStmt;
  if (sqlite3_prepare_v2(db, layoutSql, -1, &layoutStmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(layoutStmt, 1, serviceId);

    while (sqlite3_step(layoutStmt) == SQLITE_ROW) {
      FeatureLayoutRecord rec;
      rec.feature_id = sqlite3_column_int(layoutStmt, 0);
      rec.feature_key =
          reinterpret_cast<const char *>(sqlite3_column_text(layoutStmt, 1));

      rec.translations = getTranslationsForKey(rec.feature_key);

      if (sqlite3_column_type(layoutStmt, 2) != SQLITE_NULL) {
        rec.component_type =
            reinterpret_cast<const char *>(sqlite3_column_text(layoutStmt, 2));
        rec.component_key =
            reinterpret_cast<const char *>(sqlite3_column_text(layoutStmt, 3));
        int layoutId = sqlite3_column_int(layoutStmt, 4);

        // Get limits for this layout
        const char *limitSql = "SELECT l.key, fll.value "
                               "FROM FeatureLayoutLimits fll "
                               "JOIN Limits l ON fll.limit_id = l.id "
                               "WHERE fll.layout_id = ?;";

        sqlite3_stmt *limitStmt;
        if (sqlite3_prepare_v2(db, limitSql, -1, &limitStmt, nullptr) ==
            SQLITE_OK) {
          sqlite3_bind_int(limitStmt, 1, layoutId);
          while (sqlite3_step(limitStmt) == SQLITE_ROW) {
            LayoutLimitRecord lim;
            lim.key = reinterpret_cast<const char *>(
                sqlite3_column_text(limitStmt, 0));
            lim.value = reinterpret_cast<const char *>(
                sqlite3_column_text(limitStmt, 1));
            rec.limits.push_back(lim);
          }
        }
        sqlite3_finalize(limitStmt);
      }
      layout.features.push_back(rec);
    }
  }
  sqlite3_finalize(layoutStmt);

  // Recursively get children layouts
  const char *childrenSql = "SELECT id FROM Services WHERE parent_id = ?;";
  sqlite3_stmt *childrenStmt;
  if (sqlite3_prepare_v2(db, childrenSql, -1, &childrenStmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(childrenStmt, 1, serviceId);
    while (sqlite3_step(childrenStmt) == SQLITE_ROW) {
      int childId = sqlite3_column_int(childrenStmt, 0);
      auto childLayout = getScreenLayout(childId);
      if (childLayout) {
        layout.children.push_back(*childLayout);
      }
    }
  }
  sqlite3_finalize(childrenStmt);

  return layout;
}

bool RecloserManager::populateSampleLayoutData() {
  // 1. Attach layout for Overcurrent Protection (feature_id=1) -> Integer
  // (component_id=4)
  sqlite3_exec(db,
               "INSERT OR IGNORE INTO FeatureLayout (feature_id, component_id) "
               "VALUES (1, 4);",
               nullptr, nullptr, nullptr);

  // 2. Attach limits for Overcurrent Protection
  // MIN_VALUE=1, MAX_VALUE=2, STEP=4
  sqlite3_exec(
      db,
      "INSERT OR IGNORE INTO FeatureLayoutLimits (layout_id, limit_id, value) "
      "VALUES (1, 1, '0'), (1, 2, '5000'), (1, 4, '1');",
      nullptr, nullptr, nullptr);

  // 3. Attach layout for Reclose Count Limit (feature_id=2) -> ComboBox
  // (component_id=1)
  sqlite3_exec(db,
               "INSERT OR IGNORE INTO FeatureLayout (feature_id, component_id) "
               "VALUES (2, 1);",
               nullptr, nullptr, nullptr);

  // 4. Attach limits for Reclose Count Limit
  // MAX_CHAR=5
  sqlite3_exec(db,
               "INSERT OR IGNORE INTO FeatureLayoutLimits (layout_id, "
               "limit_id, value) VALUES (2, 5, '2');",
               nullptr, nullptr, nullptr);

  return true;
}
