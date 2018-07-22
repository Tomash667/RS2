#pragma once

#include "EngineCore.h"
#include <thread>

class CityGenerator;
class Game;
class GameGui;
class GameState;
class Inventory;
class Level;
class MainMenu;
class Navmesh;
class Options;
class PickPerkDialog;
class StatsPanel;

struct Collider;
struct GroundItem;
struct Item;
struct Player;
struct ThirdPersonCamera;
struct Unit;
struct Zombie;

enum class PerkId;

// recastnavigation
typedef uint dtPolyRef;
class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;
struct dtMeshTile;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcHeightfield;
struct rcPolyMesh;
struct rcPolyMeshDetail;
