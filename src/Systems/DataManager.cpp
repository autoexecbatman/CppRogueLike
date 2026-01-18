// file: Systems/DataManager.cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "DataManager.h"
#include "MessageSystem.h"

// Include only the struct definitions
#include "../Items/Weapons.h"
#include "../Attributes/StrengthAttributes.h"
#include "../Attributes/DexterityAttributes.h"
#include "../Attributes/ConstitutionAttributes.h"
#include "../Attributes/CharismaAttributes.h"
#include "../Attributes/IntelligenceAttributes.h"
#include "../Attributes/WisdomAttributes.h"

namespace {
    // Search multiple paths for a JSON file
    std::string find_data_file(const std::string& filename) {
        std::vector<std::string> searchPaths = {
            "./" + filename,                    // Current directory
            "./bin/Debug/" + filename,          // Build output (Debug)
            "./bin/Release/" + filename,        // Build output (Release)
            "../src/json/" + filename,          // From build dir to source
            "./src/json/" + filename,           // From project root
        };

        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        return "./" + filename; // Fallback to original behavior
    }
}

void DataManager::load_all_data(MessageSystem& message_system)
{
    message_system.log("DataManager: Starting data load...");

    strengthAttributes = load_strength(find_data_file("strength.json"), message_system);
    dexterityAttributes = load_dexterity(find_data_file("dexterity.json"), message_system);
    constitutionAttributes = load_constitution(find_data_file("constitution.json"), message_system);
    charismaAttributes = load_charisma(find_data_file("charisma.json"), message_system);
    intelligenceAttributes = load_intelligence(find_data_file("intelligence.json"), message_system);
    wisdomAttributes = load_wisdom(find_data_file("wisdom.json"), message_system);

    message_system.log("DataManager: All game data loaded successfully");
}

std::vector<Weapons> DataManager::load_weapons(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<Weapons> data;
    for (const auto& item : j) {
        Weapons w;
        w.name = item.value("name", "");
        w.type = item.value("type", "");
        w.damageRoll = item.value("damageRoll", "");
        w.damageRollTwoHanded = item.value("damageRollTwoHanded", "");
        w.enhancementLevel = item.value("enhancementLevel", 0);
        
        // Handle enum values with defaults
        std::string handReq = item.value("handRequirement", "ONE_HANDED");
        if (handReq == "TWO_HANDED") w.handRequirement = HandRequirement::TWO_HANDED;
        else if (handReq == "OFF_HAND_ONLY") w.handRequirement = HandRequirement::OFF_HAND_ONLY;
        else w.handRequirement = HandRequirement::ONE_HANDED;
        
        std::string weapSize = item.value("weaponSize", "MEDIUM");
        if (weapSize == "TINY") w.weaponSize = WeaponSize::TINY;
        else if (weapSize == "SMALL") w.weaponSize = WeaponSize::SMALL;
        else if (weapSize == "LARGE") w.weaponSize = WeaponSize::LARGE;
        else if (weapSize == "GIANT") w.weaponSize = WeaponSize::GIANT;
        else w.weaponSize = WeaponSize::MEDIUM;
        
        // Handle arrays if they exist in JSON
        if (item.contains("hitBonusRange") && item["hitBonusRange"].is_array()) {
            for (const auto& bonus : item["hitBonusRange"]) {
                w.hitBonusRange.push_back(bonus.get<int>());
            }
        }
        
        if (item.contains("damageBonusRange") && item["damageBonusRange"].is_array()) {
            for (const auto& bonus : item["damageBonusRange"]) {
                w.damageBonusRange.push_back(bonus.get<int>());
            }
        }
        
        if (item.contains("specialProperties") && item["specialProperties"].is_array()) {
            for (const auto& prop : item["specialProperties"]) {
                w.specialProperties.push_back(prop.get<std::string>());
            }
        }
        
        data.push_back(w);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " weapons");
    return data;
}

std::vector<StrengthAttributes> DataManager::load_strength(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<StrengthAttributes> data;
    for (const auto& item : j) {
        StrengthAttributes s;
        s.Str = item.value("Str", 0);
        s.hitProb = item.value("Hit", 0);
        s.dmgAdj = item.value("Dmg", 0);
        s.wgtAllow = item.value("Wgt", 0);
        s.maxPress = item.value("MaxPress", 0);
        s.openDoors = item.value("OpenDoors", 0);
        s.BB_LG = item.value("BB_LG", 0.0);
        s.notes = item.value("Notes", "");
        data.push_back(s);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " strength attributes");
    return data;
}

std::vector<DexterityAttributes> DataManager::load_dexterity(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<DexterityAttributes> data;
    for (const auto& item : j) {
        DexterityAttributes d;
        d.Dex = item.value("Dex", 0);
        d.ReactionAdj = item.value("ReactionAdj", 0);
        d.MissileAttackAdj = item.value("MissileAttackAdj", 0);
        d.DefensiveAdj = item.value("DefensiveAdj", 0);
        data.push_back(d);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " dexterity attributes");
    return data;
}

std::vector<ConstitutionAttributes> DataManager::load_constitution(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<ConstitutionAttributes> data;
    for (const auto& item : j) {
        ConstitutionAttributes c;
        c.Con = item.value("Con", 0);
        c.HPAdj = item.value("HPAdj", 0);
        c.SystemShock = item.value("SystemShock", 0);
        c.ResurrectionSurvival = item.value("ResurrectionSurvival", 0);
        c.PoisonSave = item.value("PoisonSave", 0);
        c.Regeneration = item.value("Regeneration", 0);
        data.push_back(c);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " constitution attributes");
    return data;
}

std::vector<CharismaAttributes> DataManager::load_charisma(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<CharismaAttributes> data;
    for (const auto& item : j) {
        CharismaAttributes c;
        c.Cha = item.value("Cha", 0);
        c.MaxHencmen = item.value("MaxHencmen", 0);
        c.Loyalty = item.value("Loyalty", 0);
        c.ReactionAdj = item.value("ReactionAdj", 0);
        data.push_back(c);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " charisma attributes");
    return data;
}

std::vector<IntelligenceAttributes> DataManager::load_intelligence(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<IntelligenceAttributes> data;
    for (const auto& item : j) {
        IntelligenceAttributes i;
        i.Int = item.value("Int", 0);
        i.NumberOfLanguages = item.value("NumberOfLanguages", 0);
        i.SpellLevel = item.value("SpellLevel", 0);
        i.ChanceToLearnSpell = item.value("ChanceToLearnSpell", 0);
        i.MaxNumberOfSpells = item.value("MaxNumberOfSpells", 0);
        i.IllusionImmunity = item.value("IllusionImmunity", 0);
        data.push_back(i);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " intelligence attributes");
    return data;
}

std::vector<WisdomAttributes> DataManager::load_wisdom(const std::string& filename, MessageSystem& message_system)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        message_system.log("DataManager: Error opening " + filename);
        return {};
    }
    
    nlohmann::json j;
    file >> j;
    
    std::vector<WisdomAttributes> data;
    for (const auto& item : j) {
        WisdomAttributes w;
        w.Wis = item.value("Wis", 0);
        w.MagicalDefenseAdj = item.value("MagicalDefenseAdj", 0);
        w.BonusSpells = item.value("BonusSpells", 0);
        w.ChanceOfSpellFailure = item.value("ChanceOfSpellFailure", 0);
        w.SpellImmunity = item.value("SpellImmunity", 0);
        data.push_back(w);
    }
    
    message_system.log("DataManager: Loaded " + std::to_string(data.size()) + " wisdom attributes");
    return data;
}

// end of file: Systems/DataManager.cpp
