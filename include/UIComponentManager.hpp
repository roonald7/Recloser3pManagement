#pragma once

#include "sqlite3.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>


// ============================================================================
// UI Component Records
// ============================================================================

struct ComponentType {
  int id;
  std::string type; // e.g., "ComboBox", "TextField", "Decimal"
  std::string key;  // e.g., "cb", "tf", "dec"
};

struct LimitType {
  int id;
  std::string key; // e.g., "MIN_VALUE", "MAX_VALUE", "DEFAULT_VALUE"
};

// ============================================================================
// Parameter Records
// ============================================================================

struct ParameterRecord {
  int id;
  std::string name;
  std::string description_key;
  int component_id;
  int feature_id;
};

struct ParameterLimitValue {
  int id;
  int parameter_id;
  int limit_id;
  std::string value;
};

// ============================================================================
// Combined View Structures
// ============================================================================

struct ParameterDefinition {
  int id;
  std::string name;
  std::string description_key;

  // Component info
  ComponentType component;

  // Limits (optional)
  std::optional<std::string> min_value;
  std::optional<std::string> max_value;
  std::optional<std::string> default_value;
  std::optional<std::string> step;
  std::optional<std::string> max_char;

  int feature_id;
};

// ============================================================================
// UI Component Manager
// ============================================================================

class UIComponentManager {
public:
  UIComponentManager(sqlite3 *db);
  ~UIComponentManager() = default;

  // Component Type Methods
  std::vector<ComponentType> getAllComponentTypes();
  std::optional<ComponentType> getComponentTypeById(int id);
  std::optional<ComponentType> getComponentTypeByKey(const std::string &key);
  bool addComponentType(const std::string &type, const std::string &key);

  // Limit Type Methods
  std::vector<LimitType> getAllLimitTypes();
  std::optional<LimitType> getLimitTypeById(int id);
  std::optional<LimitType> getLimitTypeByKey(const std::string &key);
  bool addLimitType(const std::string &key);

  // Parameter Methods
  bool addParameter(const std::string &name, const std::string &descKey,
                    int componentId, int featureId);
  std::optional<ParameterRecord> getParameterById(int id);
  std::vector<ParameterRecord> getParametersByFeature(int featureId);
  std::vector<ParameterRecord> getAllParameters();

  // Parameter Limit Methods
  bool setParameterLimit(int parameterId, const std::string &limitKey,
                         const std::string &value);
  std::optional<std::string> getParameterLimit(int parameterId,
                                               const std::string &limitKey);
  std::vector<ParameterLimitValue> getParameterLimits(int parameterId);
  bool removeParameterLimit(int parameterId, const std::string &limitKey);

  // Combined View Methods
  std::optional<ParameterDefinition> getParameterDefinition(int parameterId);
  std::vector<ParameterDefinition>
  getParameterDefinitionsByFeature(int featureId);

  // Validation Methods
  bool validateParameterValue(int parameterId, const std::string &value);
  std::string getValidationMessage(int parameterId, const std::string &value);

  // Utility Methods
  bool componentTypeExists(const std::string &key);
  bool limitTypeExists(const std::string &key);
  int getComponentIdByKey(const std::string &key);
  int getLimitIdByKey(const std::string &key);

private:
  sqlite3 *db;

  // Helper methods
  std::optional<ParameterLimitValue> findLimitValue(int parameterId,
                                                    int limitId);
  bool isNumericComponent(const std::string &componentKey);
  bool isDateTimeComponent(const std::string &componentKey);
  bool isTextComponent(const std::string &componentKey);
};
