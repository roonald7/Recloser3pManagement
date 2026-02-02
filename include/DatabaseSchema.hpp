#pragma once
#include <string>
#include <vector>

namespace Schema {
    const std::vector<std::string> INITIALIZATION_SQL = {
        "CREATE TABLE IF NOT EXISTS Languages ("
        "code TEXT PRIMARY KEY NOT NULL,"
        "name TEXT NOT NULL);",

        "CREATE TABLE IF NOT EXISTS Descriptions ("
        "key TEXT PRIMARY KEY NOT NULL);",

        "CREATE TABLE IF NOT EXISTS Translations ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "description_key TEXT NOT NULL,"
        "language_code TEXT NOT NULL,"
        "value TEXT NOT NULL,"
        "UNIQUE(description_key, language_code),"
        "FOREIGN KEY (description_key) REFERENCES Descriptions(key) ON DELETE CASCADE,"
        "FOREIGN KEY (language_code) REFERENCES Languages(code) ON DELETE CASCADE);",

        "CREATE TABLE IF NOT EXISTS Reclosers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "description_key TEXT NOT NULL,"
        "model TEXT NOT NULL,"
        "FOREIGN KEY (description_key) REFERENCES Descriptions(key));",

        "CREATE TABLE IF NOT EXISTS FirmwareVersions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "version TEXT NOT NULL,"
        "recloser_id INTEGER NOT NULL,"
        "FOREIGN KEY (recloser_id) REFERENCES Reclosers(id) ON DELETE CASCADE);",

        "CREATE TABLE IF NOT EXISTS Services ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "service_key TEXT UNIQUE NOT NULL,"
        "description_key TEXT NOT NULL,"
        "parent_id INTEGER,"
        "firmware_id INTEGER NOT NULL,"
        "FOREIGN KEY (description_key) REFERENCES Descriptions(key),"
        "FOREIGN KEY (parent_id) REFERENCES Services(id) ON DELETE CASCADE,"
        "FOREIGN KEY (firmware_id) REFERENCES FirmwareVersions(id) ON DELETE CASCADE);",

        "CREATE TABLE IF NOT EXISTS Features ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "description_key TEXT NOT NULL,"
        "service_id INTEGER NOT NULL,"
        "FOREIGN KEY (description_key) REFERENCES Descriptions(key),"
        "FOREIGN KEY (service_id) REFERENCES Services(id) ON DELETE CASCADE);"
    };
}
