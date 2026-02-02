-- ============================================================================
-- UI Component and Limits Tables - SQL Reference
-- ============================================================================
-- This file contains useful SQL queries for working with Component and Limits tables
-- ============================================================================

-- ----------------------------------------------------------------------------
-- BASIC QUERIES
-- ----------------------------------------------------------------------------

-- View all component types
SELECT * FROM Component ORDER BY id;

-- View all limit types
SELECT * FROM Limits ORDER BY id;

-- Count component types
SELECT COUNT(*) as total_components FROM Component;

-- Count limit types
SELECT COUNT(*) as total_limits FROM Limits;

-- ----------------------------------------------------------------------------
-- SEARCH QUERIES
-- ----------------------------------------------------------------------------

-- Find component by key
SELECT * FROM Component WHERE key = 'int';

-- Find component by type
SELECT * FROM Component WHERE type = 'Integer';

-- Find limit by key
SELECT * FROM Limits WHERE key = 'MAX_VALUE';

-- Search components by partial type name
SELECT * FROM Component WHERE type LIKE '%Box%';

-- ----------------------------------------------------------------------------
-- VALIDATION QUERIES
-- ----------------------------------------------------------------------------

-- Check if a component key exists
SELECT EXISTS(SELECT 1 FROM Component WHERE key = 'cb') as exists;

-- Check if a limit key exists
SELECT EXISTS(SELECT 1 FROM Limits WHERE key = 'MIN_VALUE') as exists;

-- Find duplicate keys in Component (should return empty)
SELECT key, COUNT(*) as count 
FROM Component 
GROUP BY key 
HAVING count > 1;

-- Find duplicate keys in Limits (should return empty)
SELECT key, COUNT(*) as count 
FROM Limits 
GROUP BY key 
HAVING count > 1;

-- ----------------------------------------------------------------------------
-- FORMATTED OUTPUT
-- ----------------------------------------------------------------------------

-- Component types with formatted output
SELECT 
    printf('%2d', id) as ID,
    printf('%-12s', type) as Type,
    printf('%-6s', key) as Key
FROM Component
ORDER BY id;

-- Limit types with formatted output
SELECT 
    printf('%2d', id) as ID,
    printf('%-15s', key) as Key
FROM Limits
ORDER BY id;

-- ----------------------------------------------------------------------------
-- ADDING NEW DATA
-- ----------------------------------------------------------------------------

-- Add a new component type (example)
-- INSERT INTO Component (type, key) VALUES ('CheckBox', 'chk');
-- INSERT INTO Component (type, key) VALUES ('RadioButton', 'rb');
-- INSERT INTO Component (type, key) VALUES ('Slider', 'slider');

-- Add a new limit type (example)
-- INSERT INTO Limits (key) VALUES ('MIN_LENGTH');
-- INSERT INTO Limits (key) VALUES ('PATTERN');
-- INSERT INTO Limits (key) VALUES ('REQUIRED');

-- ----------------------------------------------------------------------------
-- UPDATING DATA
-- ----------------------------------------------------------------------------

-- Update component type name (example)
-- UPDATE Component SET type = 'DropDown' WHERE key = 'cb';

-- Update limit key (example)
-- UPDATE Limits SET key = 'MINIMUM_VALUE' WHERE key = 'MIN_VALUE';

-- ----------------------------------------------------------------------------
-- DELETING DATA
-- ----------------------------------------------------------------------------

-- Delete a component type (example - use with caution)
-- DELETE FROM Component WHERE key = 'chk';

-- Delete a limit type (example - use with caution)
-- DELETE FROM Limits WHERE key = 'PATTERN';

-- ----------------------------------------------------------------------------
-- EXPORT QUERIES
-- ----------------------------------------------------------------------------

