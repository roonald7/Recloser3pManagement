#include "RecloserManager.hpp"
#include "DatabaseSchema.hpp"
#include <iostream>

RecloserManager::RecloserManager(const std::string& dbPath) : dbPath(dbPath), db(nullptr) {}

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

bool RecloserManager::runSchema() {
    for (const auto& sql : Schema::INITIALIZATION_SQL) {
        char* zErrMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error during schema initialization: " << zErrMsg << std::endl;
            sqlite3_free(zErrMsg);
            return false;
        }
    }
    return true;
}


bool RecloserManager::addLanguage(const std::string& code, const std::string& name) {
    const char* sql = "INSERT OR IGNORE INTO Languages (code, name) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool RecloserManager::addDescriptionKey(const std::string& key) {
    const char* sql = "INSERT OR IGNORE INTO Descriptions (key) VALUES (?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool RecloserManager::addTranslation(const std::string& key, const std::string& langCode, const std::string& value) {
    const char* sql = "INSERT OR REPLACE INTO Translations (description_key, language_code, value) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, langCode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::string RecloserManager::getTranslation(const std::string& key, const std::string& langCode) {
    const char* sql = "SELECT value FROM Translations WHERE description_key = ? AND language_code = ?;";
    sqlite3_stmt* stmt;
    std::string result = "";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, langCode.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

bool RecloserManager::addRecloser(const std::string& key, const std::string& model) {
    const char* sql = "INSERT INTO Reclosers (description_key, model) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, model.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<RecloserRecord> RecloserManager::getAllReclosers() {
    std::vector<RecloserRecord> records;
    const char* sql = "SELECT id, description_key, model FROM Reclosers;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            RecloserRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.description_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rec.model = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            records.push_back(rec);
        }
    }
    sqlite3_finalize(stmt);
    return records;
}

bool RecloserManager::addFirmwareVersion(const std::string& version, int recloserId) {
    const char* sql = "INSERT INTO FirmwareVersions (version, recloser_id) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, version.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, recloserId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<FirmwareVersionRecord> RecloserManager::getFirmwareVersionsForRecloser(int recloserId) {
    std::vector<FirmwareVersionRecord> records;
    const char* sql = "SELECT id, version, recloser_id FROM FirmwareVersions WHERE recloser_id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, recloserId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            FirmwareVersionRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rec.recloser_id = sqlite3_column_int(stmt, 2);
            records.push_back(rec);
        }
    }
    sqlite3_finalize(stmt);
    return records;
}

bool RecloserManager::addService(const std::string& serviceKey, const std::string& descKey, int parentId) {
    const char* sql = "INSERT INTO Services (service_key, description_key, parent_id) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, serviceKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, descKey.c_str(), -1, SQLITE_TRANSIENT);
    if (parentId > 0) {
        sqlite3_bind_int(stmt, 3, parentId);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<ServiceRecord> RecloserManager::getAllServices() {
    std::vector<ServiceRecord> records;
    const char* sql = "SELECT id, service_key, description_key, IFNULL(parent_id, 0) FROM Services;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ServiceRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.service_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rec.description_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            rec.parent_id = sqlite3_column_int(stmt, 3);
            records.push_back(rec);
        }
    }
    sqlite3_finalize(stmt);
    return records;
}

std::vector<ServiceRecord> RecloserManager::getServicesByParent(int parentId) {
    std::vector<ServiceRecord> records;
    const char* sql;
    if (parentId > 0) {
        sql = "SELECT id, service_key, description_key, parent_id FROM Services WHERE parent_id = ?;";
    } else {
        sql = "SELECT id, service_key, description_key, parent_id FROM Services WHERE parent_id IS NULL;";
    }
    
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (parentId > 0) {
            sqlite3_bind_int(stmt, 1, parentId);
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ServiceRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.service_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rec.description_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            rec.parent_id = sqlite3_column_int(stmt, 3);
            records.push_back(rec);
        }
    }
    sqlite3_finalize(stmt);
    return records;
}
