#include "Attriboost.h"

#include "Chat.h"
#include "Config.h"
#include "Spell.h"

#include <AI/ScriptedAI/ScriptedGossip.h>

void AttriboostPlayerScript::OnLogin(Player* player)
{
    if (!player)
    {
        return;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.Enable", false))
    {
        return;
    }

    auto attributes = GetAttriboosts(player);
    ApplyAttributes(player, attributes);
}

void AttriboostPlayerScript::OnPlayerCompleteQuest(Player* player, Quest const* quest)
{
    if (!player)
    {
        return;
    }

    if (quest->GetQuestId() != ATTR_QUEST)
    {
        return;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.Enable", false))
    {
        return;
    }

    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return;
    }

    attributes->Unallocated += 1;
}

void AttriboostPlayerScript::OnPlayerLeaveCombat(Player* player)
{
    if (!player)
    {
        return;
    }

    // This hook is called when removing the player from the world (like shutdown),
    // even when you are not in combat.
    if (player->IsDuringRemoveFromWorld() || !player->IsInWorld())
    {
        return;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.Enable", false))
    {
        return;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.DisablePvP", false))
    {
        return;
    }

    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return;
    }

    ApplyAttributes(player, attributes);
}

uint32 AttriboostPlayerScript::GetRandomAttributeForClass(Player* player)
{
    switch (player->getClass())
    {
    case CLASS_DEATH_KNIGHT:
    case CLASS_WARRIOR:
    case CLASS_ROGUE:
        switch (urand(0, 2))
        {
        case 0:
            return ATTR_SPELL_STAMINA;
        case 1:
            return ATTR_SPELL_AGILITY;
        case 2:
            return ATTR_SPELL_STRENGTH;
        }

    case CLASS_WARLOCK:
    case CLASS_PRIEST:
    case CLASS_MAGE:
        switch (urand(0, 2))
        {
        case 0:
            return ATTR_SPELL_STAMINA;
        case 1:
            return ATTR_SPELL_INTELLECT;
        case 2:
            return ATTR_SPELL_SPIRIT;
        }

    case CLASS_SHAMAN:
    case CLASS_HUNTER:
    case CLASS_PALADIN:
    case CLASS_DRUID:
        switch (urand(0, 4))
        {
        case 0:
            return ATTR_SPELL_STAMINA;
        case 1:
            return ATTR_SPELL_STRENGTH;
        case 2:
            return ATTR_SPELL_AGILITY;
        case 3:
            return ATTR_SPELL_INTELLECT;
        case 4:
            return ATTR_SPELL_SPIRIT;
        }
    }

    return 0;
}

std::string AttriboostPlayerScript::GetAttributeName(uint32 attribute)
{
    switch (attribute)
    {
    case ATTR_SPELL_STAMINA:
        return "Stamina";

    case ATTR_SPELL_STRENGTH:
        return "Strength";

    case ATTR_SPELL_AGILITY:
        return "Agility";

    case ATTR_SPELL_INTELLECT:
        return "Intellect";

    case ATTR_SPELL_SPIRIT:
        return "Spirit";
    }

    return std::string();
}

Attriboosts* GetAttriboosts(Player* player)
{
    auto guid = player->GetGUID().GetRawValue();

    auto attri = attriboostsMap.find(guid);
    if (attri == attriboostsMap.end())
    {
        Attriboosts attriboosts;

        attriboosts.Unallocated = 0;

        attriboosts.Stamina = 0;
        attriboosts.Strength = 0;
        attriboosts.Agility = 0;
        attriboosts.Intellect = 0;
        attriboosts.Spirit = 0;

        attriboosts.Settings = ATTR_SETTING_PROMPT;

        auto result = attriboostsMap.emplace(guid, attriboosts);
        attri = result.first;
    }

    return &attri->second;
}

void ClearAttriboosts()
{
    attriboostsMap.clear();
}

void LoadAttriboosts()
{
    auto qResult = CharacterDatabase.Query("SELECT * FROM attriboost_attributes");

    if (!qResult)
    {
        LOG_ERROR("module", "Failed to load from 'attriboost_attributes' table.");
        return;
    }

    LOG_INFO("module", "Loading player attriboosts from 'attriboost_attributes'..");

    int count = 0;

    do
    {
        auto fields = qResult->Fetch();

        uint64 guid = fields[0].Get<uint64>();

        Attriboosts attriboosts;

        attriboosts.Unallocated = fields[1].Get<uint32>();

        attriboosts.Stamina = fields[2].Get<uint32>();
        attriboosts.Strength = fields[3].Get<uint32>();
        attriboosts.Agility = fields[4].Get<uint32>();
        attriboosts.Intellect = fields[5].Get<uint32>();
        attriboosts.Spirit = fields[6].Get<uint32>();

        attriboosts.Settings = fields[7].Get<uint32>();

        attriboostsMap.emplace(guid, attriboosts);

        count++;
    } while (qResult->NextRow());

    LOG_INFO("module", "Loaded '{}' player attriboosts.", count);
}

void SaveAttriboosts()
{
    for (auto it = attriboostsMap.begin(); it != attriboostsMap.end(); ++it)
    {
        auto guid = it->first;

        auto unallocated = it->second.Unallocated;

        auto stamina = it->second.Stamina;
        auto strength = it->second.Strength;
        auto agility = it->second.Agility;
        auto intellect = it->second.Intellect;
        auto spirit = it->second.Spirit;

        auto settings = it->second.Settings;

        CharacterDatabase.Execute("INSERT INTO `attriboost_attributes` (guid, unallocated, stamina, strength, agility, intellect, spirit, settings) VALUES ({}, {}, {}, {}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE unallocated={}, stamina={}, strength={}, agility={}, intellect={}, spirit={}, settings={}",
            guid,
            unallocated, stamina, strength, agility, intellect, spirit, settings,
            unallocated, stamina, strength, agility, intellect, spirit, settings);
    }
}

void ApplyAttributes(Player* player, Attriboosts* attributes)
{
    if (attributes->Stamina > 0)
    {
        float hp = player->GetHealth();

        auto stamina = player->GetAura(ATTR_SPELL_STAMINA);
        if (!stamina)
        {
            stamina = player->AddAura(ATTR_SPELL_STAMINA, player);
        }
        stamina->SetStackAmount(attributes->Stamina);

        player->SetHealth(hp);
    }
    else
    {
        if (player->GetAura(ATTR_SPELL_STAMINA))
        {
            player->RemoveAura(ATTR_SPELL_STAMINA);
        }
    }

    if (attributes->Strength > 0)
    {
        auto strength = player->GetAura(ATTR_SPELL_STRENGTH);
        if (!strength)
        {
            strength = player->AddAura(ATTR_SPELL_STRENGTH, player);
        }
        strength->SetStackAmount(attributes->Strength);
    }
    else
    {
        if (player->GetAura(ATTR_SPELL_STRENGTH))
        {
            player->RemoveAura(ATTR_SPELL_STRENGTH);
        }
    }

    if (attributes->Agility > 0)
    {
        auto agility = player->GetAura(ATTR_SPELL_AGILITY);
        if (!agility)
        {
            agility = player->AddAura(ATTR_SPELL_AGILITY, player);
        }
        agility->SetStackAmount(attributes->Agility);
    }
    else
    {
        if (player->GetAura(ATTR_SPELL_AGILITY))
        {
            player->RemoveAura(ATTR_SPELL_AGILITY);
        }
    }

    if (attributes->Intellect > 0)
    {
        auto intellect = player->GetAura(ATTR_SPELL_INTELLECT);
        if (!intellect)
        {
            intellect = player->AddAura(ATTR_SPELL_INTELLECT, player);
        }
        intellect->SetStackAmount(attributes->Intellect);
    }
    else
    {
        if (player->GetAura(ATTR_SPELL_INTELLECT))
        {
            player->RemoveAura(ATTR_SPELL_INTELLECT);
        }
    }

    if (attributes->Spirit > 0)
    {
        auto spirit = player->GetAura(ATTR_SPELL_SPIRIT);
        if (!spirit)
        {
            spirit = player->AddAura(ATTR_SPELL_SPIRIT, player);
        }
        spirit->SetStackAmount(attributes->Spirit);
    }
    else
    {
        if (player->GetAura(ATTR_SPELL_SPIRIT))
        {
            player->RemoveAura(ATTR_SPELL_SPIRIT);
        }
    }
}

void DisableAttributes(Player* player)
{
    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return;
    }

    if (player->GetAura(ATTR_SPELL_STAMINA))
    {
        player->RemoveAura(ATTR_SPELL_STAMINA);
    }

    if (player->GetAura(ATTR_SPELL_STRENGTH))
    {
        player->RemoveAura(ATTR_SPELL_STRENGTH);
    }

    if (player->GetAura(ATTR_SPELL_AGILITY))
    {
        player->RemoveAura(ATTR_SPELL_AGILITY);
    }

    if (player->GetAura(ATTR_SPELL_INTELLECT))
    {
        player->RemoveAura(ATTR_SPELL_INTELLECT);
    }

    if (player->GetAura(ATTR_SPELL_SPIRIT))
    {
        player->RemoveAura(ATTR_SPELL_SPIRIT);
    }
}

void AddAttribute(Attriboosts* attributes, uint32 attribute)
{
    if (attributes->Unallocated < 1)
    {
        return;
    }

    switch (attribute)
    {
    case ATTR_SPELL_STAMINA:
        attributes->Stamina += 1;
        break;

    case ATTR_SPELL_STRENGTH:
        attributes->Strength += 1;
        break;

    case ATTR_SPELL_AGILITY:
        attributes->Agility += 1;
        break;

    case ATTR_SPELL_INTELLECT:
        attributes->Intellect += 1;
        break;

    case ATTR_SPELL_SPIRIT:
        attributes->Spirit += 1;
        break;
    }

    attributes->Unallocated -= 1;
}

void ResetAttributes(Attriboosts* attributes)
{
    attributes->Unallocated += attributes->Stamina + attributes->Strength + attributes->Agility + attributes->Intellect + attributes->Spirit;
    attributes->Stamina = 0;
    attributes->Strength = 0;
    attributes->Agility = 0;
    attributes->Intellect = 0;
    attributes->Spirit = 0;
}

bool HasAttributesToSpend(Player* player)
{
    if (!player)
    {
        return false;
    }

    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return false;
    }

    return attributes->Unallocated > 0;
}

