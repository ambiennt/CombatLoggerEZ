#pragma once

#include <yaml.h>
#include <base/log.h>
#include <hook.h>
#include "base/playerdb.h"
#include <base/scheduler.h>
#include <Actor/Player.h>
#include <Actor/ActorDamageSource.h>
#include <Actor/ExperienceOrb.h>
#include <Level/Level.h>
#include <Level/GameRules.h>
#include <Level/DimensionID.h>
#include <Container/PlayerInventory.h>
#include <Packet/TextPacket.h>
#include <Net/ServerNetworkHandler.h>
#include <Item/ItemStack.h>
#include <Item/ItemRegistry.h>
#include <Item/Enchant.h>
#include <BlockActor/BlockActor.h>
#include <BlockActor/ChestBlockActor.h>
#include <Block/BlockSource.h>
#include <Block/VanillaBlocks.h>
#include <Math/Vec3.h>
#include <Math/BlockPos.h>
#include <mods/CommandSupport.h>

#include <boost/scope_exit.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

struct itemToAdd {

  	int32_t id                                       = 0;
  	int32_t aux                                      = 0;
  	int32_t count                                    = 0;
  	std::string customName                           = "";
  	std::vector<std::string> lore                    = {};
  	std::vector<std::map<int32_t, int32_t>> enchants = {};

  	template <typename IO> static inline bool io(IO f, itemToAdd &settings, YAML::Node &node) {
		return f(settings.id, node["id"]) &&
			   f(settings.aux, node["aux"]) &&
			   f(settings.count, node["count"]) &&
		   	   f(settings.customName, node["customName"]) &&
		   	   f(settings.lore, node["lore"]) &&
		   	   f(settings.enchants, node["enchants"]);
   }
};

namespace YAML {
template <> struct convert<itemToAdd> {
  	static Node encode(itemToAdd const& rhs) {
		Node node;
		node["id"]         = rhs.id;
		node["aux"]        = rhs.aux;
		node["count"]      = rhs.count;
		node["customName"] = rhs.customName;
		node["lore"]       = rhs.lore;
		node["enchants"]   = rhs.enchants;
		return node;
  	}

  	static bool decode(Node const& node, itemToAdd &rhs) {

		if (!node.IsMap()) { return false; }

		rhs.id         = node["id"].as<int32_t>();
		rhs.aux        = node["aux"].as<int32_t>();
		rhs.count      = node["count"].as<int32_t>();
		rhs.customName = node["customName"].as<std::string>();
		rhs.lore 	   = node["lore"].as<std::vector<std::string>>();
		rhs.enchants   = node["enchants"].as<std::vector<std::map<int32_t, int32_t>>>();
		return true;
  	}
};
} // namespace YAML

inline struct Settings {
	bool operatorsCanBeInCombat = true;
	int32_t combatTime = 30;
	bool combatTimeMessageEnabled = true;
	std::string initiatedCombatMessage = "You are now in combat. Do not log out!";
	std::string combatTimeMessage = "You are in combat for %time% more seconds!";
	std::string endedCombatMessage = "You are no longer in combat.";
	std::string logoutWhileInCombatMessage = "%name% logged out while in combat!";

	bool useResourcePackGlyphsInDeathMessage = false;
	bool executeDeathCommands = true;
	std::string deathCommand = "function death";
	std::string killerCommand = "function killer";
	bool setChestGravestoneOnLog = false;
	bool enableExtraItemsForChestGravestone = false;
	std::vector<itemToAdd> extraItems = {itemToAdd()};

	template <typename IO> static inline bool io(IO f, Settings &settings, YAML::Node &node) {
		return f(settings.operatorsCanBeInCombat, node["operatorsCanBeInCombat"]) &&
			   	f(settings.combatTime, node["combatTime"]) &&
			   	f(settings.combatTimeMessageEnabled, node["combatTimeMessageEnabled"]) &&
			   	f(settings.initiatedCombatMessage, node["initiatedCombatMessage"]) &&
			   	f(settings.combatTimeMessage, node["combatTimeMessage"]) &&
			   	f(settings.endedCombatMessage, node["endedCombatMessage"]) &&
			   	f(settings.logoutWhileInCombatMessage, node["logoutWhileInCombatMessage"]) &&
			   	f(settings.useResourcePackGlyphsInDeathMessage, node["useResourcePackGlyphsInDeathMessage"]) &&
			   	f(settings.executeDeathCommands, node["executeDeathCommands"]) &&
			   	f(settings.deathCommand, node["deathCommand"]) &&
			   	f(settings.killerCommand, node["killerCommand"]) &&
			   	f(settings.setChestGravestoneOnLog, node["setChestGravestoneOnLog"]) &&
			   	f(settings.enableExtraItemsForChestGravestone, node["enableExtraItemsForChestGravestone"]) &&
			   	f(settings.extraItems, node["extraItems"]);
	}
} settings;

namespace CombatLogger {

struct Combat {
	uint64_t xuid;
	int32_t time;
};

inline constexpr const char* HEALTH_GLYPH      = "\ue1fe"; // , glyph 0xE1FE
inline constexpr const char* ABSORPTION_GLYPH  = "\ue1ff"; // , glyph 0xE1FF
inline constexpr const char* HEALTH_ASCII      = "\u00a7\u0063\u2764\u00a7\u0072"; // §c❤§r
inline constexpr const char* ABSORPTION_ASCII  = "\u00a7\u0065\u2764\u00a7\u0072"; // §e❤§r

inline Mod::Scheduler::Token SCHEDULER_TOKEN;
inline std::unordered_map<uint64_t /*xuid*/, Combat> COMBAT_MAP;
inline bool IS_RUNNING = false;

inline std::unordered_map<uint64_t, Combat>& getInCombat() { return COMBAT_MAP; }
inline bool isInCombat(uint64_t xuid) { return getInCombat().count(xuid); }
inline void clearCombatStatus(uint64_t xuid) { getInCombat().erase(xuid); }
bool isInCombatWith(uint64_t thisXuid, uint64_t thatXuid);
void clearCombatTokenIfNeeded();
std::string dimIdToString(DimensionID d);
void handleCombatDeathSequence(Player &dead, Player *killer);
void dropPlayerInventory(Player &player);

} // namespace CombatLogger

namespace ChestGravestone {

bool isSafeBlock(const Block &block, bool isAboveBlock);
bool isSafeRegion(const BlockSource &region, int32_t leadX, int32_t leadY, int32_t leadZ);
std::pair<BlockPos, BlockPos> tryGetSafeChestGravestonePos(const Player &player);
void transferPlayerInventoryToChest(Player &player, Container &chestContainer);
void tryAddYAMLItemStacksToChest(Container &chestContainer);

} // namespace ChestGravestone

#define PLAYER_DB Mod::PlayerDatabase::GetInstance()
#define CMD_SUPPORT Mod::CommandSupport::GetInstance()

DEF_LOGGER("CombatLogger");