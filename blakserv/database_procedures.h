#ifndef DATABASE_PROCEDURES_H
#define DATABASE_PROCEDURES_H

typedef enum {
   DB_WRITE_INSERT,
   DB_WRITE_UPSERT,
   DB_WRITE_UPDATE
} database_write_mode;

typedef struct {
   sql_recordtype stat_type;
   int num_fields;
   bool enabled;
   const char *procedure_name;
   const char *table_name;
   const char *columns;
   const char *timestamp_column;
   int timestamp_position;
   database_write_mode write_mode;
   const char *conflict_columns;
   const char *update_columns;
   const char *update_assignments;
} sql_statistic_type;

// This is the shared contract between MySQL procedures and PostgreSQL writes.
// columns lists data arguments in their public parameter order. A timestamp
// column is generated at timestamp_position, which counts preceding arguments.
static sql_statistic_type Statistics_Table[] = {
   {STAT_NONE, 0, false, NULL, NULL, NULL, NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_TOTALMONEY, 1, true, "WriteTotalMoney", "money_total", "money_total_amount", "money_total_time", 0, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_MONEYCREATED, 1, true, "WriteMoneyCreated", "money_created", "money_created_amount", "money_created_time", 0, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_PLAYERLOGIN, 3, true, "WritePlayerLogin", "player_logins", "plogin_account_name,plogin_character_name,plogin_IP", "plogin_time", 3, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_ASSESS_DAM, 7, true, "WritePlayerAssessDamage", "player_damaged", "pdamaged_who,pdamaged_attacker,pdamaged_aspell,pdamaged_atype,pdamaged_applied,pdamaged_original,pdamaged_weapon", "pdamaged_time", 7, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_PLAYERDEATH, 5, true, "WritePlayerDeath", "player_death", "pdeath_victim,pdeath_killer,pdeath_room,pdeath_attack,pdeath_ispvp", "pdeath_time", 5, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_PLAYER, 13, true, "WritePlayer", "player", "player_account_id,player_name,player_home,player_bind,player_guild,player_max_health,player_max_mana,player_might,player_int,player_myst,player_stam,player_agil,player_aim", NULL, -1, DB_WRITE_UPSERT, "player_name", "player_account_id,player_home,player_bind,player_guild,player_max_health,player_max_mana,player_might,player_int,player_myst,player_stam,player_agil,player_aim", NULL},
   {STAT_PLAYERSUICIDE, 2, true, "WritePlayerSuicide", "player", "player_account_id,player_name", NULL, -1, DB_WRITE_UPDATE, NULL, NULL, "player_suicide = 1,player_suicide_time = CURRENT_TIMESTAMP"},
   {STAT_GUILD, 4, true, "WriteGuild", "guild", "guild_name,guild_leader,guild_hall,guild_rent_paid", NULL, -1, DB_WRITE_UPSERT, "guild_name", "guild_leader,guild_hall,guild_rent_paid", NULL},
   {STAT_GUILDDISBAND, 1, true, "WriteGuildDisband", "guild", "guild_name", NULL, -1, DB_WRITE_UPDATE, NULL, NULL, "guild_hall = '',guild_disbanded = 1,guild_disbanded_time = CURRENT_TIMESTAMP"},
   {STAT_SPELLS, 14, true, "WriteSpells", "wiki_spells", "spell_id,spell_name,spell_name_ger,spell_icon,spell_desc,spell_desc_ger,spell_school,spell_level,spell_mana,spell_chance,spell_mediate_ratio,spell_exertion,spell_casttime,spell_iflag", NULL, -1, DB_WRITE_UPSERT, "spell_id", "spell_name,spell_name_ger,spell_desc,spell_desc_ger,spell_school,spell_level,spell_mana,spell_chance,spell_mediate_ratio,spell_exertion,spell_casttime,spell_iflag", NULL},
   {STAT_SPELL_REAGENT, 3, true, "WriteSpellReagent", "wiki_spell_reagent", "spell_id,spell_reagent,spell_reagent_amount", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_TREASURE_GEN, 3, true, "WriteTreasureGen", "wiki_treasure_gen", "treasure_id,item_name,item_chance", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_MONSTER, 13, true, "WriteMonster", "wiki_monster", "monster_name,monster_name_ger,monster_icon,monster_desc,monster_desc_ger,monster_level,monster_karma,monster_treasure,monster_speed,monster_behavior,monster_difficulty,monster_visiondistance,monster_iflag", NULL, -1, DB_WRITE_UPSERT, "monster_name", "monster_desc,monster_desc_ger,monster_level,monster_karma,monster_treasure,monster_speed,monster_behavior,monster_difficulty,monster_visiondistance,monster_iflag", NULL},
   {STAT_MONSTER_ZONE, 3, true, "WriteMonsterZone", "wiki_monster_zone", "monster_rid,monster_name,monster_spawnchance", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_NPCS, 7, true, "WriteNpcs", "wiki_npcs", "npc_name,npc_name_ger,npc_icon,npc_desc,npc_desc_ger,npc_merchantmarkup,npc_iflag", NULL, -1, DB_WRITE_UPSERT, "npc_name", "npc_name_ger,npc_desc,npc_desc_ger,npc_merchantmarkup,npc_iflag", NULL},
   {STAT_WEAPONS, 14, true, "WriteWeapons", "wiki_weapons", "weapon_name,weapon_name_ger,weapon_icon,weapon_group,weapon_color,weapon_desc,weapon_desc_ger,weapon_value,weapon_weight,weapon_bulk,weapon_range,weapon_skill,weapon_prof,weapon_iflag", NULL, -1, DB_WRITE_UPSERT, "weapon_name", "weapon_group,weapon_color,weapon_desc,weapon_desc_ger,weapon_value,weapon_weight,weapon_bulk,weapon_range,weapon_skill,weapon_prof,weapon_iflag", NULL},
   {STAT_ROOMS, 6, true, "WriteRooms", "wiki_rooms", "room_name,room_name_ger,room_roo,room_number,room_region,room_iflag", NULL, -1, DB_WRITE_UPSERT, "room_number", "room_name,room_name_ger,room_roo,room_region,room_iflag", NULL},
   {STAT_NPC_ZONE, 4, true, "WriteNpcZone", "wiki_npc_zone", "npc_name,npc_roomid,npc_row,npc_col", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_NPC_SELLITEM, 3, true, "WriteNpcSellItem", "wiki_npc_sellitem", "npc_name,npc_item_sold,item_color", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_NPC_SELLSKILL, 2, true, "WriteNpcSellSkill", "wiki_npc_sellskill", "npc_name,npc_skill_id", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_NPC_SELLSPELL, 2, true, "WriteNpcSellSpell", "wiki_npc_sellspell", "npc_name,npc_spell_id", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_REAGENTS, 11, true, "WriteReagents", "wiki_reagents", "reagent_name,reagent_name_ger,reagent_icon,reagent_group,reagent_color,reagent_desc,reagent_desc_ger,reagent_value,reagent_weight,reagent_bulk,reagent_iflag", NULL, -1, DB_WRITE_UPSERT, "reagent_name", "reagent_icon,reagent_group,reagent_color,reagent_desc,reagent_desc_ger,reagent_value,reagent_weight,reagent_bulk,reagent_iflag", NULL},
   {STAT_FOOD, 11, true, "WriteFood", "wiki_food", "food_name,food_name_ger,food_icon,food_group,food_color,food_desc,food_desc_ger,food_value,food_weight,food_bulk,food_iflag", NULL, -1, DB_WRITE_UPSERT, "food_name", "food_group,food_color,food_desc,food_desc_ger,food_value,food_weight,food_bulk,food_iflag", NULL},
   {STAT_AMMO, 11, true, "WriteAmmo", "wiki_ammo", "ammo_name,ammo_name_ger,ammo_icon,ammo_group,ammo_color,ammo_desc,ammo_desc_ger,ammo_value,ammo_weight,ammo_bulk,ammo_iflag", NULL, -1, DB_WRITE_UPSERT, "ammo_name", "ammo_group,ammo_color,ammo_desc,ammo_desc_ger,ammo_value,ammo_weight,ammo_bulk,ammo_iflag", NULL},
   {STAT_ARMOR, 11, true, "WriteArmor", "wiki_armor", "armor_name,armor_name_ger,armor_icon,armor_group,armor_color,armor_desc,armor_desc_ger,armor_value,armor_weight,armor_bulk,armor_iflag", NULL, -1, DB_WRITE_UPSERT, "armor_name", "armor_group,armor_color,armor_desc,armor_desc_ger,armor_value,armor_weight,armor_bulk,armor_iflag", NULL},
   {STAT_MISCITEMS, 11, true, "WriteMiscItems", "wiki_miscitems", "misc_name,misc_name_ger,misc_icon,misc_group,misc_color,misc_desc,misc_desc_ger,misc_value,misc_weight,misc_bulk,misc_iflag", NULL, -1, DB_WRITE_UPSERT, "misc_name", "misc_group,misc_color,misc_desc,misc_desc_ger,misc_value,misc_weight,misc_bulk,misc_iflag", NULL},
   {STAT_RINGS, 11, true, "WriteRings", "wiki_rings", "rings_name,rings_name_ger,rings_icon,rings_group,rings_color,rings_desc,rings_desc_ger,rings_value,rings_weight,rings_bulk,rings_iflag", NULL, -1, DB_WRITE_UPSERT, "rings_name", "rings_group,rings_color,rings_desc,rings_desc_ger,rings_value,rings_weight,rings_bulk,rings_iflag", NULL},
   {STAT_RODS, 11, true, "WriteRods", "wiki_rods", "rods_name,rods_name_ger,rods_icon,rods_group,rods_color,rods_desc,rods_desc_ger,rods_value,rods_weight,rods_bulk,rods_iflag", NULL, -1, DB_WRITE_UPSERT, "rods_name", "rods_group,rods_color,rods_desc,rods_desc_ger,rods_value,rods_weight,rods_bulk,rods_iflag", NULL},
   {STAT_POTIONS, 11, true, "WritePotions", "wiki_potions", "potion_name,potion_name_ger,potion_icon,potion_group,potion_color,potion_desc,potion_desc_ger,potion_value,potion_weight,potion_bulk,potion_iflag", NULL, -1, DB_WRITE_UPSERT, "potion_name", "potion_group,potion_color,potion_desc,potion_desc_ger,potion_value,potion_weight,potion_bulk,potion_iflag", NULL},
   {STAT_SCROLLS, 11, true, "WriteScrolls", "wiki_scrolls", "scrolls_name,scrolls_name_ger,scrolls_icon,scrolls_group,scrolls_color,scrolls_desc,scrolls_desc_ger,scrolls_value,scrolls_weight,scrolls_bulk,scrolls_iflag", NULL, -1, DB_WRITE_UPSERT, "scrolls_name", "scrolls_group,scrolls_color,scrolls_desc,scrolls_desc_ger,scrolls_value,scrolls_weight,scrolls_bulk,scrolls_iflag", NULL},
   {STAT_WANDS, 11, true, "WriteWands", "wiki_wands", "wands_name,wands_name_ger,wands_icon,wands_group,wands_color,wands_desc,wands_desc_ger,wands_value,wands_weight,wands_bulk,wands_iflag", NULL, -1, DB_WRITE_UPSERT, "wands_name", "wands_group,wands_color,wands_desc,wands_desc_ger,wands_value,wands_weight,wands_bulk,wands_iflag", NULL},
   {STAT_QUESTITEMS, 11, true, "WriteQuestItems", "wiki_questitems", "questitem_name,questitem_name_ger,questitem_icon,questitem_group,questitem_color,questitem_desc,questitem_desc_ger,questitem_value,questitem_weight,questitem_bulk,questitem_iflag", NULL, -1, DB_WRITE_UPSERT, "questitem_name", "questitem_group,questitem_color,questitem_desc,questitem_desc_ger,questitem_value,questitem_weight,questitem_bulk,questitem_iflag", NULL},
   {STAT_SKILLS, 12, true, "WriteSkills", "wiki_skills", "skill_id,skill_name,skill_name_ger,skill_icon,skill_desc,skill_desc_ger,skill_school,skill_level,skill_chance,skill_mediate_ratio,skill_exertion,skill_iflag", NULL, -1, DB_WRITE_UPSERT, "skill_id", "skill_name,skill_name_ger,skill_desc,skill_desc_ger,skill_school,skill_level,skill_chance,skill_mediate_ratio,skill_exertion,skill_iflag", NULL},
   {STAT_NECKLACE, 11, true, "WriteNecklace", "wiki_necklace", "necklace_name,necklace_name_ger,necklace_icon,necklace_group,necklace_color,necklace_desc,necklace_desc_ger,necklace_value,necklace_weight,necklace_bulk,necklace_iflag", NULL, -1, DB_WRITE_UPSERT, "necklace_name", "necklace_group,necklace_color,necklace_desc,necklace_desc_ger,necklace_value,necklace_weight,necklace_bulk,necklace_iflag", NULL},
   {STAT_INSTRUMENTS, 11, true, "WriteInstruments", "wiki_instruments", "instrument_name,instrument_name_ger,instrument_icon,instrument_group,instrument_color,instrument_desc,instrument_desc_ger,instrument_value,instrument_weight,instrument_bulk,instrument_iflag", NULL, -1, DB_WRITE_UPSERT, "instrument_name", "instrument_group,instrument_color,instrument_desc,instrument_desc_ger,instrument_value,instrument_weight,instrument_bulk,instrument_iflag", NULL},
   {STAT_GEMS, 11, true, "WriteGems", "wiki_gems", "gem_name,gem_name_ger,gem_icon,gem_group,gem_color,gem_desc,gem_desc_ger,gem_value,gem_weight,gem_bulk,gem_iflag", NULL, -1, DB_WRITE_UPSERT, "gem_name", "gem_group,gem_color,gem_desc,gem_desc_ger,gem_value,gem_weight,gem_bulk,gem_iflag", NULL},
   {STAT_OFFERINGS, 11, true, "WriteOfferings", "wiki_offerings", "offering_name,offering_name_ger,offering_icon,offering_group,offering_color,offering_desc,offering_desc_ger,offering_value,offering_weight,offering_bulk,offering_iflag", NULL, -1, DB_WRITE_UPSERT, "offering_name", "offering_group,offering_color,offering_desc,offering_desc_ger,offering_value,offering_weight,offering_bulk,offering_iflag", NULL},
   {STAT_QUESTS, 11, true, "WriteQuests", "wiki_quests", "quest_id,quest_name,quest_name_ger,quest_icon,quest_icon_group,quest_desc,quest_desc_ger,quest_recent_time,quest_schedule_pct,quest_est_time,quest_est_diff", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_TREASURE_EXTRA, 4, true, "WriteTreasureExtra", "wiki_treasure_extra", "treasure_id,item_name,item_min_amount,item_max_amount", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_TREASURE_MAGIC, 3, true, "WriteTreasureMagic", "wiki_treasure_magic", "treasure_id,item_name,item_attribute_id", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_NPC_SELLCOND, 4, true, "WriteNpcSellCond", "wiki_npc_sellcond", "npc_name,npc_item_sold,item_color,item_price", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_LOGPEN, 7, true, "WriteLogpen", "player_logpen", "player_name,room_id,logpen_xp,logpen_hp,logpen_spellpct,logpen_skillpct,logpen_numitems", "logpen_time", 1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_LOGPEN_ITEM, 3, true, "WriteLogpenItem", "player_logpen_items", "player_name,item_name,item_number", "logpen_time", 1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_QUESTGIVER, 2, true, "WriteQuestGiver", "wiki_quest_giver", "quest_id,quest_npc_name", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_ACCOUNTS, 8, true, "WriteAccount", "server_accounts", "acct_id,acct_name,acct_password,acct_email,acct_type,acct_loggedin_time,acct_last_login,acct_suspend_time", NULL, -1, DB_WRITE_UPSERT, "acct_id", "acct_name,acct_password,acct_email,acct_type,acct_loggedin_time,acct_last_login,acct_suspend_time", NULL},
   {STAT_ACCOUNT_CHARS, 2, true, "WriteAccountChar", "server_account_chars", "acct_id,char_name", NULL, -1, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_COMPL_QUESTS, 5, true, "WriteCompletedQuest", "player_completed_quests", "quest_id,player_name,num_seconds_taken,xp_rewarded,tp_rewarded", "completion_time", 2, DB_WRITE_INSERT, NULL, NULL, NULL},
   {STAT_COMPL_QUEST_ITEMS, 4, true, "WriteCompQuestRewardItems", "player_quest_reward_items", "quest_id,player_name,item_name,item_number", "completion_time", 2, DB_WRITE_INSERT, NULL, NULL, NULL}
};

#endif