bool HasAttributes(Player* player)
{
    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return false;
    }

    return (attributes->Stamina + attributes->Strength + attributes->Agility + attributes->Intellect + attributes->Spirit) > 0;
}

uint32 GetAttributesToSpend(Player* player)
{
    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return 0;
    }

    return attributes->Unallocated;
}

bool HasSetting(Player* player, uint32 setting)
{
    if (!player)
    {
        return false;
    }

    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return false;
    }

    return (attributes->Settings & setting) == setting;
}
void ToggleSetting(Player* player, uint32 setting)
{
    if (!player)
    {
        return;
    }

    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return;
    }

    if (HasSetting(player, setting))
    {
        attributes->Settings -= setting;
    }
    else
    {
        attributes->Settings += setting;
    }
}

void AttriboostWorldScript::OnAfterConfigLoad(bool reload)
{
    if (reload)
    {
        SaveAttriboosts();
        ClearAttriboosts();
    }

    LoadAttriboosts();
}

bool AttriboostCreatureScript::OnGossipHello(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);

    if (!sConfigMgr->GetOption<bool>("Attriboost.Enable", false))
    {
        SendGossipMenuFor(player, 441192, creature);

        return true;
    }

    player->PrepareQuestMenu(creature->GetGUID());

    if (HasAttributesToSpend(player))
    {
        auto attributes = GetAttriboosts(player);
        if (!attributes)
        {
            CloseGossipMenuFor(player);
            return false;
        }

        AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t |cffFF0000{} |rAttribute(s) to spend.", GetAttributesToSpend(player)), GOSSIP_SENDER_MAIN, 0);

        if (HasSetting(player, ATTR_SETTING_PROMPT))
        {
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Stamina ({})", attributes->Stamina), GOSSIP_SENDER_MAIN, ATTR_SPELL_STAMINA, "Are you sure you want to spend your points in stamina?", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Strength ({})", attributes->Strength), GOSSIP_SENDER_MAIN, ATTR_SPELL_STRENGTH, "Are you sure you want to spend your points in strength?", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Agility ({})", attributes->Agility), GOSSIP_SENDER_MAIN, ATTR_SPELL_AGILITY, "Are you sure you want to spend your points in agility?", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Intellect ({})", attributes->Intellect), GOSSIP_SENDER_MAIN, ATTR_SPELL_INTELLECT, "Are you sure you want to spend your points in intellect?", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Spirit ({})", attributes->Spirit), GOSSIP_SENDER_MAIN, ATTR_SPELL_SPIRIT, "Are you sure you want to spend your points in spirit?", 0, false);
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Stamina ({})", attributes->Stamina), GOSSIP_SENDER_MAIN, ATTR_SPELL_STAMINA);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Strength ({})", attributes->Strength), GOSSIP_SENDER_MAIN, ATTR_SPELL_STRENGTH);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Agility ({})", attributes->Agility), GOSSIP_SENDER_MAIN, ATTR_SPELL_AGILITY);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Intellect ({})", attributes->Intellect), GOSSIP_SENDER_MAIN, ATTR_SPELL_INTELLECT);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t Spirit ({})", attributes->Spirit), GOSSIP_SENDER_MAIN, ATTR_SPELL_SPIRIT);
        }
    }

    if (HasAttributes(player))
    {
        uint32 resetCost = sConfigMgr->GetOption<uint32>("Attriboost.ResetCost", 2500000);
        AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\UnlearnGossipIcon:16|t Reset Attributes", GOSSIP_SENDER_MAIN, 1000, "Are you sure you want to reset your attributes?", resetCost, false);
    }

    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\HealerGossipIcon:16|t Settings", GOSSIP_SENDER_MAIN, 2000);

    if (HasAttributesToSpend(player))
    {
        SendGossipMenuFor(player, 441191, creature);
    }
    else
    {
        SendGossipMenuFor(player, 441190, creature);
    }

    return true;
}

