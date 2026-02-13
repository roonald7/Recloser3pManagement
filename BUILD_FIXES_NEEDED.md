# BUILD FIXES NEEDED - ServiceDeviceModelFirmware Removal

## STATUS: Partially Complete - Build Will Fail

The major structural changes are done, but several method implementations still need updating to match the new signatures.

## COMPLETED ✅

1. **DatabaseSchema.hpp** - Table structure updated
2. **DeviceManager.hpp** - Method signatures updated
3. **device.proto** - Message definitions updated  
4. **DeviceManager.cpp** - PARTIALLY updated:
   - ✅ `addFeature()` - Updated
   - ✅ `updateFeature()` - Updated
   - ✅ `getFeaturesByServiceAndModelFirmware()` - Renamed and updated
   - ✅ `getServicesByParentAndModelFirmware()` - Updated to use Features join
5. **DeviceServiceImpl.cpp** - PARTIALLY updated:
   - ✅ `AddServiceNode()` - Removed junction table call
   - ✅ `CreateFeature()` - Updated to use new signature
   - ✅ `UpdateFeature()` - Updated to use new signature

## CRITICAL BUILD FAILURES - Must Fix ⚠️

### 1. DeviceManager.cpp - getScreenLayout() [Line 880-1048]

**Current signature:**

```cpp
std::optional<DeviceManager::ServiceLayoutRecord>
DeviceManager::getScreenLayout(int serviceModelFirmwareId)
```

**Required signature:**

```cpp
std::optional<DeviceManager::ServiceLayoutRecord>
DeviceManager::getScreenLayout(int serviceId, int modelFirmwareId)
```

**Changes Needed:**

- Line 881: Change parameter from `int serviceModelFirmwareId` to `int serviceId, int modelFirmwareId`
- Lines 883-887: Remove JOIN with ServiceDeviceModelFirmware:

  ```cpp
  // OLD:
  const char *serviceSql =
      "SELECT s.id, s.description_key "
      "FROM Services s "
      "JOIN ServiceDeviceModelFirmware sdf ON s.id = sdf.service_id "
      "WHERE sdf.id = ?;";
      
  // NEW:
  const char *serviceSql =
      "SELECT id, description_key "
      "FROM Services "
      "WHERE id = ?;";
  ```

- Line 895: Change from `serviceModelFirmwareId` to `serviceId`
- Line 920: Change WHERE clause:

  ```cpp
  // OLD:
  "WHERE f.service_device_model_firmware_id = ?;";
  
  // NEW:
  "WHERE f.service_id = ? AND f.model_firmware_id = ?;";
  ```

- Line 925: Bind both parameters:

  ```cpp
  sqlite3_bind_int(layoutStmt, 1, serviceId);
  sqlite3_bind_int(layoutStmt, 2, modelFirmwareId);
  ```

- Lines 1016-1027: Remove the dmfId lookup section (no longer needed)
- Line 1037: Change recursive call:

  ```cpp
  // OLD:
  auto childLayout = getScreenLayout(childScfId);
  
  // NEW:
  auto childLayout = getScreenLayout(childServiceId, modelFirmwareId);
  ```

### 2. DeviceManager.cpp - getFeatureById() [Line ~855-874]

**Changes Needed:**
Update SQL to match new schema:

```cpp
// OLD:
const char *sql = "SELECT id, description_key, service_firmware_id FROM "
                  "Features WHERE id = ?;";

// NEW:
const char *sql = "SELECT id, description_key, service_id, model_firmware_id, "
                  "IFNULL(parent_feature_id, 0) FROM Features WHERE id = ?;";
```

Update column assignments:

```cpp
rec.service_id = sqlite3_column_int(stmt, 2);
rec.model_firmware_id = sqlite3_column_int(stmt, 3);
rec.parent_feature_id = sqlite3_column_int(stmt, 4);
```

### 3. DeviceServiceImpl.cpp - Update getFeaturesByServiceModelFirmware calls

**Files with calls to update:**

- Line 136: In `buildServiceNode()`
- Line 173: In `buildInternalTree()`  
- Line 600: In `GetFullInventory()`

**Change from:**

```cpp
auto features = manager_->getFeaturesByServiceModelFirmware(smfId);
```

**Change to:**

```cpp
auto features = manager_->getFeaturesByServiceAndModelFirmware(serviceId, mfId);
```

**Context needed:** You'll need to determine the appropriate `serviceId` from the surrounding code context in each location.

### 4. DeviceServiceImpl.cpp - GetScreenLayout() [Line 269-326]

**Current:**

```cpp
auto layoutResult = manager_->getScreenLayout(targetSdfId);
```

**Change to:**

```cpp
// Need to extract serviceId - it's already tracked in the method
int serviceId = /* extract from context */;
auto layoutResult = manager_->getScreenLayout(serviceId, dmfId);
```

The `targetSdfId` variable represents what was a ServiceDeviceModelFirmware ID but should now be a service ID.

### 5. ServiceHelpers.cpp - Line 72

**Current:**

```cpp
auto features = manager->getFeaturesByServiceModelFirmware(sdfId);
```

**Change to:**

```cpp
auto features = manager->getFeaturesByServiceAndModelFirmware(serviceId, dmfId);
```

You'll need to pass both `serviceId` and `dmfId` (modelFirmwareId) to this helper function.

## METHOD DEPENDENCIES

The following calling chain needs updating:

```
DeviceServiceImpl::GetScreenLayout() 
  → DeviceManager::getScreenLayout(serviceId, mfId)
    → DeviceManager::getScreenLayout(childServiceId, mfId) [recursive]

DeviceServiceImpl::GetFullInventory()
  → DeviceServiceImpl::buildServiceNode(serviceId, mfId)  
    → DeviceManager::getFeaturesByServiceAndModelFirmware(serviceId, mfId)

ServiceHelpers::buildServiceNode()
  → DeviceManager::getFeaturesByServiceAndModelFirmware(serviceId, mfId)
```

## HOW TO PROCEED

### Step 1: Fix getScreenLayout in DeviceManager.cpp
This is the largest single change. Consider:

1. Change the signature
2. Update the query to not use the junction table
3. Update all recursive calls
4. Remove the dmfId lookup section

### Step 2: Fix getFeatureById in DeviceManager.cpp

Quick SQL update.

### Step 3: Update all callers of getFeaturesByServiceModelFirmware

Search and replace, but ensure you have the right context for serviceId.

### Step 4: Update GetScreenLayout in DeviceServiceImpl.cpp

Determine what serviceId should be used.

### Step 5: Regenerate protobuf files

```bash
protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=grpc_cpp_plugin device.proto
```

### Step 6: Build and test

```bash
cmake --build build
```

## NOTES

- The junction table methods (linkServiceToModelFirmware, etc.) in DeviceManager.cpp can safely be deleted but it's not critical for the build
- Client-side code should not need changes
- You may need to migrate existing database data if you have any

## QUICK FIX TEMPLATE

For each method that calls `getFeaturesByServiceModelFirmware`:

1. Find where the service ID comes from in that context
2. The modelFirmwareId is usually already available (often called `dmfId` or `mfId`)
3. Replace the single parameter with two

Example:

```cpp
// Before:
int sdfId = manager_->getServiceModelFirmwareId(service.id, dmfId);
auto features = manager_->getFeaturesByServiceModelFirmware(sdfId);

// After:
auto features = manager_->getFeaturesByServiceAndModelFirmware(service.id, dmfId);
```
