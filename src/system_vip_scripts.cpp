/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "SystemVip.h"
#include "WorldSessionMgr.h"

#define sV sSystemVip

// Add player scripts
class SystemVipPlayer : public PlayerScript
{
public:
    SystemVipPlayer() : PlayerScript("SystemVipPlayer", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_GIVE_EXP,
        PLAYERHOOK_ON_BEFORE_LOOT_MONEY,
        PLAYERHOOK_ON_PLAYER_RELEASED_GHOST,
        PLAYERHOOK_ON_VICTIM_REWARD_AFTER
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("SystemVip.Announce", false))
        {
			uint32 loc = player->GetSession()->GetSessionDbLocaleIndex();
            if (loc == 4)
                ChatHandler(player->GetSession()).SendSysMessage("|cff00ff00本服务端已加载|r |cff00ccffVIP系统 |r|cff00ff00模块.|r");
            else
				ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00SystemVip |rmodule.");
        }

        if (sV->isVip(player) && sV->loginAnnounce) {
            sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, sV->getLoginMessage(player));
        }

        if (sV->isVip(player)) {
            ChatHandler(player->GetSession()).PSendSysMessage("可用的VIP订阅时间: |cff4CFF00{}|r", sV->getFormatedVipTime(player).c_str());
        }

        sV->delExpireVip(player);
        if (sV->saveTeleport && sV->isVip(player))
            sV->loadTeleportVip(player);
    }

    void OnPlayerLogout(Player* player) override {
        if (sV->saveTeleport && sV->isVip(player))
            sV->teleportMap.erase(player->GetSession()->GetAccountId());
    }

	//void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override {
 //       if (sV->isVip(player) && sV->rateCustom) {
 //           amount *= sV->professionRate;
 //       }
 //   }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override {
        if (sV->isVip(player) && sV->rateCustom) {
            amount *= sV->rateXp;
        }
    }

    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override {
        if (sV->isVip(player) && sV->rateCustom) {
            loot->gold *= sV->goldRate;
        }
    }

    void OnPlayerReleasedGhost(Player* player) override {
        if (sV->isVip(player) && sV->ghostMount) {
            // Cast Mount Speed
            if (!player->HasAura(55164))
                player->AddAura(55164, player);

        }
    }

    void OnPlayerVictimRewardAfter(Player* player, Player* /*victim*/, uint32& /*killer_title*/, uint32& /*victim_rank*/, float& honor_f) override {
        if (sV->isVip(player) && sV->rateCustom) {
            if (sV->honorRate > 1) {
                player->ModifyHonorPoints((honor_f * sV->honorRate) - honor_f);
            }
        }
    }
};

class SystemVipVendor : public CreatureScript {
public:
    SystemVipVendor() : CreatureScript("SystemVipVendor") {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
		uint32 TimeVipDay = sConfigMgr->GetOption<uint32>("SystemVip.TimeVip", 7);
		std::string vipDaysStr = std::to_string(TimeVipDay);
        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/INV_Misc_Coin_02:28:28:0:0|t VIP会员资格.", 0, 1, "你想订阅" + vipDaysStr + "天的VIP系统吗?\n价格: " + to_string(sV->TokenAmount) + "\n " + sV->TokenIcon + " " +sV->getItemLink(sV->TokenEntry, player), 0, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/INV_Misc_Book_07:28:28:0:0|t VIP说明书.", 0, 2);
        if(sV->isVip(player))
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/ability_hunter_beastcall:28:28:0:0|t 找回我的VIP宠物.", 0, 4);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Trade_Engineering:28:28:0:0|t 关闭.", 0, 3);
        SendGossipMenuFor(player, 1, creature->GetGUID());
        return true;
    }


    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        ClearGossipMenuFor(player);
        switch (action)
        {
        case 1:
            if (player->HasItemCount(sV->TokenEntry, sV->TokenAmount)) {
                player->DestroyItemCount(sV->TokenEntry, sV->TokenAmount, true);
                sV->addRemainingVipTime(player);
                if (!player->HasItemCount(44830, 1, true)) {
                    player->AddItem(44830, 1);
                }
                ChatHandler(player->GetSession()).PSendSysMessage("感谢你的VIP订阅.");
                ChatHandler(player->GetSession()).PSendSysMessage("可用的VIP订阅时间: {}", sV->getFormatedVipTime(player).c_str());
                OnGossipSelect(player, creature, 0, 2);
            }
            else {
                ChatHandler(player->GetSession()).PSendSysMessage("你没有足够的代币.");
                CloseGossipMenuFor(player);
            }
            break;
        case 2:
            sV->sendGossipInformation(player, true);
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Trade_Engineering:28:28:0:0|t 关闭.", 0, 3);
            SendGossipMenuFor(player, VENDOR_INFO, creature->GetGUID());
            break;
        case 4:
            if (!player->HasItemCount(44830, 1, true)) {
                player->AddItem(44830, 1);
                creature->Whisper("请不要再弄丢它了.", LANG_UNIVERSAL, player, false);
                CloseGossipMenuFor(player);
            }
            else {
                creature->Whisper("你已经有一个用来召唤你的VIP宠物的物品了.", LANG_UNIVERSAL, player, false);
                OnGossipHello(player, creature);
            }
            break;
        default:
            CloseGossipMenuFor(player);
            break;
        }
        return true;
    }
};

