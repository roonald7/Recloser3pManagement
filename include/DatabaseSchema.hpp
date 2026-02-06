#pragma once
#include <map>
#include <string>
#include <vector>

namespace Schema {
const std::vector<std::string> INITIALIZATION_SQL = {
    "CREATE TABLE IF NOT EXISTS Migrations (id INTEGER PRIMARY KEY, version "
    "INTEGER UNIQUE NOT NULL, applied_at DATETIME DEFAULT CURRENT_TIMESTAMP);",
    "CREATE TABLE IF NOT EXISTS Languages (code TEXT PRIMARY KEY NOT NULL, "
    "name TEXT NOT NULL);",

    "CREATE TABLE IF NOT EXISTS Descriptions (key TEXT PRIMARY KEY NOT NULL);",

    "CREATE TABLE IF NOT EXISTS Translations (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT NOT NULL, language_code TEXT NOT "
    "NULL, value TEXT NOT NULL, UNIQUE(description_key, language_code), "
    "FOREIGN KEY (description_key) REFERENCES Descriptions(key) ON DELETE "
    "CASCADE, FOREIGN KEY (language_code) REFERENCES Languages(code) ON DELETE "
    "CASCADE);",

    "CREATE TABLE IF NOT EXISTS Reclosers (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT UNIQUE NOT NULL, "
    "FOREIGN KEY (description_key) REFERENCES Descriptions(key));",

    "CREATE TABLE IF NOT EXISTS FirmwareVersions (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, version TEXT NOT NULL, recloser_id INTEGER NOT NULL, "
    "FOREIGN KEY (recloser_id) REFERENCES Reclosers(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS Services (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT UNIQUE NOT NULL, "
    "parent_id INTEGER, FOREIGN KEY "
    "(description_key) REFERENCES Descriptions(key), FOREIGN KEY (parent_id) "
    "REFERENCES Services(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS ServiceFirmware (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, service_id INTEGER NOT NULL, firmware_id INTEGER NOT NULL, "
    "FOREIGN KEY (service_id) REFERENCES Services(id) ON DELETE CASCADE, "
    "FOREIGN KEY (firmware_id) REFERENCES FirmwareVersions(id) ON DELETE "
    "CASCADE, "
    "UNIQUE(service_id, firmware_id));",

    "CREATE TABLE IF NOT EXISTS Features (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT NOT NULL, service_firmware_id INTEGER "
    "NOT "
    "NULL, FOREIGN KEY (description_key) REFERENCES Descriptions(key), FOREIGN "
    "KEY (service_firmware_id) REFERENCES ServiceFirmware(id) ON DELETE "
    "CASCADE);",

    "CREATE TABLE IF NOT EXISTS Component (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, type TEXT UNIQUE NOT NULL);",

    "CREATE TABLE IF NOT EXISTS Limits (id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "key TEXT UNIQUE NOT NULL);",

    // Insert default component types
    "INSERT OR IGNORE INTO Component (type) VALUES ('ComboBox');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('TextField');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Decimal');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Integer');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Date');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Time');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('DateTime');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Spinner');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('CheckBox');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Toggle');",
    "INSERT OR IGNORE INTO Component (type) VALUES ('Button');",

    // Insert default limit keys
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MIN_VALUE');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MAX_VALUE');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('DEFAULT_VALUE');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('STEP');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MAX_CHAR');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MIN_CHAR');",

    "CREATE TABLE IF NOT EXISTS FeatureComponent ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "feature_id INTEGER NOT NULL,"
    "component_id INTEGER NOT NULL,"
    "FOREIGN KEY (feature_id) REFERENCES Features(id) ON DELETE CASCADE,"
    "FOREIGN KEY (component_id) REFERENCES Component(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS FeatureComponentLimits ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "feature_component_id INTEGER NOT NULL,"
    "limit_id INTEGER NOT NULL,"
    "value TEXT NOT NULL,"
    "FOREIGN KEY (feature_component_id) REFERENCES FeatureComponent(id) ON "
    "DELETE "
    "CASCADE,"
    "FOREIGN KEY (limit_id) REFERENCES Limits(id) ON DELETE CASCADE);",
    "INSERT OR IGNORE INTO Migrations (version) VALUES (1);"};

const std::map<int, std::vector<std::string>> MIGRATIONS_SQL = {
    // Example for version 2:
    // {2, {"ALTER TABLE ...", "UPDATE ..."}}
};
} // namespace Schema
