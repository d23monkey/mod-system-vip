#include "SystemVip.h"

SystemVip* SystemVip::instance()
{
    static SystemVip instance;
    return &instance;
}

void SystemVip::LoadConfig() {
    TimeVip = sConfigMgr->GetOption<uint32>("SystemVip.TimeVip", 7) * 86400;
    TokenEntry = sConfigMgr->GetOption<uint32>("SystemVip.Token", 123);
    TokenAmount = sConfigMgr->GetOption<uint32>("SystemVip.TokenAmount", 10);
    TokenIcon = sConfigMgr->GetOption<string>("SystemVip.TokenIcon", "|TInterface/ICONS/icgoods171:25:25:-15:0|t ");

    loginAnnounce = sConfigMgr->GetOption<bool>("SystemVip.LoginAnnounce", false);
    loginMessage = sConfigMgr->GetOption<string>("SystemVip.LoginAnnounceMessage", "|CFFFF0099VIP玩家: %s 已上线.|r");
    rateCustom = sConfigMgr->GetOption<bool>("SystemVip.EnableRateCustom", false);
    rateXp = sConfigMgr->GetOption<uint32>("SystemVip.RateXP", 1);
    professionRate = sConfigMgr->GetOption<uint32>("SystemVip.ProfessionRate", 1);
    goldRate = sConfigMgr->GetOption<uint32>("SystemVip.GoldRate", 1);
    honorRate = sConfigMgr->GetOption<uint32>("SystemVip.HonorRate", 1);
    ghostMount = sConfigMgr->GetOption<bool>("SystemVip.GhostMount", false);

    petEnable = sConfigMgr->GetOption<bool>("SystemVip.Pet", false);
    vipZone = sConfigMgr->GetOption<bool>("SystemVip.VipZone", false);
    vipZoneMapId = sConfigMgr->GetOption<uint32>("SystemVip.VipZoneMapId", 571);
    vipZonePosX = sConfigMgr->GetOption<float>("SystemVip.VipZoneCoorX", 5804.15);
    vipZonePosY = sConfigMgr->GetOption<float>("SystemVip.VipZoneCoorY", 624.771);
    vipZonePosZ = sConfigMgr->GetOption<float>("SystemVip.VipZoneCoorZ", 647.767);
    vipZoneO = sConfigMgr->GetOption<float>("SystemVip.VipZoneOrien", 1.64);
    armorRep = sConfigMgr->GetOption<bool>("SystemVip.ArmorRep", false);
    bankEnable = sConfigMgr->GetOption<bool>("SystemVip.Bank", false);
    mailEnable = sConfigMgr->GetOption<bool>("SystemVip.Mail", false);
    buffsEnable = sConfigMgr->GetOption<bool>("SystemVip.Buffs", false);

    string buffString = sConfigMgr->GetOption<string>("SystemVip.BuffIds", "25898,48469,42995,48169,48073,48161,53307,23735,23736,23737,23738,23766,23767,23768,23769");
    stringstream ss(buffString);
    buffIds.clear();
    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        int num = std::stoi(substr);
        buffIds.push_back(num);
    }

    refreshEnable = sConfigMgr->GetOption<bool>("SystemVip.Refresh", false);
    sicknessEnbale = sConfigMgr->GetOption<bool>("SystemVip.Sickness", false);
    deserterEnable = sConfigMgr->GetOption<bool>("SystemVip.Deserter", false);
    resetInstance = sConfigMgr->GetOption<bool>("SystemVip.ResetInstance", false);
    saveTeleport = sConfigMgr->GetOption<bool>("SystemVip.SaveTeleport", false);
    saveTeleportAmount = sConfigMgr->GetOption<uint32>("SystemVip.SaveTeleportAmount", 5);
}

bool SystemVip::isVip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();

    if (vipMap.count(accountId) == 0)
        return false;

    return time(nullptr) < vipMap[accountId];
}

void SystemVip::addRemainingVipTime(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    if (isVip(player)) {
        vipMap[accountId] += TimeVip;
        LoginDatabase.Execute("UPDATE account_vip SET subscription_date = {} WHERE id = {};", vipMap[accountId], accountId);
    }
    else {
        vipMap.erase(accountId);
        vipMap.emplace(accountId, time(nullptr) + TimeVip);
        LoginDatabase.Execute("REPLACE INTO account_vip (id, subscription_date) VALUES ({}, {});", accountId, vipMap[accountId]);
    }
}

uint32 SystemVip::getRemainingVipTime(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    return time(nullptr) >= vipMap[accountId] ? 0 : vipMap[accountId] - time(nullptr);
}

