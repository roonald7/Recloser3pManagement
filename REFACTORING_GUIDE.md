# ServiceDeviceModelFirmware Table Removal - Implementation Guide

## Summary

The ServiceDeviceModelFirmware junction table has been removed from the database schema. Services are now linked to ModelFirmware combinations directly through the Features table.

## Database Schema Changes - COMPLETED ✅

- **Removed**: `ServiceDeviceModelFirmware` table
- **Updated**: `Features` table now has `service_id` and `model_firmware_id` columns directly
  - UNIQUE constraint on (service_id, model_firmware_id, description_key)

## DeviceManager.hpp Changes - COMPLETED ✅

- **Updated**: `FeatureRecord` struct now has `service_id` and `model_firmware_id` fields
- **Removed**: Junction table methods:
  - `linkServiceToModelFirmware()`
  - `getServiceModelFirmwareId()`
  - `unlinkServiceFromModelFirmware()`
- **Updated**: Method signatures:
  - `addFeature(descKey, serviceId, modelFirmwareId, parentFeatureId)`
  - `updateFeature(id, descKey, serviceId, modelFirmwareId, parentFeatureId)`
  - `getFeaturesByServiceAndModelFirmware(serviceId, modelFirmwareId)`
  - `getScreenLayout(serviceId, modelFirmwareId)`

## DeviceManager.cpp Changes - NEEDED

### 1. DELETE these methods (lines 574-632)

- `linkServiceToModelFirmware()`
- `getServiceModelFirmwareId()`  
- `unlinkServiceFromModelFirmware()`

### 2. UPDATE `getServicesByParentAndModelFirmware()` (around line 666-703)

Change query from:

```cpp
sql = "SELECT s.id, s.description_key, s.parent_id "
      "FROM Services s "
      "JOIN ServiceDeviceModelFirmware sdf ON s.id = sdf.service_id "
      "WHERE s.parent_id = ? AND sdf.model_firmware_id = ?;";
```

To:

```cpp
sql = "SELECT DISTINCT s.id, s.description_key, s.parent_id "
      "FROM Services s "
      "JOIN Features f ON s.id = f.service_id "
      "WHERE s.parent_id = ? AND f.model_firmware_id = ?;";
```

### 3. UPDATE `addFeature()` (around line 726-749)

Change signature and SQL:

```cpp
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
```

### 4. UPDATE `updateFeature()` (around line 751-773)

```cpp
bool DeviceManager::updateFeature(int id, const std::string &descKey,
                                  int serviceId, int modelFirmwareId,
                                  int parentFeatureId) {
  const char *sql =
      "UPDATE Features SET description_key = ?, "
      "service_id = ?, model_firmware_id = ?, parent_feature_id = ? WHERE id = ?;";
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
```

### 5. UPDATE `getFeaturesByServiceAndModelFirmware()` (around line  787-810)

Rename from `getFeaturesByServiceModelFirmware` and update:

```cpp
std::vector<FeatureRecord>
DeviceManager::getFeaturesByServiceAndModelFirmware(int serviceId, int modelFirmwareId) {
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
```

### 6. UPDATE `getScreenLayout()` (around line 876-1045)

Change signature and implementation:

```cpp
std::optional<DeviceManager::ServiceLayoutRecord>
DeviceManager::getScreenLayout(int serviceId, int modelFirmwareId) {
  // Get service details directly
  const char *serviceSql =
      "SELECT id, description_key "
      "FROM Services WHERE id = ?;";

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

    // ... rest of the method remains the same, just with updated parameters ...
  }

  // Remove the section that retrieves dmfId from ServiceDeviceModelFirmware (lines 1012-1023)
  
  // Update children query to use service_id and modelFirmwareId directly:
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
  }
  sqlite3_finalize(childrenStmt);

  return layout;
}
```

### 7. UPDATE `getFeatureById()` (around line 855-874)

Update column name in SQL:

```cpp
const char *sql = "SELECT id, description_key, service_id, model_firmware_id FROM "
                  "Features WHERE id = ?;";
// And update the column reads:
rec.service_id = sqlite3_column_int(stmt, 2);
rec.model_firmware_id = sqlite3_column_int(stmt, 3);
```

## DeviceServiceImpl.cpp Changes - NEEDED

### 1. UPDATE `AddServiceNode()` (around line 466-481)

Remove the linkServiceToModelFirmware call - services are now linked via features only.

### 2. UPDATE `CreateFeature()` (around line 504-513)

Update to pass service_id and model_firmware_id instead of service_device_model_firmware_id.
This requires changes to the proto file first.

### 3. UPDATE `UpdateFeature()` (around line 515-525)

Same as CreateFeature.

### 4. UPDATE `Get ScreenLayout()` (around line 269-326)

Change the call from:

```cpp
auto layoutResult = manager_->getScreenLayout(targetSdfId);
```

To:

```cpp
auto layoutResult = manager_->getScreenLayout(serviceId, dmfId);
```

### 5. UPDATE `GetFullInventory()` and tree building methods

Change all calls from `getFeaturesByServiceModelFirmware(smfId)` to
`getFeaturesByServiceAndModelFirmware(serviceId, mfId)`.

## Proto File Changes - NEEDED

Update `FeatureRecord` message to use `service_id` and `model_firmware_id`:

```protobuf
message FeatureRecord {
  int32 id = 1;
  string description_key = 2;
  int32 service_id = 3;
  int32 model_firmware_id = 4;
}
```

Update `ServiceRecord` to remove `device_model_firmware_id` field.

## ServiceHelpers.cpp Changes - NEEDED

Update all calls to use the new method signatures.

## Client Changes

No changes needed - client already uses device_id, model_id, firmware_id separately.