class SystemVipPocket : ItemScript {
public:
    SystemVipPocket() : ItemScript("SystemVipPocket") {}

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/) {
        if (!sV->isVip(player)) {
            ChatHandler(player->GetSession()).PSendSysMessage("你还不是VIP!");
            ChatHandler(player->GetSession()).PSendSysMessage("请更新你的VIP订阅.");
            return false;
        }

        /*if (player->IsInCombat()) {
            ChatHandler(player->GetSession()).PSendSysMessage("Estas en combate!");
            return false;
        }*/

        if (player->GetMap()->IsBattleArena()) {
            ChatHandler(player->GetSession()).PSendSysMessage("你不能在竞技场中使用!");
            return false;
        }

        // Sound 3980
        // emote 73213
        player->CastSpell(player, 73213);
        player->PlayDistanceSound(3980, player);

        float distance = 20;
        float angle = player->GetOrientation() * M_PI / 180.0f;

        Creature* pet = player->SummonCreature(100002, player->GetPositionX() + (distance * cos(angle)), player->GetPositionY() + (distance * sin(angle)), player->GetPositionZ(), player->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 90000);//90second
        pet->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST + 2.0, PET_FOLLOW_ANGLE);
        pet->SetFaction(player->GetFaction());
        pet->SetLevel(player->GetLevel());
        pet->SetCreatorGUID(player->GetGUID());
        return false;
    }

};

class SystemVipPet : CreatureScript {
public:
    SystemVipPet() : CreatureScript("SystemVipPet") {}