string SystemVip::getFormatedVipTime(Player* player) {
    uint32 time = getRemainingVipTime(player);
    int minutes = time / 60;
    int hours = minutes / 60;
    int days = hours / 24;

    hours = hours % 24;
    minutes = minutes % 60;
    // int seconds = time % 60;

    string result = to_string(days) + "天 " + to_string(hours) + "小时 " + to_string(minutes) + "分钟.";
    return result;
}

string SystemVip::getItemLink(uint32 entry, Player* player) {
    const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    std::string name = temp->Name1;
    if (ItemLocale const* il = sObjectMgr->GetItemLocale(entry))
        ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

    std::ostringstream oss;
    oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
        "|Hitem:" << entry << ":0:0:0:0:0:0:0:0:0|h[" << name << "]|h|r";

    return oss.str();
}

string SystemVip::getInformationVip(Player* player) {
    std::ostringstream text;
    std::string accName;
    if (AccountMgr::GetName(player->GetSession()->GetAccountId(), accName))
        text << "剩余时间: |CFFFF0099" << getFormatedVipTime(player) << "|r\n";

    return text.str();
}

string SystemVip::getInformationAdavantages() {
    std::ostringstream text;
    text << "福利列表:\n";
    text << "----------------------------------\n";
    if(loginAnnounce)
        text << "|TInterface/ICONS/Spell_unused2:25:25:-10:-8|t 登录时的公告." << "\n";
    if (rateCustom) {
        text << "|TInterface/ICONS/Achievement_BG_KillXEnemies_GeneralsRoom:25:25:-10:-8|t 经验值速率  x " << rateXp << "\n";
        text << "|TInterface/ICONS/Achievement_BG_overcome500disadvantage:25:25:-10:-8|t 专业速率  x " << professionRate << "\n";
        text << "|TInterface/ICONS/Achievement_BG_ABshutout:25:25:-10:-8|t 获得金币速率  x " << goldRate << "\n";
        text << "|TInterface/ICONS/Achievement_BG_kill_carrier_opposing_flagroom:25:25:-10:-8|t 获得荣誉速率  x " << honorRate << "\n";
    }
    if(ghostMount)
        text << "|TInterface/ICONS/ability_vanish:25:25:-10:-8|t 作为幽灵时的速度." << "\n";
    if (petEnable) {
        text << "|TInterface/ICONS/ability_hunter_beastcall:25:25:-10:-8|t VIP宠物." << "\n";
        if(vipZone)
            text << "|TInterface/ICONS/Achievement_Zone_ZulDrak_12:25:25:-10:-8|t VIP区域." << "\n";
        if(armorRep)
            text << "|TInterface/ICONS/INV_Hammer_25:25:20:-10:-8|t 修理护甲." << "\n";
        if(bankEnable)
            text << "|TInterface/ICONS/INV_Ingot_03:25:25:-10:-8|t 个人银行." << "\n";
        if(mailEnable)
            text << "|TInterface/ICONS/inv_letter_15:25:25:-10:-8|t 个人邮箱." << "\n";
        if(buffsEnable)
            text << "|TInterface/ICONS/Spell_Magic_GreaterBlessingofKings:25:25:-10:-8|t VIP BUFF." << "\n";
        if(refreshEnable)
            text << "|TInterface/ICONS/Spell_Holy_LayOnHands:25:25:-10:-8|t 恢复生命、法力." << "\n";
        if(sicknessEnbale)
            text << "|TInterface/ICONS/spell_shadow_deathscream:25:25:-10:-8|t 移除减益状态." << "\n";
        if(deserterEnable)
            text << "|TInterface/ICONS/ability_druid_cower:25:25:-10:-8|t 移除逃兵标记." << "\n";
        if(resetInstance)
            text << "|TInterface/ICONS/Achievement_Dungeon_Icecrown_IcecrownEntrance:25:25:-10:-8|t 重置副本." << "\n";
        if(saveTeleport)
            text << "|TInterface/ICONS/Spell_Holy_LightsGrace:25:25:-10:-8|t 保存位置." << "\n";
    }
    return text.str();
}

