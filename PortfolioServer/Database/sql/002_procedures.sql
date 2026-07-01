USE portfolio;

DROP PROCEDURE IF EXISTS get_or_create_character;
DROP PROCEDURE IF EXISTS update_character_gold;

DELIMITER $$

CREATE PROCEDURE get_or_create_character(
    IN p_id BIGINT,
    IN p_name VARCHAR(32),
    IN p_initial_gold BIGINT)
BEGIN
    IF p_id <= 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'CHARACTER_ID_INVALID';
    END IF;

    IF p_name IS NULL OR CHAR_LENGTH(p_name) = 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'CHARACTER_NAME_INVALID';
    END IF;

    IF p_initial_gold < 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'CHARACTER_GOLD_NEGATIVE';
    END IF;

    INSERT IGNORE INTO characters(id, name, gold)
    VALUES(p_id, p_name, p_initial_gold);

    IF ROW_COUNT() = 0
       AND NOT EXISTS (SELECT 1 FROM characters WHERE id = p_id) THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'CHARACTER_CREATE_CONFLICT';
    END IF;

    SELECT id, name, gold
      FROM characters
     WHERE id = p_id;
END$$

CREATE PROCEDURE update_character_gold(
    IN p_id BIGINT,
    IN p_gold BIGINT)
BEGIN
    IF p_gold < 0 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'CHARACTER_GOLD_NEGATIVE';
    END IF;

    UPDATE characters
       SET gold = p_gold
     WHERE id = p_id;

    IF ROW_COUNT() = 0
       AND NOT EXISTS (SELECT 1 FROM characters WHERE id = p_id) THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'CHARACTER_NOT_FOUND';
    END IF;
END$$

DELIMITER ;
