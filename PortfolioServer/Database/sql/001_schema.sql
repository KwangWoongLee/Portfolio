CREATE DATABASE IF NOT EXISTS portfolio
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_0900_ai_ci;

USE portfolio;

CREATE TABLE IF NOT EXISTS characters
(
    id BIGINT NOT NULL,
    name VARCHAR(32) NOT NULL,
    gold BIGINT NOT NULL DEFAULT 1000000,
    created_ut BIGINT NOT NULL DEFAULT (UNIX_TIMESTAMP()),
    PRIMARY KEY (id),
    UNIQUE KEY uk_characters_name (name),
    CONSTRAINT chk_characters_id CHECK (id > 0),
    CONSTRAINT chk_characters_gold CHECK (gold >= 0)
) ENGINE = InnoDB;
