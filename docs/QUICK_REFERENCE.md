# Component & Limits Tables - Quick Reference

## Table Structures

### Component

```
┌────┬───────────┬──────┐
│ id │   type    │ key  │
├────┼───────────┼──────┤
│ 1  │ ComboBox  │ cb   │
│ 2  │ TextField │ tf   │
│ 3  │ Decimal   │ dec  │
│ 4  │ Integer   │ int  │
│ 5  │ Date      │ date │
│ 6  │ Time      │ time │
│ 7  │ DateTime  │ dt   │
└────┴───────────┴──────┘
```

### Limits

```
┌────┬───────────────┐
│ id │      key      │
├────┼───────────────┤
│ 1  │ MIN_VALUE     │
│ 2  │ MAX_VALUE     │
│ 3  │ DEFAULT_VALUE │
│ 4  │ STEP          │
│ 5  │ MAX_CHAR      │
└────┴───────────────┘
```

---

## Common SQL Queries

```sql
-- Get all components
SELECT * FROM Component;

-- Get all limits
SELECT * FROM Limits;

-- Find component by key
SELECT * FROM Component WHERE key = 'int';

-- Find limit by key
SELECT * FROM Limits WHERE key = 'MAX_VALUE';

-- Check if component exists
SELECT EXISTS(SELECT 1 FROM Component WHERE key = 'cb');

-- Count components
SELECT COUNT(*) FROM Component;
```

---

## C++ Quick Examples

### Load All Components

```cpp
std::vector<ComponentType> loadComponentTypes(sqlite3* db) {
    std::vector<ComponentType> components;
    const char* sql = "SELECT id, type, key FROM Component;";
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

### Find Component by Key

```cpp
std::optional<ComponentType> findByKey(sqlite3* db, const std::string& key) {
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

## Component Type Mapping

| Key | Type | Use Case |
|-----|------|----------|
| `cb` | ComboBox | Dropdown selection from predefined options |
| `tf` | TextField | Single-line text input |
| `dec` | Decimal | Floating-point number input |
| `int` | Integer | Whole number input |
| `date` | Date | Date selection |
| `time` | Time | Time selection |
| `dt` | DateTime | Combined date and time selection |

---

## Limit Type Usage

| Key | Applies To | Description |
|-----|------------|-------------|
| `MIN_VALUE` | Decimal, Integer, Date, Time, DateTime | Minimum allowed value |
| `MAX_VALUE` | Decimal, Integer, Date, Time, DateTime | Maximum allowed value |
| `DEFAULT_VALUE` | All types | Initial/default value |
| `STEP` | Decimal, Integer | Increment/decrement step |
| `MAX_CHAR` | TextField | Maximum character length |

---

## Adding New Data

### New Component Type

```sql
INSERT INTO Component (type, key) VALUES ('CheckBox', 'chk');
```

### New Limit Type

```sql
INSERT INTO Limits (key) VALUES ('REQUIRED');
```

---

## Files Reference

| File | Purpose |
|------|---------|
| `include/DatabaseSchema.hpp` | Schema definition |
| `docs/UI_COMPONENT_SCHEMA.md` | Full documentation |
| `docs/DATABASE_SCHEMA_DIAGRAM.md` | ER diagrams |
| `docs/SQL_REFERENCE.sql` | SQL query examples |

---

**Quick Tip:** Use `INSERT OR IGNORE` to prevent duplicate entries when adding data.
