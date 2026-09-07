CREATE TABLE IF NOT EXISTS wardrobe (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT,
    category    TEXT, 
    l           REAL NOT NULL,
    c           REAL NOT NULL,
    h           REAL NOT NULL,
    hex         TEXT,
    pattern     TEXT,
    formality   TEXT,
    fabric      TEXT,
    silhouette  TEXT,
    length      TEXT,
    warmth      INTEGER DEFAULT 3,
    clean       INTEGER DEFAULT 1,
    weight      REAL DEFAULT 1.0,
    last_worn   TEXT,
    cutout_path TEXT NOT NULL
);