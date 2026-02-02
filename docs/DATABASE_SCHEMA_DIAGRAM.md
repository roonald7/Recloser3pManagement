# Database Schema - Entity Relationship Diagram

## Complete Schema Overview

```mermaid
erDiagram
    Languages ||--o{ Translations : "has"
    Descriptions ||--o{ Translations : "has"
    Descriptions ||--o{ Reclosers : "describes"
    Descriptions ||--o{ Services : "describes"
    Descriptions ||--o{ Features : "describes"
    Reclosers ||--o{ FirmwareVersions : "has"
    FirmwareVersions ||--o{ Services : "has"
    Services ||--o{ Services : "parent-child"
    Services ||--o{ Features : "has"
    Features ||--o{ FeatureLayout : "has"
    Component ||--o{ FeatureLayout : "is used in"
    FeatureLayout ||--o{ FeatureLayoutLimits : "has"
    Limits ||--o{ FeatureLayoutLimits : "constrains"

    Languages {
        TEXT code PK "Language code (e.g., 'en', 'pt')"
        TEXT name "Language name"
    }

    Descriptions {
        TEXT key PK "Description key"
    }

    Translations {
        INTEGER id PK "Auto-increment ID"
        TEXT description_key FK "References Descriptions"
        TEXT language_code FK "References Languages"
        TEXT value "Translated text"
    }

    Reclosers {
        INTEGER id PK "Auto-increment ID"
        TEXT description_key FK "References Descriptions"
        TEXT model "Recloser model"
    }

    FirmwareVersions {
        INTEGER id PK "Auto-increment ID"
        TEXT version "Firmware version"
        INTEGER recloser_id FK "References Reclosers"
    }

    Services {
        INTEGER id PK "Auto-increment ID"
        TEXT service_key "Unique service key"
        TEXT description_key FK "References Descriptions"
        INTEGER parent_id FK "References Services (self)"
        INTEGER firmware_id FK "References FirmwareVersions"
    }

    Features {
        INTEGER id PK "Auto-increment ID"
        TEXT description_key FK "References Descriptions"
        INTEGER service_id FK "References Services"
    }

    Component {
        INTEGER id PK "Auto-increment ID"
        TEXT type "Component type name"
        TEXT key "Short key (unique)"
    }

    Limits {
        INTEGER id PK "Auto-increment ID"
        TEXT key "Limit type key (unique)"
    }

    FeatureLayout {
        INTEGER id PK "Auto-increment ID"
        INTEGER feature_id FK "References Features"
        INTEGER component_id FK "References Component"
    }

    FeatureLayoutLimits {
        INTEGER id PK "Auto-increment ID"
        INTEGER layout_id FK "References FeatureLayout"
        INTEGER limit_id FK "References Limits"
        TEXT value "Constraint value"
    }
```

## UI Component Layout Tables

```mermaid
erDiagram
    Features ||--o{ FeatureLayout : "defines UI for"
    Component ||--o{ FeatureLayout : "is used in"
    FeatureLayout ||--o{ FeatureLayoutLimits : "has"
    Limits ||--o{ FeatureLayoutLimits : "defines"

    FeatureLayout {
        INTEGER id PK
        INTEGER feature_id FK
        INTEGER component_id FK
    }

    FeatureLayoutLimits {
        INTEGER id PK
        INTEGER layout_id FK
        INTEGER limit_id FK
        TEXT value
    }
```

## Table Relationships Summary

### Core Schema

- **Languages** → **Translations** (1:N)
- **Descriptions** → **Translations** (1:N)
- **Descriptions** → **Reclosers** (1:N)
- **Descriptions** → **Services** (1:N)
- **Descriptions** → **Features** (1:N)
- **Reclosers** → **FirmwareVersions** (1:N)
- **FirmwareVersions** → **Services** (1:N)
- **Services** → **Services** (1:N, self-referencing)
- **Services** → **Features** (1:N)

### UI Layout Schema

- **Features** → **FeatureLayout** (1:N)
- **Component** → **FeatureLayout** (1:N)
- **FeatureLayout** → **FeatureLayoutLimits** (1:N)
- **Limits** → **FeatureLayoutLimits** (1:N)

## SQL Schema Definition

### Component Table

```sql
CREATE TABLE IF NOT EXISTS Component (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type TEXT NOT NULL,
    key TEXT UNIQUE NOT NULL
);
```

### Limits Table

```sql
CREATE TABLE IF NOT EXISTS Limits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    key TEXT UNIQUE NOT NULL
);
```

## Default Data

### Component Types

| id | type | key |
| :--- | :--- | :--- |
| 1 | ComboBox | cb |
| 2 | TextField | tf |
| 3 | Decimal | dec |
| 4 | Integer | int |
| 5 | Date | date |
| 6 | Time | time |
| 7 | DateTime | dt |

### Limit Types

| id | key |
| :--- | :--- |
| 1 | MIN_VALUE |
| 2 | MAX_VALUE |
| 3 | DEFAULT_VALUE |
| 4 | STEP |
| 5 | MAX_CHAR |

---

**Generated:** 2026-02-02  
**Schema Version:** 1.1.0
