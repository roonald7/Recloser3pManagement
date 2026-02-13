# Refactoring ON/OFF Labels to Use ComponentOptions

## Overview

Refactored the system to use `ComponentOptions` for ON/OFF labels in TOGGLE and CHECKBOX components instead of separate `on_label_translations` and `off_label_translations` fields.

## Benefits

1. **Consistency**: All translatable options use the same `ComponentOption` structure
2. **Simplicity**: Fewer fields to manage in the protobuf definition
3. **Flexibility**: Easy to extend with additional states (e.g., "UNKNOWN", "ERROR")
4. **Reusability**: The same structure works for toggles, checkboxes, and comboboxes

## Changes Made

### 1. Protobuf Definition (`device.proto`)

**Before:**

```protobuf
message FeatureComponentDetail {
  // ... other fields
  repeated Translation on_label_translations = 7;
  repeated Translation off_label_translations = 8;
  repeated ComponentOption options = 9;
}
```

**After:**

```protobuf
message FeatureComponentDetail {
  // ... other fields
  repeated ComponentOption options = 7;  // Includes ON/OFF labels for toggles
}
```

### 2. Backend Implementation (`DeviceServiceImpl.cpp`)

Updated `populateFeatureDetail()` to add ON/OFF labels as `ComponentOptions` with special values:

```cpp
// Add ON/OFF labels as options for TOGGLE/CHECKBOX components
if (!rec.on_translations.empty()) {
  auto *onOption = detail->add_options();
  onOption->set_value("ON");
  for (const auto &t : rec.on_translations) {
    auto *trans = onOption->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }
}

if (!rec.off_translations.empty()) {
  auto *offOption = detail->add_options();
  offOption->set_value("OFF");
  for (const auto &t : rec.off_translations) {
    auto *trans = offOption->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }
}

// Add regular options (for COMBOBOX components)
for (const auto &opt : rec.options) {
  // ... existing option population
}
```

### 3. Frontend Implementation (`Configurator.tsx`)

Updated the TOGGLE component to look for ON/OFF options in the options array:

**Before:**

```tsx
{isToggled
  ? getTranslation(feature.on_label_translations) || 'Enabled'
  : getTranslation(feature.off_label_translations) || 'Disabled'
}
```

**After:**

```tsx
const onOption = feature.options?.find((opt: any) => opt.value === 'ON');
const offOption = feature.options?.find((opt: any) => opt.value === 'OFF');

{isToggled
  ? (onOption ? getTranslation(onOption.translations) : 'Enabled')
  : (offOption ? getTranslation(offOption.translations) : 'Disabled')
}
```

## Data Structure

The options array now contains:

- **For TOGGLE/CHECKBOX**: Options with `value = "ON"` and `value = "OFF"` containing their translations
- **For COMBOBOX**: Regular options with their respective values and translations

## Example Data

```json
{
  "feature_id": 123,
  "component_type": "TOGGLE",
  "options": [
    {
      "value": "ON",
      "translations": [
        { "language_code": "enUs", "value": "Enabled" },
        { "language_code": "ptBr", "value": "Ativado" }
      ]
    },
    {
      "value": "OFF",
      "translations": [
        { "language_code": "enUs", "value": "Disabled" },
        { "language_code": "ptBr", "value": "Desativado" }
      ]
    }
  ]
}
```

## Migration Notes

- **Database Schema Updated**: Removed `ON_LABEL` and `OFF_LABEL` from the `Limits` table initialization
- **DeviceManager Updated**: `FeatureComponentRecord` no longer has `on_translations` and `off_translations` fields
- **Data Storage**: ON/OFF labels are now stored as `ComponentOptions` with `value = "ON"` or `value = "OFF"` in the database
- **Backend Conversion**: `DeviceManager::getScreenLayout()` converts ON_LABEL and OFF_LABEL limits to ComponentOptions when reading from the database
- **gRPC Layer**: `DeviceServiceImpl::populateFeatureDetail()` simply passes through the options without conversion
- **No Migration Required**: Existing databases with ON_LABEL/OFF_LABEL limits will be automatically converted to ComponentOptions when read
- **New Data**: New toggle/checkbox features should store ON/OFF labels as ComponentOptions in the `ComponentOptions` table, not as limits

## Future Enhancements

- Could add additional states like "UNKNOWN", "ERROR", "PARTIAL" for more complex toggles
- Could standardize all component configuration to use options-based approach
- Could add validation to ensure TOGGLE components have both ON and OFF options
- Could add a database migration to convert existing ON_LABEL/OFF_LABEL limits to ComponentOptions permanently
