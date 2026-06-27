USE portfolio;

DROP PROCEDURE IF EXISTS update_character_gold;

DELIMITER $$

CREATE PROCEDURE update_character_gold(
    IN p_id BIGINT UNSIGNED,
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