bool AttriboostCreatureScript::OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
    if (action == 0)
    {
        OnGossipHello(player, creature);
    }

    if (action > 5000)
    {
        HandleAttributeAllocation(player, action, false);
        OnGossipHello(player, creature);
    }

    if (action == 1000)
    {
        HandleAttributeAllocation(player, action, true);
        OnGossipHello(player, creature);
    }

    if (action >= 2000 && action < 3000)
    {
        HandleSettings(player, creature, action);
    }

    return true;
}

void AttriboostCreatureScript::HandleSettings(Player* player, Creature* creature, uint32 action)
{
    if (action == 2000)
    {
        ClearGossipMenuFor(player);

        player->PrepareQuestMenu(creature->GetGUID());

        auto hasPromptSetting = HasSetting(player, ATTR_SETTING_PROMPT);
        AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormatFmt("|TInterface\\GossipFrame\\HealerGossipIcon:16|t Prompt 'Are you sure': {}", hasPromptSetting ? "|cff00FF00Enabled|r" : "|cffFF0000Disabled"), GOSSIP_SENDER_MAIN, 2001);

        AddGossipItemFor(player, GOSSIP_ICON_DOT, "Back", GOSSIP_SENDER_MAIN, 0);

        SendGossipMenuFor(player, 441190, creature);

        return;
    }

    if (action == 2001)
    {
        ToggleSetting(player, ATTR_SETTING_PROMPT);
        HandleSettings(player, creature, 2000);
    }
}

void AttriboostCreatureScript::HandleAttributeAllocation(Player* player, uint32 attribute, bool reset)
{
    if (!player)
    {
        return;
    }

    auto attributes = GetAttriboosts(player);
    if (!attributes)
    {
        return;
    }

    if (reset)
    {
        ResetAttributes(attributes);
    }
    else
    {
        AddAttribute(attributes, attribute);
    }

    ApplyAttributes(player, attributes);
}

void AttriboostWorldScript::OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/)
{
    SaveAttriboosts();
}

void AttriboostUnitScript::OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/)
{
    if (!attacker || !victim)
    {
        return;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.Enable", false))
    {
        return;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.DisablePvP", false))
    {
        return;
    }

    auto p1 = attacker->ToPlayer();
    auto p2 = victim->ToPlayer();

    if (!p1 || !p2)
    {
        return;
    }

    DisableAttributes(p1);
    DisableAttributes(p2);
}

void SC_AddAttriboostScripts()
{
    new AttriboostWorldScript();
    new AttriboostPlayerScript();
    new AttriboostCreatureScript();
    new AttriboostUnitScript();
}
