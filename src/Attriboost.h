#ifndef MODULE_ATTRIBOOST_H
#define MODULE_ATTRIBOOST_H

#include "ScriptMgr.h"
#include "Player.h"

#include <unordered_map>

enum AttriboostConstants
{
    ATTR_ITEM = 16073,
    ATTR_SPELL = 18282,
    ATTR_QUEST = 441153
};

enum AttriboostStats
{
    ATTR_SPELL_STAMINA = 7477,
    ATTR_SPELL_AGILITY = 7471,
    ATTR_SPELL_INTELLECT = 7468,
    ATTR_SPELL_STRENGTH = 7464,
    ATTR_SPELL_SPIRIT = 7474
};

struct Attriboosts
{
    uint32 Unallocated;
    uint32 Stamina;
    uint32 Strength;
    uint32 Agility;
    uint32 Intellect;
    uint32 Spirit;
};

std::unordered_map<uint64, Attriboosts> attriboostsMap;

Attriboosts* GetAttriboosts(uint64 /*guid*/);
void ClearAttriboosts();
void LoadAttriboosts();
void ApplyAttributes(Player* /*player*/, Attriboosts* /*attributes*/);
void AddAttribute(Attriboosts* /*attributes*/, uint32 /*attribute*/);
void ResetAttributes(Attriboosts* /*attributes*/);
bool HasAttributesToSpend(Player* /*player*/);
bool HasAttributes(Player* /*player*/);
uint32 GetAttributesToSpend(Player* /*player*/);

class AttriboostPlayerScript : public PlayerScript
{
public:
    AttriboostPlayerScript() : PlayerScript("AttriboostPlayerScript") { }

    virtual void OnLogin(Player* /*player*/) override;
    virtual void OnPlayerCompleteQuest(Player* /*player*/, Quest const* /*quest*/) override;

    uint32 GetRandomAttributeForClass(Player* /*player*/);
    std::string GetAttributeName(uint32 /*attribute*/);
};

class AttriboostCreatureScript : public CreatureScript
{
public:
    AttriboostCreatureScript() : CreatureScript("AttriboostCreatureScript") { }

    virtual bool OnGossipHello(Player* /*player*/, Creature* /*creature*/) override;
    virtual bool OnGossipSelect(Player* /*player*/, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/) override;

    void HandleAttributeAllocation(Player* /*player*/, uint32 /*attribute*/, bool /*reset*/);
};

class AttriboostWorldScript : public WorldScript
{
public:
    AttriboostWorldScript() : WorldScript("AttriboostWorldScript") { }

    virtual void OnAfterConfigLoad(bool /*reload*/) override;
    virtual void OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/) override;
};

#endif // MODULE_ATTRIBOOST_H