-- Export component types as INSERT statements
SELECT 
    'INSERT INTO Component (type, key) VALUES (''' || type || ''', ''' || key || ''');' as sql_insert
FROM Component
ORDER BY id;

-- Export limit types as INSERT statements
SELECT 
    'INSERT INTO Limits (key) VALUES (''' || key || ''');' as sql_insert
FROM Limits
ORDER BY id;

-- ----------------------------------------------------------------------------
-- STATISTICS
-- ----------------------------------------------------------------------------

-- Component statistics
SELECT 
    'Total Components' as metric,
    COUNT(*) as value
FROM Component
UNION ALL
SELECT 
    'Shortest Key Length',
    MIN(LENGTH(key))
FROM Component
UNION ALL
SELECT 
    'Longest Key Length',
    MAX(LENGTH(key))
FROM Component
UNION ALL
SELECT 
    'Average Key Length',
    CAST(AVG(LENGTH(key)) AS INTEGER)
FROM Component;

-- Limit statistics
SELECT 
    'Total Limits' as metric,
    COUNT(*) as value
FROM Limits
UNION ALL
SELECT 
    'Shortest Key Length',
    MIN(LENGTH(key))
FROM Limits
UNION ALL
SELECT 
    'Longest Key Length',
    MAX(LENGTH(key))
FROM Limits
UNION ALL
SELECT 
    'Average Key Length',
    CAST(AVG(LENGTH(key)) AS INTEGER)
FROM Limits;

-- ----------------------------------------------------------------------------
-- FEATURE LAYOUT AND LIMITS QUERIES
-- ----------------------------------------------------------------------------

-- View all layouts with feature names and component types
SELECT 
    fl.id as layout_id,
    f.id as feature_id,
    c.type as component_type,
    c.key as component_key
FROM FeatureLayout fl
JOIN Features f ON fl.feature_id = f.id
JOIN Component c ON fl.component_id = c.id
ORDER BY fl.id;

-- View all limits applied to layouts
SELECT 
    fll.id as limit_id,
    fl.id as layout_id,
    l.key as limit_type,
    fll.value
FROM FeatureLayoutLimits fll
JOIN FeatureLayout fl ON fll.layout_id = fl.id
JOIN Limits l ON fll.limit_id = l.id
ORDER BY fll.layout_id;

-- Detailed view: Feature -> Component -> Limits
SELECT 
    f.id as feature_id,
    c.type as component,
    l.key as constraint_type,
    fll.value
FROM Features f
JOIN FeatureLayout fl ON f.id = fl.feature_id
JOIN Component c ON fl.component_id = c.id
LEFT JOIN FeatureLayoutLimits fll ON fl.id = fll.layout_id
LEFT JOIN Limits l ON fll.limit_id = l.id
ORDER BY f.id, l.key;

-- ----------------------------------------------------------------------------
-- DATA INSERTION EXAMPLES
-- ----------------------------------------------------------------------------

-- Example: Map Feature #1 to a ComboBox
-- INSERT INTO FeatureLayout (feature_id, component_id) VALUES (1, 1);

-- Example: Set limits for Layout #1
-- INSERT INTO FeatureLayoutLimits (layout_id, limit_id, value)
-- VALUES 
--     (1, 1, '0'),      -- MIN_VALUE
--     (1, 2, '255');    -- MAX_VALUE

-- ----------------------------------------------------------------------------
-- TESTING QUERIES
-- ----------------------------------------------------------------------------

-- Verify all default component types exist
SELECT 
    CASE 
        WHEN COUNT(*) = 7 THEN 'PASS: All 7 component types exist'
        ELSE 'FAIL: Expected 7 component types, found ' || COUNT(*)
    END as test_result
FROM Component;

-- Verify all default limit types exist
SELECT 
    CASE 
        WHEN COUNT(*) = 5 THEN 'PASS: All 5 limit types exist'
        ELSE 'FAIL: Expected 5 limit types, found ' || COUNT(*)
    END as test_result
FROM Limits;

-- Verify no NULL values in Component
SELECT 
    CASE 
        WHEN COUNT(*) = 0 THEN 'PASS: No NULL values in Component'
        ELSE 'FAIL: Found ' || COUNT(*) || ' NULL values in Component'
    END as test_result
FROM Component
WHERE type IS NULL OR key IS NULL;

-- Verify no NULL values in Limits
SELECT 
    CASE 
        WHEN COUNT(*) = 0 THEN 'PASS: No NULL values in Limits'
        ELSE 'FAIL: Found ' || COUNT(*) || ' NULL values in Limits'
    END as test_result
FROM Limits
WHERE key IS NULL;

-- ----------------------------------------------------------------------------
-- UTILITY QUERIES
-- ----------------------------------------------------------------------------

-- Get table schema
PRAGMA table_info(Component);
PRAGMA table_info(Limits);

-- Check indexes
SELECT * FROM sqlite_master WHERE type = 'index' AND tbl_name = 'Component';
SELECT * FROM sqlite_master WHERE type = 'index' AND tbl_name = 'Limits';

-- Database integrity check
PRAGMA integrity_check;

-- ----------------------------------------------------------------------------
-- END OF FILE
-- ----------------------------------------------------------------------------