    bool OnGossipHello(Player* player, Creature* creature) {
        ClearGossipMenuFor(player);
        sV->sendGossipInformation(player, false);
        if (creature->GetCreatorGUID() != player->GetGUID()) {
            return true;
        }

        if (!sV->petEnable) {
            return true;
        }
        if(sV->vipZone)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Achievement_Zone_ZulDrak_12:28:28:0:0|t VIP区域", 0, 1, "确定要传送VIP区域吗.", 0, false);
        if(sV->armorRep)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/INV_Hammer_20:28:28:0:0|t 修理护甲.", 0, 2);
        if(sV->bankEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/INV_Ingot_03:28:28:0:0|t 我的银行.", 0, 3);
        if(sV->mailEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/inv_letter_15:28:28:0:0|t 我的邮箱.", 0, 8);
        if (sV->buffsEnable) {
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Spell_Magic_GreaterBlessingofKings:28:28:0:0|t 增益效果", 0, 4);
            AddGossipItemFor(player, 0, "|TInterface/PAPERDOLLINFOFRAME/UI-GearManager-Undo:28:28:0:0|t 移除增益效果", 0, 11);
        }
        if(sV->refreshEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Spell_Holy_LayOnHands:28:28:0:0|t 恢复生命值/法力值.", 0, 5);
        if(sV->sicknessEnbale)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/spell_shadow_deathscream:28:28:0:0|t 移除疾病.", 0, 6);
        if(sV->deserterEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/ability_druid_cower:28:28:0:0|t 移除逃兵标记.", 0, 7);
        if(sV->resetInstance)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Achievement_Dungeon_Icecrown_IcecrownEntrance:28:28:0:0|t 重置副本.", 0, 9);
        if(sV->saveTeleport)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Spell_Holy_LightsGrace:28:28:0:0|t 我的传送.", 0, 10);
        AddGossipItemFor(player, 0, "|TInterface/ICONS/Trade_Engineering:28:28:0:0|t 关闭.", 0, 100);

        SendGossipMenuFor(player, PET_INFO, creature->GetGUID());
        return true;
    }
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) {
        ClearGossipMenuFor(player);
        switch (action)
        {
        case 1:
            if (player->IsInCombat()) {
                ChatHandler(player->GetSession()).PSendSysMessage("你在战斗中!");
                CloseGossipMenuFor(player);
            }
            else {
                player->TeleportTo(sV->vipZoneMapId, sV->vipZonePosX, sV->vipZonePosY, sV->vipZonePosZ, sV->vipZoneO);
                CloseGossipMenuFor(player);
            }
            break;
        case 2:
            player->DurabilityRepairAll(false, 0, false);
            ChatHandler(player->GetSession()).PSendSysMessage("你修理了你的护甲.");
            OnGossipHello(player, creature);
            break;
        case 3:
            player->GetSession()->SendShowBank(creature->GetGUID());
            break;
        case 4:
            for (size_t i = 0; i < sV->buffIds.size(); i++) {
                player->AddAura(sV->buffIds[i], player);
            }
            player->CastSpell(player, 16609);
            ChatHandler(player->GetSession()).PSendSysMessage("给你的增益效果!");
            OnGossipHello(player, creature);
            break;
        case 5:
            if (player->IsInCombat())
            {
                CloseGossipMenuFor(player);
                ChatHandler(player->GetSession()).PSendSysMessage("你在战斗中!");
                return false;
            }
            else if (player->getPowerType() == POWER_MANA)
                player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));

            player->SetHealth(player->GetMaxHealth());
            ChatHandler(player->GetSession()).PSendSysMessage("生命值/法力值已恢复!");
            creature->CastSpell(player, 31726, true);
            OnGossipHello(player, creature);
            break;
        case 6:
            if (player->HasAura(15007))
                player->RemoveAura(15007);
            ChatHandler(player->GetSession()).PSendSysMessage("你的疾病已被移除.");
            creature->CastSpell(player, 31726, true);
            OnGossipHello(player, creature);
            break;
        case 7:
            // remover desertor
            if(player->HasAura(26013))
                player->RemoveAura(26013);
            ChatHandler(player->GetSession()).PSendSysMessage("你的逃兵标记已被移除.");
            creature->CastSpell(player, 31726);
            OnGossipHello(player, creature);
            break;
        case 8:
            // mostrar correo
            CloseGossipMenuFor(player);
            player->GetSession()->SendShowMailBox(creature->GetGUID());
            break;
        case 9:
            // reiniciar cds
            for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            {
                BoundInstancesMap const& m_boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), Difficulty(i));
                for (BoundInstancesMap::const_iterator itr = m_boundInstances.begin(); itr != m_boundInstances.end();)
                {
                    if (itr->first != player->GetMapId())
                    {
                        sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), itr->first, Difficulty(i), true, player);
                        itr = m_boundInstances.begin();
                    }
                    else
                        ++itr;
                }
            }

            ChatHandler(player->GetSession()).PSendSysMessage("你的副本已被重置!");
            creature->CastSpell(player, 59908);
            OnGossipHello(player, creature);
            return true;
            break;
        case 11:
            for (size_t i = 0; i < sV->buffIds.size(); i++) {
                if (player->HasAura(sV->buffIds[i]))
                    player->RemoveAura(sV->buffIds[i]);
            }
            if (player->HasAura(16609))
                player->RemoveAura(16609);
            creature->CastSpell(player, 31726);
            ChatHandler(player->GetSession()).PSendSysMessage("给你的增益效果!");
            OnGossipHello(player, creature);
            break;
        case 10:
            // Sistema teleports
            AddGossipItemFor(player, 0, "|TInterface/GUILDBANKFRAME/UI-GuildBankFrame-NewTab:28:28:0:0|t 添加新的.", 0, 1, "用于保存您的坐标的名称.", 0, true);
            AddGossipItemFor(player, 0, "|TInterface/PAPERDOLLINFOFRAME/UI-GearManager-Undo:28:28:0:0|t 删除.", 0, 2, "要删除的名称.", 0, true);
            sV->getTeleports(player);
            SendGossipMenuFor(player, 1, creature->GetGUID());
            break;
        case 12:
            sV->teleportPlayer(player, sender);
            CloseGossipMenuFor(player);
			break;
        default:
            CloseGossipMenuFor(player);
            break;
        }
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 /*sender*/, uint32 action, const char* code) {
        switch (action)
        {
        case 1:
            sV->addTeleportVip(player, code);
            OnGossipSelect(player, creature, 0, 10);
            break;
        case 2:
            sV->delTeleportVip(player, code);
            OnGossipSelect(player, creature, 0, 10);
            break;
        default:
            CloseGossipMenuFor(player);
            break;
        }
        return true;
    }
};



class SystemVipWorld : WorldScript {
public:
    SystemVipWorld() : WorldScript("SystemVipWorld", {
        WORLDHOOK_ON_AFTER_CONFIG_LOAD
    }) {}

    virtual void OnAfterConfigLoad(bool /*reload*/) {
        sV->LoadConfig();
        sV->vipMap.clear();
        QueryResult result = LoginDatabase.Query("SELECT * FROM account_vip;");
        if (result) {
            do
            {
                sV->vipMap.emplace((*result)[0].Get<uint32>(), (*result)[1].Get<uint32>());
            } while (result->NextRow());
        }

        LOG_INFO("module", "Loading vip accounts... {} accounts loaded.", sV->vipMap.size());
    }
};
// Add all scripts in one
void Addsystem_vipScripts()
{
    new SystemVipPlayer();
    new SystemVipVendor();
    new SystemVipWorld();
    new SystemVipPocket();
    new SystemVipPet();
}
