#ifndef DATABASE_SCHEMA_H
#define DATABASE_SCHEMA_H

#if defined(DATABASE_SCHEMA_POSTGRESQL)
#define DATABASE_SCHEMA_AUTO_PRIMARY_KEY "SERIAL PRIMARY KEY"
#define DATABASE_SCHEMA_TIMESTAMP_TYPE "TIMESTAMP"
#define DATABASE_SCHEMA_TABLE_OPTIONS ""
#else
#define DATABASE_SCHEMA_AUTO_PRIMARY_KEY "INT NOT NULL AUTO_INCREMENT PRIMARY KEY"
#define DATABASE_SCHEMA_TIMESTAMP_TYPE "DATETIME"
#define DATABASE_SCHEMA_TABLE_OPTIONS " ENGINE=InnoDB DEFAULT CHARSET=latin1"
#endif

static const char *const DatabaseSchemaStatements[] = {
   "CREATE TABLE IF NOT EXISTS money_total ("
   "money_total_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "money_total_amount VARCHAR(18) NOT NULL,"
   "PRIMARY KEY (money_total_time, money_total_amount))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS money_created ("
   "money_created_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "money_created_amount INT NOT NULL,"
   "PRIMARY KEY (money_created_time, money_created_amount))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_logins ("
   "plogin_account_name VARCHAR(45) NOT NULL,"
   "plogin_character_name VARCHAR(45) NOT NULL,"
   "plogin_IP VARCHAR(45) NOT NULL,"
   "plogin_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "PRIMARY KEY (plogin_account_name, plogin_time))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_damaged ("
   "idpdamaged " DATABASE_SCHEMA_AUTO_PRIMARY_KEY ","
   "pdamaged_who VARCHAR(45) NOT NULL,"
   "pdamaged_attacker VARCHAR(45) NOT NULL,"
   "pdamaged_aspell INT NOT NULL,"
   "pdamaged_atype INT NOT NULL,"
   "pdamaged_applied INT NOT NULL,"
   "pdamaged_original INT NOT NULL,"
   "pdamaged_weapon VARCHAR(45) NOT NULL,"
   "pdamaged_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_death ("
   "pdeath_victim VARCHAR(45) NOT NULL,"
   "pdeath_killer VARCHAR(45) NOT NULL,"
   "pdeath_room VARCHAR(63) NOT NULL,"
   "pdeath_attack VARCHAR(45) NOT NULL,"
   "pdeath_ispvp INT NOT NULL,"
   "pdeath_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "PRIMARY KEY (pdeath_victim, pdeath_time))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player ("
   "player_account_id INT NOT NULL,"
   "player_name VARCHAR(45) NOT NULL PRIMARY KEY,"
   "player_home VARCHAR(128) DEFAULT NULL,"
   "player_bind VARCHAR(128) DEFAULT NULL,"
   "player_guild VARCHAR(45) DEFAULT NULL,"
   "player_max_health INT DEFAULT NULL,"
   "player_max_mana INT DEFAULT NULL,"
   "player_might INT DEFAULT NULL,"
   "player_int INT DEFAULT NULL,"
   "player_myst INT DEFAULT NULL,"
   "player_stam INT DEFAULT NULL,"
   "player_agil INT DEFAULT NULL,"
   "player_aim INT DEFAULT NULL,"
   "player_suicide INT DEFAULT 0,"
   "player_suicide_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS guild ("
   "guild_name VARCHAR(100) NOT NULL PRIMARY KEY,"
   "guild_leader VARCHAR(45) NOT NULL,"
   "guild_hall VARCHAR(100) DEFAULT NULL,"
   "guild_rent_paid INT NOT NULL,"
   "guild_disbanded INT NOT NULL DEFAULT 0,"
   "guild_disbanded_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_spells ("
   "spell_id INT NOT NULL PRIMARY KEY,"
   "spell_name VARCHAR(63) NOT NULL,"
   "spell_name_ger VARCHAR(63) NOT NULL,"
   "spell_icon VARCHAR(63) NOT NULL,"
   "spell_desc TEXT DEFAULT NULL,"
   "spell_desc_ger TEXT DEFAULT NULL,"
   "spell_school INT DEFAULT NULL,"
   "spell_level INT DEFAULT NULL,"
   "spell_mana INT DEFAULT NULL,"
   "spell_chance INT DEFAULT NULL,"
   "spell_mediate_ratio INT DEFAULT NULL,"
   "spell_exertion INT DEFAULT NULL,"
   "spell_casttime INT DEFAULT NULL,"
   "spell_iflag INT NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_skills ("
   "skill_id INT NOT NULL PRIMARY KEY,"
   "skill_name VARCHAR(63) NOT NULL,"
   "skill_name_ger VARCHAR(63) NOT NULL,"
   "skill_icon VARCHAR(63) NOT NULL,"
   "skill_desc TEXT DEFAULT NULL,"
   "skill_desc_ger TEXT DEFAULT NULL,"
   "skill_school INT DEFAULT NULL,"
   "skill_level INT DEFAULT NULL,"
   "skill_chance INT DEFAULT NULL,"
   "skill_mediate_ratio INT DEFAULT NULL,"
   "skill_exertion INT DEFAULT NULL,"
   "skill_iflag INT NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_spell_reagent ("
   "spell_id INT NOT NULL,"
   "spell_reagent VARCHAR(63) NOT NULL,"
   "spell_reagent_amount INT DEFAULT NULL,"
   "PRIMARY KEY (spell_id, spell_reagent))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_treasure_gen ("
   "treasure_id INT NOT NULL,"
   "item_name VARCHAR(63) NOT NULL,"
   "item_chance INT DEFAULT NULL,"
   "PRIMARY KEY (treasure_id, item_name))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_treasure_extra ("
   "treasure_id INT NOT NULL,"
   "item_name VARCHAR(63) NOT NULL,"
   "item_min_amount INT DEFAULT NULL,"
   "item_max_amount INT DEFAULT NULL,"
   "PRIMARY KEY (treasure_id, item_name))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_treasure_magic ("
   "treasure_id INT NOT NULL,"
   "item_name VARCHAR(63) NOT NULL,"
   "item_attribute_id INT NOT NULL,"
   "PRIMARY KEY (treasure_id, item_name, item_attribute_id))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_monster ("
   "monster_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "monster_name_ger VARCHAR(63) NOT NULL,"
   "monster_icon VARCHAR(63) NOT NULL,"
   "monster_desc TEXT DEFAULT NULL,"
   "monster_desc_ger TEXT DEFAULT NULL,"
   "monster_level INT DEFAULT NULL,"
   "monster_karma INT DEFAULT NULL,"
   "monster_treasure INT DEFAULT NULL,"
   "monster_speed INT DEFAULT NULL,"
   "monster_behavior INT DEFAULT NULL,"
   "monster_difficulty INT DEFAULT NULL,"
   "monster_visiondistance INT DEFAULT NULL,"
   "monster_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_monster_zone ("
   "monster_rid INT NOT NULL,"
   "monster_name VARCHAR(63) NOT NULL,"
   "monster_spawnchance INT DEFAULT NULL,"
   "PRIMARY KEY (monster_rid, monster_name))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_npcs ("
   "npc_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "npc_name_ger VARCHAR(63) NOT NULL,"
   "npc_icon VARCHAR(63) NOT NULL,"
   "npc_desc TEXT DEFAULT NULL,"
   "npc_desc_ger TEXT DEFAULT NULL,"
   "npc_merchantmarkup INT DEFAULT NULL,"
   "npc_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_weapons ("
   "weapon_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "weapon_name_ger VARCHAR(63) NOT NULL,"
   "weapon_icon VARCHAR(63) NOT NULL,"
   "weapon_group INT NOT NULL,"
   "weapon_color INT NOT NULL,"
   "weapon_desc TEXT DEFAULT NULL,"
   "weapon_desc_ger TEXT DEFAULT NULL,"
   "weapon_value INT DEFAULT NULL,"
   "weapon_weight INT DEFAULT NULL,"
   "weapon_bulk INT DEFAULT NULL,"
   "weapon_range INT DEFAULT NULL,"
   "weapon_skill INT DEFAULT NULL,"
   "weapon_prof INT DEFAULT NULL,"
   "weapon_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_rooms ("
   "room_name VARCHAR(63) NOT NULL,"
   "room_name_ger VARCHAR(63) NOT NULL,"
   "room_roo VARCHAR(63) NOT NULL,"
   "room_number INT NOT NULL PRIMARY KEY,"
   "room_region INT NOT NULL,"
   "room_iflag INT NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_npc_zone ("
   "npc_name VARCHAR(63) NOT NULL,"
   "npc_roomid INT NOT NULL,"
   "npc_row INT NOT NULL,"
   "npc_col INT NOT NULL,"
   "PRIMARY KEY (npc_name, npc_roomid, npc_row, npc_col))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_npc_sellitem ("
   "npc_name VARCHAR(63) NOT NULL,"
   "npc_item_sold VARCHAR(63) NOT NULL,"
   "item_color INT NOT NULL,"
   "PRIMARY KEY (npc_name, npc_item_sold, item_color))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_npc_sellskill ("
   "npc_name VARCHAR(63) NOT NULL,"
   "npc_skill_id INT NOT NULL,"
   "PRIMARY KEY (npc_name, npc_skill_id))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_npc_sellspell ("
   "npc_name VARCHAR(63) NOT NULL,"
   "npc_spell_id INT NOT NULL,"
   "PRIMARY KEY (npc_name, npc_spell_id))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_npc_sellcond ("
   "npc_name VARCHAR(63) NOT NULL,"
   "npc_item_sold VARCHAR(63) NOT NULL,"
   "item_color INT NOT NULL,"
   "item_price INT NOT NULL,"
   "PRIMARY KEY (npc_name, npc_item_sold, item_color))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_reagents ("
   "reagent_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "reagent_name_ger VARCHAR(63) NOT NULL,"
   "reagent_icon VARCHAR(63) NOT NULL,"
   "reagent_group INT NOT NULL,"
   "reagent_color INT NOT NULL,"
   "reagent_desc TEXT DEFAULT NULL,"
   "reagent_desc_ger TEXT DEFAULT NULL,"
   "reagent_value INT DEFAULT NULL,"
   "reagent_weight INT DEFAULT NULL,"
   "reagent_bulk INT DEFAULT NULL,"
   "reagent_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_food ("
   "food_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "food_name_ger VARCHAR(63) NOT NULL,"
   "food_icon VARCHAR(63) NOT NULL,"
   "food_group INT NOT NULL,"
   "food_color INT NOT NULL,"
   "food_desc TEXT DEFAULT NULL,"
   "food_desc_ger TEXT DEFAULT NULL,"
   "food_value INT DEFAULT NULL,"
   "food_weight INT DEFAULT NULL,"
   "food_bulk INT DEFAULT NULL,"
   "food_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_ammo ("
   "ammo_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "ammo_name_ger VARCHAR(63) NOT NULL,"
   "ammo_icon VARCHAR(63) NOT NULL,"
   "ammo_group INT NOT NULL,"
   "ammo_color INT NOT NULL,"
   "ammo_desc TEXT DEFAULT NULL,"
   "ammo_desc_ger TEXT DEFAULT NULL,"
   "ammo_value INT DEFAULT NULL,"
   "ammo_weight INT DEFAULT NULL,"
   "ammo_bulk INT DEFAULT NULL,"
   "ammo_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_armor ("
   "armor_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "armor_name_ger VARCHAR(63) NOT NULL,"
   "armor_icon VARCHAR(63) NOT NULL,"
   "armor_group INT NOT NULL,"
   "armor_color INT NOT NULL,"
   "armor_desc TEXT DEFAULT NULL,"
   "armor_desc_ger TEXT DEFAULT NULL,"
   "armor_value INT DEFAULT NULL,"
   "armor_weight INT DEFAULT NULL,"
   "armor_bulk INT DEFAULT NULL,"
   "armor_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_miscitems ("
   "misc_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "misc_name_ger VARCHAR(63) NOT NULL,"
   "misc_icon VARCHAR(63) NOT NULL,"
   "misc_group INT NOT NULL,"
   "misc_color INT NOT NULL,"
   "misc_desc TEXT DEFAULT NULL,"
   "misc_desc_ger TEXT DEFAULT NULL,"
   "misc_value INT DEFAULT NULL,"
   "misc_weight INT DEFAULT NULL,"
   "misc_bulk INT DEFAULT NULL,"
   "misc_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_rings ("
   "rings_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "rings_name_ger VARCHAR(63) NOT NULL,"
   "rings_icon VARCHAR(63) NOT NULL,"
   "rings_group INT NOT NULL,"
   "rings_color INT NOT NULL,"
   "rings_desc TEXT DEFAULT NULL,"
   "rings_desc_ger TEXT DEFAULT NULL,"
   "rings_value INT DEFAULT NULL,"
   "rings_weight INT DEFAULT NULL,"
   "rings_bulk INT DEFAULT NULL,"
   "rings_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_rods ("
   "rods_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "rods_name_ger VARCHAR(63) NOT NULL,"
   "rods_icon VARCHAR(63) NOT NULL,"
   "rods_group INT NOT NULL,"
   "rods_color INT NOT NULL,"
   "rods_desc TEXT DEFAULT NULL,"
   "rods_desc_ger TEXT DEFAULT NULL,"
   "rods_value INT DEFAULT NULL,"
   "rods_weight INT DEFAULT NULL,"
   "rods_bulk INT DEFAULT NULL,"
   "rods_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_potions ("
   "potion_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "potion_name_ger VARCHAR(63) NOT NULL,"
   "potion_icon VARCHAR(63) NOT NULL,"
   "potion_group INT NOT NULL,"
   "potion_color INT NOT NULL,"
   "potion_desc TEXT DEFAULT NULL,"
   "potion_desc_ger TEXT DEFAULT NULL,"
   "potion_value INT DEFAULT NULL,"
   "potion_weight INT DEFAULT NULL,"
   "potion_bulk INT DEFAULT NULL,"
   "potion_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_scrolls ("
   "scrolls_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "scrolls_name_ger VARCHAR(63) NOT NULL,"
   "scrolls_icon VARCHAR(63) NOT NULL,"
   "scrolls_group INT NOT NULL,"
   "scrolls_color INT NOT NULL,"
   "scrolls_desc TEXT DEFAULT NULL,"
   "scrolls_desc_ger TEXT DEFAULT NULL,"
   "scrolls_value INT DEFAULT NULL,"
   "scrolls_weight INT DEFAULT NULL,"
   "scrolls_bulk INT DEFAULT NULL,"
   "scrolls_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_wands ("
   "wands_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "wands_name_ger VARCHAR(63) NOT NULL,"
   "wands_icon VARCHAR(63) NOT NULL,"
   "wands_group INT NOT NULL,"
   "wands_color INT NOT NULL,"
   "wands_desc TEXT DEFAULT NULL,"
   "wands_desc_ger TEXT DEFAULT NULL,"
   "wands_value INT DEFAULT NULL,"
   "wands_weight INT DEFAULT NULL,"
   "wands_bulk INT DEFAULT NULL,"
   "wands_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_questitems ("
   "questitem_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "questitem_name_ger VARCHAR(63) NOT NULL,"
   "questitem_icon VARCHAR(63) NOT NULL,"
   "questitem_group INT NOT NULL,"
   "questitem_color INT NOT NULL,"
   "questitem_desc TEXT DEFAULT NULL,"
   "questitem_desc_ger TEXT DEFAULT NULL,"
   "questitem_value INT DEFAULT NULL,"
   "questitem_weight INT DEFAULT NULL,"
   "questitem_bulk INT DEFAULT NULL,"
   "questitem_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_necklace ("
   "necklace_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "necklace_name_ger VARCHAR(63) NOT NULL,"
   "necklace_icon VARCHAR(63) NOT NULL,"
   "necklace_group INT NOT NULL,"
   "necklace_color INT NOT NULL,"
   "necklace_desc TEXT DEFAULT NULL,"
   "necklace_desc_ger TEXT DEFAULT NULL,"
   "necklace_value INT DEFAULT NULL,"
   "necklace_weight INT DEFAULT NULL,"
   "necklace_bulk INT DEFAULT NULL,"
   "necklace_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_instruments ("
   "instrument_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "instrument_name_ger VARCHAR(63) NOT NULL,"
   "instrument_icon VARCHAR(63) NOT NULL,"
   "instrument_group INT NOT NULL,"
   "instrument_color INT NOT NULL,"
   "instrument_desc TEXT DEFAULT NULL,"
   "instrument_desc_ger TEXT DEFAULT NULL,"
   "instrument_value INT DEFAULT NULL,"
   "instrument_weight INT DEFAULT NULL,"
   "instrument_bulk INT DEFAULT NULL,"
   "instrument_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_gems ("
   "gem_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "gem_name_ger VARCHAR(63) NOT NULL,"
   "gem_icon VARCHAR(63) NOT NULL,"
   "gem_group INT NOT NULL,"
   "gem_color INT NOT NULL,"
   "gem_desc TEXT DEFAULT NULL,"
   "gem_desc_ger TEXT DEFAULT NULL,"
   "gem_value INT DEFAULT NULL,"
   "gem_weight INT DEFAULT NULL,"
   "gem_bulk INT DEFAULT NULL,"
   "gem_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_offerings ("
   "offering_name VARCHAR(63) NOT NULL PRIMARY KEY,"
   "offering_name_ger VARCHAR(63) NOT NULL,"
   "offering_icon VARCHAR(63) NOT NULL,"
   "offering_group INT NOT NULL,"
   "offering_color INT NOT NULL,"
   "offering_desc TEXT DEFAULT NULL,"
   "offering_desc_ger TEXT DEFAULT NULL,"
   "offering_value INT DEFAULT NULL,"
   "offering_weight INT DEFAULT NULL,"
   "offering_bulk INT DEFAULT NULL,"
   "offering_iflag INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_quests ("
   "quest_id INT NOT NULL PRIMARY KEY,"
   "quest_name VARCHAR(63) NOT NULL,"
   "quest_name_ger VARCHAR(63) NOT NULL,"
   "quest_icon VARCHAR(63) NOT NULL,"
   "quest_icon_group INT NOT NULL,"
   "quest_desc TEXT DEFAULT NULL,"
   "quest_desc_ger TEXT DEFAULT NULL,"
   "quest_recent_time INT DEFAULT NULL,"
   "quest_schedule_pct INT DEFAULT NULL,"
   "quest_est_time INT DEFAULT NULL,"
   "quest_est_diff INT DEFAULT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS wiki_quest_giver ("
   "quest_id INT NOT NULL,"
   "quest_npc_name VARCHAR(63) NOT NULL,"
   "PRIMARY KEY (quest_id, quest_npc_name))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS server_accounts ("
   "acct_id INT NOT NULL PRIMARY KEY,"
   "acct_name VARCHAR(63) NOT NULL,"
   "acct_password VARCHAR(63) NOT NULL,"
   "acct_email VARCHAR(255) NOT NULL,"
   "acct_type INT NOT NULL,"
   "acct_loggedin_time INT NOT NULL,"
   "acct_last_login INT NOT NULL,"
   "acct_suspend_time INT NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS server_account_chars ("
   "entry_id " DATABASE_SCHEMA_AUTO_PRIMARY_KEY ","
   "acct_id INT NOT NULL,"
   "char_name VARCHAR(63) NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_logpen ("
   "player_name VARCHAR(63) NOT NULL,"
   "logpen_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "room_id INT DEFAULT NULL,"
   "logpen_xp INT NOT NULL,"
   "logpen_hp INT NOT NULL,"
   "logpen_spellpct INT NOT NULL,"
   "logpen_skillpct INT NOT NULL,"
   "logpen_numitems INT NOT NULL,"
   "PRIMARY KEY (player_name, logpen_time))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_logpen_items ("
   "logpen_item_id " DATABASE_SCHEMA_AUTO_PRIMARY_KEY ","
   "player_name VARCHAR(63) NOT NULL,"
   "logpen_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "item_name VARCHAR(63) NOT NULL,"
   "item_number INT NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_completed_quests ("
   "quest_id INT NOT NULL,"
   "player_name VARCHAR(63) NOT NULL,"
   "completion_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "num_seconds_taken INT NOT NULL,"
   "xp_rewarded INT NOT NULL,"
   "tp_rewarded INT NOT NULL,"
   "PRIMARY KEY (quest_id, player_name, completion_time))"
   DATABASE_SCHEMA_TABLE_OPTIONS ";",

   "CREATE TABLE IF NOT EXISTS player_quest_reward_items ("
   "quest_rewarditem_id " DATABASE_SCHEMA_AUTO_PRIMARY_KEY ","
   "quest_id INT NOT NULL,"
   "player_name VARCHAR(63) NOT NULL,"
   "completion_time " DATABASE_SCHEMA_TIMESTAMP_TYPE " NOT NULL,"
   "item_name VARCHAR(63) NOT NULL,"
   "item_number INT NOT NULL)"
   DATABASE_SCHEMA_TABLE_OPTIONS ";"
};

#define DATABASE_SCHEMA_STATEMENT_COUNT \
   (sizeof(DatabaseSchemaStatements) / sizeof(DatabaseSchemaStatements[0]))

#undef DATABASE_SCHEMA_AUTO_PRIMARY_KEY
#undef DATABASE_SCHEMA_TIMESTAMP_TYPE
#undef DATABASE_SCHEMA_TABLE_OPTIONS

#endif
