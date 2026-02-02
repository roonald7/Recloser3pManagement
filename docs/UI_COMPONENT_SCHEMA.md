# UI Component Database Schema

## Overview

This document describes the database schema for UI components and their associated limits/constraints. These tables support dynamic form generation and validation for the Recloser Management application.

---

## Table Structures

### Component Table Schema

Stores UI component type definitions used for dynamic form generation.

#### Component Columns

| Column | Type | Constraints | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY, AUTOINCREMENT | Unique identifier for the component |
| `type` | TEXT | NOT NULL | Human-readable component type name |
| `key` | TEXT | UNIQUE, NOT NULL | Short key/abbreviation for the component |

#### Component Default Data

| id | type | key |
| :--- | :--- | :--- |
| 1 | ComboBox | cb |
| 2 | TextField | tf |
| 3 | Decimal | dec |
| 4 | Integer | int |
| 5 | Date | date |
| 6 | Time | time |
| 7 | DateTime | dt |
| 8 | Spinner | spinner |
| 9 | CheckBox | chBox |
| 10 | Toggle | tgBut |
| 11 | Button | bt |

#### Component Type Descriptions

- **ComboBox** (`cb`) - Dropdown selection list
- **TextField** (`tf`) - Single-line text input
- **Decimal** (`dec`) - Floating-point numeric input
- **Integer** (`int`) - Whole number numeric input
- **Date** (`date`) - Date picker
- **Time** (`time`) - Time picker
- **DateTime** (`dt`) - Combined date and time picker
- **Spinner** (`spinner`) - Numerical value selector with increment/decrement buttons
- **CheckBox** (`chBox`) - Binary selection option
- **Toggle** (`tgBut`) - On/Off switch control
- **Button** (`bt`) - Clickable action element

---

### Limits Table Schema

Stores constraint/limit type definitions that can be applied to UI components.

#### Limits Columns

| Column | Type | Constraints | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY, AUTOINCREMENT | Unique identifier for the limit type |
| `key` | TEXT | UNIQUE, NOT NULL | Limit type identifier |

#### Limits Default Data

| id | key |
| :--- | :--- |
| 1 | MIN_VALUE |
| 2 | MAX_VALUE |
| 3 | DEFAULT_VALUE |
| 4 | STEP |
| 5 | MAX_CHAR |

#### Limit Type Descriptions

- **MIN_VALUE** - Minimum allowed value (for numeric/date/time inputs)
- **MAX_VALUE** - Maximum allowed value (for numeric/date/time inputs)
- **DEFAULT_VALUE** - Default value when component is initialized
- **STEP** - Increment/decrement step size (for numeric inputs)
- **MAX_CHAR** - Maximum character length (for text inputs)

---

## Usage Examples

### Query All Component Types

```sql
SELECT * FROM Component ORDER BY id;
```

**Result:**

```text
id | type      | key
---|-----------|-----
1  | ComboBox  | cb
2  | TextField | tf
3  | Decimal   | dec
4  | Integer   | int
5  | Date      | date
6  | Time      | time
7  | DateTime  | dt
```

### Query All Limit Types

```sql
SELECT * FROM Limits ORDER BY id;
```

**Result:**

```text
id | key
---|-------------
1  | MIN_VALUE
2  | MAX_VALUE
3  | DEFAULT_VALUE
4  | STEP
5  | MAX_CHAR
```

### Find Component by Key

```sql
SELECT * FROM Component WHERE key = 'int';
```

**Result:**

```text
id | type    | key
---|---------|-----
4  | Integer | int
```

### Find Limit by Key

```sql
SELECT * FROM Limits WHERE key = 'MAX_VALUE';
```

**Result:**

```text
id | key
---|----------
2  | MAX_VALUE
```

---

## Feature Layout and Limits

These tables define how UI components are mapped to features and what specific limits are applied to them.

### 3. FeatureLayout Entity

Links features to their UI component types.

#### FeatureLayout Schema

```sql
CREATE TABLE IF NOT EXISTS FeatureLayout (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    feature_id INTEGER NOT NULL,
    component_id INTEGER NOT NULL,
    FOREIGN KEY (feature_id) REFERENCES Features(id) ON DELETE CASCADE,
    FOREIGN KEY (component_id) REFERENCES Component(id) ON DELETE CASCADE
);
```

#### FeatureLayout Columns

| Column | Type | Constraints | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY, AUTOINCREMENT | Unique identifier for the layout mapping |
| `feature_id` | INTEGER | NOT NULL, FK(Features.id) | Reference to the feature |
| `component_id` | INTEGER | NOT NULL, FK(Component.id) | Reference to the component type |

---

### 4. FeatureLayoutLimits Entity

Stores specific limit values for a feature's component.

#### FeatureLayoutLimits Schema

```sql
CREATE TABLE IF NOT EXISTS FeatureLayoutLimits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    layout_id INTEGER NOT NULL,
    limit_id INTEGER NOT NULL,
    value TEXT NOT NULL,
    FOREIGN KEY (layout_id) REFERENCES FeatureLayout(id) ON DELETE CASCADE,
    FOREIGN KEY (limit_id) REFERENCES Limits(id) ON DELETE CASCADE
);
```

