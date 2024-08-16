#include <stdint.h>
#include <string>
#include <stdio.h>

#include <vector>

#ifndef NAV_PROFILES_H
#define NAV_PROFILES_H

using std::string;
using std::vector;

typedef struct _MESH_DEFINITION
{
	string NavMeshName = "Nav Mesh";

	float AgentRadius = 16.0f;
	float AgentStandingHeight = 72.0f;
	float AgentCrouchingHeight = 32.0f;
	float MaxStep = 18.0f;
	float MaxSlope = 50.0f;
} NavMeshDefinition;

typedef struct _NAV_MOVE_DEF
{
	string ConnName = "New Connection Type";
	unsigned int ConnIndex = 0;
	unsigned int AreaIndex = 0;
	unsigned int FlagIndex = 0;
	bool bCustom = false;
} NavOffMeshConnectionDefinition;

typedef struct _NAV_AREA_DEF
{
	string AreaName = "New Area";
	string TechnicalName = "NAV_AREA_CUSTOM";
	unsigned int NavAreaIndex = 0;
	unsigned char AreaId = 0;
	unsigned int FlagIndex = 0;
	bool bCustom = false;
	float R = 0.0f;
	float G = 0.0f;
	float B = 0.0f;
	unsigned int DebugColor = 0;
} NavAreaDefinition;

typedef struct _NAV_FLAG_DEF
{
	string FlagName = "New Flag";
	string TechnicalName = "NAV_FLAG_CUSTOM";
	unsigned int NavFlagIndex = 0;
	unsigned int FlagId = 0;
	bool bCustom = false;
	float R = 0.0f;
	float G = 0.0f;
	float B = 0.0f;
	unsigned int DebugColor = 0;
} NavFlagDefinition;

typedef struct _AGENT_PROFILE
{
	string ProfileName = "New Profile";

	unsigned int NavMeshIndex = 0;

	int NumAreas = 0;
	float AreaCosts[32];

	unsigned int MovementFlags = 0;

} NavAgentProfile;

typedef struct _GAME_PROFILE
{
	string GameName = "Game";
	vector<NavAreaDefinition> AreaDefinitions;
	vector<NavFlagDefinition> FlagDefinitions;
	vector<NavOffMeshConnectionDefinition> ConnectionDefinitions;
	vector<NavAgentProfile> ProfileDefinitions;

	vector<NavMeshDefinition> MeshDefinitions;


} NavGameProfile;

vector<NavGameProfile> GetAllGameProfiles();

vector<NavAreaDefinition> GetAllNavAreaDefinitions();
vector<NavFlagDefinition> GetAllNavFlagDefinitions();
vector<NavOffMeshConnectionDefinition> GetAllConnectionDefinitions();
vector<NavMeshDefinition> GetAllMeshDefinitions();
vector<NavAgentProfile> GetAllAgentProfileDefinitions();

unsigned int GetCurrentGameProfileIndex();
void SetCurrentGameProfileIndex(unsigned int NewIndex);

NavGameProfile* CreateNewGameProfile();
NavAreaDefinition* CreateNewAreaDefinition();
NavFlagDefinition* CreateNewMovementFlag();
NavOffMeshConnectionDefinition* CreateConnectionType();
NavMeshDefinition* CreateNewNavMesh();
NavAgentProfile* CreateNewAgentProfile();

NavAreaDefinition* GetAreaAtIndex(unsigned int Index);
NavAreaDefinition* GetNavAreaById(unsigned short Id);
NavFlagDefinition* GetFlagAtIndex(unsigned int Index);
NavFlagDefinition* GetFlagByFlagId(unsigned int Id);
NavOffMeshConnectionDefinition* GetConnectionAtIndex(unsigned int Index);
NavMeshDefinition* GetMeshAtIndex(unsigned int Index);
NavAgentProfile* GetAgentProfileAtIndex(unsigned int Index);

int GetNumNavMeshes();

void DeleteGameProfile(unsigned int ProfileIndex);

NavGameProfile* GetCurrentGameProfile();

int GetIndexForNewArea();
int GetIndexForNewFlag();
int GetIndexForNewConnectionType();

bool CanRemoveMovementArea(unsigned int RemoveIndex);
bool CanRemoveMovementFlag(unsigned int RemoveIndex);
bool CanRemoveConnectionType(unsigned int RemoveIndex);

void RemoveMovementFlag(unsigned int RemoveIndex);
void RemoveMovementArea(unsigned int RemoveIndex);
void RemoveConnectionType(unsigned int RemoveIndex);
void RemoveMeshDefinition(unsigned int RemoveIndex);
void RemoveAgentProfile(unsigned int RemoveIndex);

string* GetCurrentlyModifiedString();
void SetCurrentlyModifiedString(string* NewModifiedString);
void EndModifyString();

void SetConnAreaModifier(int NewIndex);
void SetConnFlagModifier(int NewIndex);
void SetAreaFlagModifier(int NewIndex);
void SetProfileMeshModifier(int NewIndex);
void SetFlagColorModifier(unsigned int* NewColorIndex);

int GetProfileModifier();
void SetProfileModifier(int NewIndex);

bool GetShowFlagList();
bool GetShowConnAreaList();
int GetAreaFlagModifier();
int GetConnAreaModifier();
int GetConnFlagModifier();
int GetProfileMeshModifier();

unsigned int* GetFlagColorModifier();

void ResetProfileMenus();

void OutputProfileConfig(NavGameProfile* Profile);

#endif // NAV_PROFILES_H