void SystemVip::sendGossipInformation(Player* player, bool advantages) {
    std::ostringstream text;
    std::string accName;
    if (AccountMgr::GetName(player->GetSession()->GetAccountId(), accName))
        text << "用户(账号名): |CFF0E3CE6" << accName << "|r\n";

    if (isVip(player)) {
        text << "剩余时间: |CFFFF0099" << getFormatedVipTime(player) << "|r\n\n";
        text << "感谢您购买VIP订阅.\n\n";
    }
    else {
        text << "您没有可用的VIP订阅.\n";
        text << "购买一个订阅并享受成为VIP的所有福利!\n";
    }

    if (advantages) {
        text << "记住，购买VIP后，您账户中的所有角色都将享有VIP福利." << "\n";
        text << getInformationAdavantages();
    }

    WorldPacket data(384, 100);
    if(advantages)
        data << uint32_t(VENDOR_INFO); // id npc_text
    else
        data << uint32_t(PET_INFO); // id npc_text
    for (int i = 0; i < 10; ++i) {
        data << float(0.0f);
        data << std::string(text.str());
        data << std::string(text.str());
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
    }
    player->GetSession()->SendPacket(&data);
}

void SystemVip::delExpireVip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    if (vipMap.count(accountId)) {
        if (getRemainingVipTime(player) == 0) {
            vipMap.erase(accountId);
        }
    }
}


string SystemVip::getLoginMessage(Player* player) {
    string welcomeMessage = loginMessage;
    string name = player->GetName();
    size_t pos = welcomeMessage.find("%s");

    if (pos != std::string::npos) {
        welcomeMessage.replace(pos, 2, name);
    }
    return welcomeMessage;
}

void SystemVip::loadTeleportVip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    QueryResult result = LoginDatabase.Query("SELECT * FROM account_vip_teleport WHERE id = {};", accountId);
    if (result) {
        uint32 i = 1;
        do
        {
            Teleports teleport = { i, (*result)[1].Get<string>(), (*result)[2].Get<uint32>(), (*result)[3].Get<float>(), (*result)[4].Get<float>(), (*result)[5].Get<float>(), (*result)[6].Get<float>() };
            teleportMap[accountId].push_back(teleport);
            i++;
        } while (result->NextRow());
    }
}

void SystemVip::addTeleportVip(Player* player, string name) {
    uint32 accountId = player->GetSession()->GetAccountId();
    Teleports teleport = { 0, name, player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation() };
    uint32 id = 1;
    if (teleportMap.count(accountId) > 0) {
        if (teleportMap[accountId].size() == saveTeleportAmount) {
            ChatHandler(player->GetSession()).PSendSysMessage("你不能再保存更多的传送点!");
            return;
        }

        for (size_t i = 0; i < teleportMap[accountId].size(); i++) {
            if (teleportMap[accountId][i].name == teleport.name) {
                ChatHandler(player->GetSession()).PSendSysMessage("已经存在一个同名的传送点!");
                return;
            }
        }
        id = teleportMap[accountId].back().id + 1;
    }
    teleport.id = id;
    teleportMap[accountId].push_back(teleport);
    LoginDatabase.Execute("INSERT INTO account_vip_teleport VALUES ( {} , '{}', {}, {}, {}, {}, {} );", accountId, name, teleport.mapId, teleport.coord_x, teleport.coord_y, teleport.coord_z, teleport.orientation);
    ChatHandler(player->GetSession()).PSendSysMessage("当前位置保存成功.");
}

void SystemVip::delTeleportVip(Player* player, string name) {
    uint32 accountId = player->GetSession()->GetAccountId();
    for (size_t i = 0; i < teleportMap[accountId].size(); i++) {
        if (teleportMap[accountId][i].name == name) {
            teleportMap[accountId].erase(teleportMap[accountId].begin() + i);
            LoginDatabase.Execute("DELETE FROM account_vip_teleport WHERE id = {} AND name = '{}';", accountId, name);
            return;
        }
    }
    ChatHandler(player->GetSession()).PSendSysMessage("名称错误.");
}

void SystemVip::getTeleports(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    if( teleportMap.count(accountId) != 0){
        for (size_t i = 0; i < teleportMap[accountId].size(); i++) {
            AddGossipItemFor(player, 0, "|TInterface/CURSOR/Taxi:28:28:-15:0|t "+teleportMap[accountId][i].name, teleportMap[accountId][i].id, 12, "你想传送自己吗?", 0, false);
        }
    }
}

void SystemVip::teleportPlayer(Player* player, uint32 id) {
    uint32 accountId = player->GetSession()->GetAccountId();
    Teleports teleport;
    for (size_t i = 0; i < teleportMap[accountId].size(); i++) {
        if (teleportMap[accountId][i].id == id) {
            teleport = teleportMap[accountId][i];
            break;
        }
    }

    player->TeleportTo(teleport.mapId, teleport.coord_x, teleport.coord_y, teleport.coord_z, teleport.orientation);
}
