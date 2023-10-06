#include "Attriboost.h"

#include "Chat.h"
#include "Config.h"
#include "Spell.h"

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

    auto attributes = GetAttriboosts(player->GetGUID().GetRawValue());
    ApplyAttributes(player, attributes);
}

bool AttriboostPlayerScript::CanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& /*result*/)
{
    if (!proto)
    {
        return true;
    }

    if (proto->ItemId != ATTR_ITEM)
    {
        return true;
    }

    if (!sConfigMgr->GetOption<bool>("Attriboost.Enable", false))
    {
        ChatHandler(player->GetSession()).SendSysMessage("This item is disabled.");
        return false;
    }

    auto attribute = GetRandomAttributeForClass(player);
    if (!attribute)
    {
        LOG_WARN("module", "Failed to get random attribute for player '{}'.", player->GetName());
        return false;
    }

    auto attributes = GetAttriboosts(player->GetGUID().GetRawValue());
    AddAttribute(attributes, attribute);
    ApplyAttributes(player, attributes);

    auto attributeName = GetAttributeName(attribute);
    if (!attributeName.empty())
    {
        ChatHandler(player->GetSession()).SendSysMessage(Acore::StringFormatFmt("Increased {} by 1.", GetAttributeName(attribute)));
    }

    player->DestroyItemCount(ATTR_ITEM, 1, true);

    return true;
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

Attriboosts* GetAttriboosts(uint64 guid)
{
    auto attri = attriboostsMap.find(guid);
    if (attri == attriboostsMap.end())
    {
        Attriboosts attriboosts;
        attriboosts.Stamina = 0;
        attriboosts.Strength = 0;
        attriboosts.Agility = 0;
        attriboosts.Intellect = 0;
        attriboosts.Spirit = 0;

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
        attriboosts.Stamina = fields[1].Get<uint64>();
        attriboosts.Strength = fields[2].Get<uint64>();
        attriboosts.Agility = fields[3].Get<uint64>();
        attriboosts.Intellect = fields[4].Get<uint64>();
        attriboosts.Spirit = fields[5].Get<uint64>();

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

        auto stamina = it->second.Stamina;
        auto strength = it->second.Strength;
        auto agility = it->second.Agility;
        auto intellect = it->second.Intellect;
        auto spirit = it->second.Spirit;

        CharacterDatabase.Execute("INSERT INTO `attriboost_attributes` (guid, stamina, strength, agility, intellect, spirit) VALUES ({}, {}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE stamina={}, strength={}, agility={}, intellect={}, spirit={}",
            guid,
            stamina, strength, agility, intellect, spirit,
            stamina, strength, agility, intellect, spirit);
    }
}

void ApplyAttributes(Player* player, Attriboosts* attributes)
{
    if (attributes->Stamina > 0)
    {
        auto stamina = player->GetAura(ATTR_SPELL_STAMINA);
        if (!stamina)
        {
            stamina = player->AddAura(ATTR_SPELL_STAMINA, player);
        }
        stamina->SetStackAmount(attributes->Stamina);
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

    if (attributes->Agility > 0)
    {
        auto agility = player->GetAura(ATTR_SPELL_AGILITY);
        if (!agility)
        {
            agility = player->AddAura(ATTR_SPELL_AGILITY, player);
        }
        agility->SetStackAmount(attributes->Agility);
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

    if (attributes->Spirit > 0)
    {
        auto spirit = player->GetAura(ATTR_SPELL_SPIRIT);
        if (!spirit)
        {
            spirit = player->AddAura(ATTR_SPELL_SPIRIT, player);
        }
        spirit->SetStackAmount(attributes->Spirit);
    }
}

void AddAttribute(Attriboosts* attributes, uint32 attribute)
{
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

void AttriboostWorldScript::OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/)
{
    SaveAttriboosts();
}

void SC_AddAttriboostScripts()
{
    new AttriboostWorldScript();
    new AttriboostPlayerScript();
}
