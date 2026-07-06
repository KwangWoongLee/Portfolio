USE portfolio;

CREATE TABLE IF NOT EXISTS siege_reward_claim
(
    id BIGINT NOT NULL,
    siege_war_id BIGINT NOT NULL,
    world_id BIGINT NOT NULL,
    siege_war_type INT NOT NULL,
    character_id BIGINT NOT NULL,
    guild_id BIGINT NOT NULL,
    reward_type VARCHAR(32) NOT NULL,
    claim_state VARCHAR(32) NOT NULL DEFAULT 'ReadyToClaim',
    gold_amount BIGINT NOT NULL DEFAULT 0,
    created_ut BIGINT NOT NULL DEFAULT (UNIX_TIMESTAMP()),
    claimed_ut BIGINT NULL,
    updated_ut BIGINT NOT NULL DEFAULT (UNIX_TIMESTAMP()),
    PRIMARY KEY (id),
    UNIQUE KEY uk_siege_reward_once (siege_war_id, character_id, reward_type),
    KEY idx_siege_reward_character (character_id, claim_state),
    KEY idx_siege_reward_guild (guild_id, siege_war_id),
    KEY idx_siege_reward_state (claim_state, created_ut),
    CONSTRAINT chk_siege_reward_id CHECK (id > 0),
    CONSTRAINT chk_siege_reward_siege CHECK (siege_war_id > 0),
    CONSTRAINT chk_siege_reward_world CHECK (world_id > 0),
    CONSTRAINT chk_siege_reward_type_id CHECK (siege_war_type > 0),
    CONSTRAINT chk_siege_reward_character CHECK (character_id > 0),
    CONSTRAINT chk_siege_reward_guild CHECK (guild_id > 0),
    CONSTRAINT chk_siege_reward_gold CHECK (gold_amount >= 0),
    CONSTRAINT chk_siege_reward_type CHECK (reward_type IN ('SiegeWinnerGold')),
    CONSTRAINT chk_siege_reward_state CHECK (claim_state IN ('ReadyToClaim', 'Claimed', 'Failed'))
) ENGINE = InnoDB;