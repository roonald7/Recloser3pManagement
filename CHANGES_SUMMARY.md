# ServiceDeviceModelFirmware Table Removal - Summary of Changes

## Completed Changes ✅

### 1. Database Schema (DatabaseSchema.hpp)

- ✅ **Removed** `ServiceDeviceModelFirmware` junction table
- ✅ **Updated** `Features` table to include:
  - `service_id INTEGER NOT NULL`
  - `model_firmware_id INTEGER NOT NULL`
  - UNIQUE constraint on `(service_id, model_firmware_id, description_key)`

### 2. DeviceManager Header (DeviceManager.hpp)

- ✅ **Updated** `FeatureRecord` struct:
  - Added `int service_id;`
  - Added `int model_firmware_id;`
  - Removed `int service_model_firmware_id;`
- ✅ **Removed** junction table methods:
  - `linkServiceToModelFirmware()`
  - `getServiceModelFirmwareId()`  
  - `unlinkServiceFromModelFirmware()`
- ✅ **Updated** method signatures:
  - `addFeature(descKey, serviceId, modelFirmwareId, parentFeatureId)`
  - `updateFeature(id, descKey, serviceId, modelFirmwareId, parentFeatureId)`
  - `getFeaturesByServiceAndModelFirmware(serviceId, modelFirmwareId)` (renamed)
  - `getScreenLayout(serviceId, modelFirmwareId)`

### 3. Protocol Buffers (device.proto)

- ✅ **Updated** `ServiceRecord`:
  - Removed `int32 device_model_firmware_id = 4;`
- ✅ **Updated** `FeatureRecord`:
  - Removed `int32 service_device_model_firmware_id = 3;`
  - Added `int32 service_id = 3;`
  - Added `int32 model_firmware_id = 4;`

### 4. Device Service Implementation (DeviceServiceImpl.cpp)

- ✅ **Updated** `AddServiceNode()`:
  - Removed `linkServiceToModelFirmware()` call
  - Services are now linked via Features only
- ✅ **Updated** `CreateFeature()`:
  - Changed to pass `service_id` and `model_firmware_id` separately
- ✅ **Updated** `UpdateFeature()`:
  - Changed to pass `service_id` and `model_firmware_id` separately

## Remaining Work ⚠️

### DeviceManager.cpp - Critical Updates Needed

The following methods in `DeviceManager.cpp` need to be updated:

#### 1. Remove Junction Table Methods (lines 574-632)

Delete these three methods:

- `linkServiceToModelFirmware()`
- `getServiceModelFirmwareId()`
- `unlinkServiceFromModelFirmware()`

#### 2. Update `getServicesByParentAndModelFirmware()` (lines 666-703)

**Current Query:**

```sql
SELECT s.id, s.description_key, s.parent_id 
FROM Services s 
JOIN ServiceDeviceModelFirmware sdf ON s.id = sdf.service_id 
WHERE s.parent_id = ? AND sdf.model_firmware_id = ?
```

**New Query:**

```sql
SELECT DISTINCT s.id, s.description_key, s.parent_id 
FROM Services s 
JOIN Features f ON s.id = f.service_id 
WHERE s.parent_id = ? AND f.model_firmware_id = ?
```

#### 3. Rewrite `addFeature()` (lines 726-749)

**New Implementation:**

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

#### 4. Rewrite `updateFeature()` (lines 751-773)

**New Implementation:**

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

#### 5. Rename and Rewrite `getFeaturesByServiceAndModelFirmware()` (lines 787-810)

**New Implementation:**

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

#### 6. Rewrite `getScreenLayout()` (lines 876-1045)

**Key Changes:**

- Change signature to `getScreenLayout(int serviceId, int modelFirmwareId)`
- Query Services table directly instead of through junction table
- Remove the dmfId lookup section (lines 1012-1023)
- Pass `serviceId` and `modelFirmwareId` directly to recursive calls

#### 7. Update `getFeatureById()` (lines 855-874)

Update SQL to select the new columns:

```sql
SELECT id, description_key, service_id, model_firmware_id, parent_feature_id 
FROM Features WHERE id = ?
```

### DeviceServiceImpl.cpp - Additional Updates Needed

#### 1. Update `GetScreenLayout()` (lines 269-326)

Change from:

```cpp
auto layoutResult = manager_->getScreenLayout(targetSdfId);
```

To:

```cpp
// Need to find a service ID first from the features or use the request service_id  
int serviceId = request->service_id();
if (serviceId <= 0) {
  // Find first root service for this model firmware
  auto topServices = manager_->getServicesByParentAndModelFirmware(0, dmfId);
  if (!topServices.empty()) {
    serviceId = topServices[0].id;
  }
}
auto layoutResult = manager_->getScreenLayout(serviceId, dmfId);
```

#### 2. Update all tree building methods

Change all calls from:

- `getFeaturesByServiceModelFirmware(smfId)`
To:
- `getFeaturesByServiceAndModelFirmware(serviceId, mfId)`

This requires extracting the service_id from the appropriate context.

### ServiceHelpers.cpp - Updates Needed

Update the `buildServiceNode()` and related methods to use the new API signatures.

## Client-Side

✅ **No changes needed** - The client already passes device_id, model_id, and firmware_id separately.

## Next Steps

1. **Complete DeviceManager.cpp updates** (most critical)
2. **Complete DeviceServiceImpl.cpp updates** (for GetScreenLayout and tree building)
3. **Update ServiceHelpers.cpp**
4. **Test the build**
5. **Migrate existing database** (if any data exists)
6. **Test end-to-end functionality**

## Migration Notes

If you have an existing database, you'll need a migration script:

```sql
-- Create new Features table structure
CREATE TABLE Features_new (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  description_key TEXT NOT NULL,
  service_id INTEGER NOT NULL,
  model_firmware_id INTEGER NOT NULL,
  parent_feature_id INTEGER,
  FOREIGN KEY (description_key) REFERENCES Descriptions(key),
  FOREIGN KEY (service_id) REFERENCES Services(id) ON DELETE CASCADE,
  FOREIGN KEY (model_firmware_id) REFERENCES ModelFirmware(id) ON DELETE CASCADE,
  FOREIGN KEY (parent_feature_id) REFERENCES Features(id) ON DELETE CASCADE,
  UNIQUE(service_id, model_firmware_id, description_key)
);

-- Migrate data
INSERT INTO Features_new (id, description_key, service_id, model_firmware_id, parent_feature_id)
SELECT 
  f.id, 
  f.description_key, 
  sdf.service_id,
  sdf.model_firmware_id,
  f.parent_feature_id
FROM Features f
JOIN ServiceDeviceModelFirmware sdf ON f.service_device_model_firmware_id = sdf.id;

-- Drop old tables
DROP TABLE Features;
DROP TABLE ServiceDeviceModelFirmware;

-- Rename new table
ALTER TABLE Features_new RENAME TO Features;
```
