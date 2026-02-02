#pragma once
#include <string>
#include <vector>

namespace Schema {
const std::vector<std::string> INITIALIZATION_SQL = {
    "CREATE TABLE IF NOT EXISTS Languages (code TEXT PRIMARY KEY NOT NULL, "
    "name TEXT NOT NULL);",

    "CREATE TABLE IF NOT EXISTS Descriptions ("
    "key TEXT PRIMARY KEY NOT NULL);",

    "CREATE TABLE IF NOT EXISTS Translations (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT NOT NULL, language_code TEXT NOT "
    "NULL, value TEXT NOT NULL, UNIQUE(description_key, language_code), "
    "FOREIGN KEY (description_key) REFERENCES Descriptions(key) ON DELETE "
    "CASCADE, FOREIGN KEY (language_code) REFERENCES Languages(code) ON DELETE "
    "CASCADE);",

    "CREATE TABLE IF NOT EXISTS Reclosers (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT NOT NULL, model TEXT NOT NULL, "
    "FOREIGN KEY (description_key) REFERENCES Descriptions(key));",

    "CREATE TABLE IF NOT EXISTS FirmwareVersions (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, version TEXT NOT NULL, recloser_id INTEGER NOT NULL, "
    "FOREIGN KEY (recloser_id) REFERENCES Reclosers(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS Services (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, service_key TEXT UNIQUE NOT NULL, description_key TEXT NOT "
    "NULL, parent_id INTEGER, firmware_id INTEGER NOT NULL, FOREIGN KEY "
    "(description_key) REFERENCES Descriptions(key), FOREIGN KEY (parent_id) "
    "REFERENCES Services(id) ON DELETE CASCADE, FOREIGN KEY (firmware_id) "
    "REFERENCES FirmwareVersions(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS Features (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, description_key TEXT NOT NULL, service_id INTEGER NOT "
    "NULL, FOREIGN KEY (description_key) REFERENCES Descriptions(key), FOREIGN "
    "KEY (service_id) REFERENCES Services(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS Component (id INTEGER PRIMARY KEY "
    "AUTOINCREMENT, type TEXT NOT NULL, key TEXT UNIQUE NOT NULL);",

    "CREATE TABLE IF NOT EXISTS Limits ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "key TEXT UNIQUE NOT NULL);",

    // Insert default component types
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('ComboBox', 'cb');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('TextField', 'tf');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Decimal', 'dec');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Integer', 'int');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Date', 'date');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Time', 'time');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('DateTime', 'dt');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Spinner', "
    "'spinner');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('CheckBox', 'chBox');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Toggle', 'tgBut');",
    "INSERT OR IGNORE INTO Component (type, key) VALUES ('Button', 'bt');",

    // Insert default limit keys
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MIN_VALUE');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MAX_VALUE');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('DEFAULT_VALUE');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('STEP');",
    "INSERT OR IGNORE INTO Limits (key) VALUES ('MAX_CHAR');",

    "CREATE TABLE IF NOT EXISTS FeatureLayout ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "feature_id INTEGER NOT NULL,"
    "component_id INTEGER NOT NULL,"
    "FOREIGN KEY (feature_id) REFERENCES Features(id) ON DELETE CASCADE,"
    "FOREIGN KEY (component_id) REFERENCES Component(id) ON DELETE CASCADE);",

    "CREATE TABLE IF NOT EXISTS FeatureLayoutLimits ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "layout_id INTEGER NOT NULL,"
    "limit_id INTEGER NOT NULL,"
    "value TEXT NOT NULL,"
    "FOREIGN KEY (layout_id) REFERENCES FeatureLayout(id) ON DELETE CASCADE,"
    "FOREIGN KEY (limit_id) REFERENCES Limits(id) ON DELETE CASCADE);"};
}