#### FeatureLayoutLimits Columns

| Column | Type | Constraints | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY, AUTOINCREMENT | Unique identifier for the limit value |
| `layout_id` | INTEGER | NOT NULL, FK(FeatureLayout.id) | Reference to the layout mapping |
| `limit_id` | INTEGER | NOT NULL, FK(Limits.id) | Reference to the limit type |
| `value` | TEXT | NOT NULL | The actual limit value (e.g., '100', '0.0', '10') |

---

## Integration with Existing Schema

The full hierarchy of the layout system is as follows:

```text
Features
    └── FeatureLayout
            ├── Component (Type of control)
            └── FeatureLayoutLimits
                    └── Limits (Type of constraint)
```

**Example Usage:**

```sql
-- 1. Map Feature #1 (e.g., 'Overcurrent Threshold') to an 'Integer' component
INSERT INTO FeatureLayout (feature_id, component_id) VALUES (1, 4); -- 4 is Integer

-- 2. Set min/max values for that layout (assuming layout_id = 1)
INSERT INTO FeatureLayoutLimits (layout_id, limit_id, value)
VALUES 
    (1, 1, '0'),      -- MIN_VALUE = 0
    (1, 2, '5000'),   -- MAX_VALUE = 5000
    (1, 3, '1000');   -- DEFAULT_VALUE = 1000
```

---

## C++ Integration

### Example: Loading Component Types

```cpp
#include <sqlite3.h>
#include <vector>
#include <string>

struct ComponentType {
    int id;
    std::string type;
    std::string key;
};

std::vector<ComponentType> loadComponentTypes(sqlite3* db) {
    std::vector<ComponentType> components;
    const char* sql = "SELECT id, type, key FROM Component ORDER BY id;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ComponentType comp;
            comp.id = sqlite3_column_int(stmt, 0);
            comp.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            comp.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            components.push_back(comp);
        }
        sqlite3_finalize(stmt);
    }
    
    return components;
}
```

### Example: Loading Limit Types

```cpp
struct LimitType {
    int id;
    std::string key;
};

std::vector<LimitType> loadLimitTypes(sqlite3* db) {
    std::vector<LimitType> limits;
    const char* sql = "SELECT id, key FROM Limits ORDER BY id;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LimitType limit;
            limit.id = sqlite3_column_int(stmt, 0);
            limit.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            limits.push_back(limit);
        }
        sqlite3_finalize(stmt);
    }
    
    return limits;
}
```

### Example: Finding Component by Key

```cpp
std::optional<ComponentType> findComponentByKey(sqlite3* db, const std::string& key) {
    const char* sql = "SELECT id, type, key FROM Component WHERE key = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ComponentType comp;
            comp.id = sqlite3_column_int(stmt, 0);
            comp.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            comp.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            sqlite3_finalize(stmt);
            return comp;
        }
        sqlite3_finalize(stmt);
    }
    
    return std::nullopt;
}
```

---

## Adding New Component Types

To add new component types, insert them into the Component table:

```sql
INSERT INTO Component (type, key) VALUES ('CheckBox', 'chk');
INSERT INTO Component (type, key) VALUES ('RadioButton', 'rb');
INSERT INTO Component (type, key) VALUES ('Slider', 'slider');
INSERT INTO Component (type, key) VALUES ('ColorPicker', 'color');
```

---

## Adding New Limit Types

To add new limit types, insert them into the Limits table:

```sql
INSERT INTO Limits (key) VALUES ('MIN_LENGTH');
INSERT INTO Limits (key) VALUES ('MAX_LENGTH');
INSERT INTO Limits (key) VALUES ('PATTERN');
INSERT INTO Limits (key) VALUES ('REQUIRED');
INSERT INTO Limits (key) VALUES ('READONLY');
```

---

## Database Initialization

The tables are automatically created and populated when the application initializes using the `INITIALIZATION_SQL` vector in `DatabaseSchema.hpp`:

```cpp
#include "DatabaseSchema.hpp"

void initializeDatabase(sqlite3* db) {
    for (const auto& sql : Schema::INITIALIZATION_SQL) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
}
```

---

## Benefits

1. **Dynamic UI Generation** - Component types can be used to automatically generate appropriate UI controls
2. **Validation** - Limits provide constraint information for input validation
3. **Extensibility** - Easy to add new component types and limit types without code changes
4. **Consistency** - Centralized definition of UI component types across the application
5. **Database-Driven** - UI behavior can be modified by changing database content

---

## Notes

- All INSERT statements use `INSERT OR IGNORE` to prevent duplicate entries on re-initialization
- The `key` field in both tables is UNIQUE to ensure no duplicates
- Component types are intentionally simple and can be extended as needed
- Limit types are generic and can apply to different component types as appropriate

---

## Related Files

- **Schema Definition**: `include/DatabaseSchema.hpp`
- **Database Manager**: `include/RecloserManager.hpp`
- **Implementation**: `src/RecloserManager.cpp`

---

**Last Updated:** 2026-02-02
