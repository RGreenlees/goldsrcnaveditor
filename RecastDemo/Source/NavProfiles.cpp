#include "NavProfiles.h"

#include <vector>
#include <fstream>
#include <sstream>

using std::vector;

vector<NavGameProfile> NavGameProfiles;

unsigned int CurrentGameProfileIndex = 0;

string* CurrentlyModifiedString = nullptr;

int AreaFlagModifier = -1;
int ConnAreaModifier = -1;
int ConnFlagModifier = -1;
int ProfileMeshModifier = -1;
int ProfileModifier = 0;

unsigned int* FlagColorModifier = nullptr;


unsigned int GetCurrentGameProfileIndex()
{
	return CurrentGameProfileIndex;
}

void SetCurrentGameProfileIndex(unsigned int NewIndex)
{
	if (NewIndex >= NavGameProfiles.size())
	{
		CurrentGameProfileIndex = 0;
	}
	else
	{
		CurrentGameProfileIndex = NewIndex;
	}
}

inline unsigned int duRGBA2(int r, int g, int b, int a)
{
	return ((unsigned int)r) | ((unsigned int)g << 8) | ((unsigned int)b << 16) | ((unsigned int)a << 24);
}

NavGameProfile* CreateNewBlankGameProfile()
{
	NavGameProfile NewProfile;

	NavGameProfiles.push_back(NewProfile);

	return &(*(prev(NavGameProfiles.end())));
}

NavGameProfile* CreateNewGameProfile()
{
	NavGameProfile NewProfile;
	NewProfile.GameName = "Game";
	
	NavAreaDefinition DefaultNullArea;
	DefaultNullArea.NavAreaIndex = 0;
	DefaultNullArea.AreaName = "Unwalkable";
	DefaultNullArea.TechnicalName = "NAV_AREA_UNWALKABLE";
	DefaultNullArea.AreaId = 60;
	DefaultNullArea.FlagIndex = 31;
	DefaultNullArea.bCustom = false;
	DefaultNullArea.R = 10.0f;
	DefaultNullArea.G = 10.0f;
	DefaultNullArea.B = 10.0f;
	DefaultNullArea.DebugColor = duRGBA2(10, 10, 10, 255);

	NavAreaDefinition DefaultWalkArea;
	DefaultWalkArea.NavAreaIndex = 1;
	DefaultWalkArea.AreaName = "Walk";
	DefaultWalkArea.TechnicalName = "NAV_AREA_WALK";
	DefaultWalkArea.AreaId = 1;
	DefaultWalkArea.FlagIndex = 1;
	DefaultWalkArea.bCustom = false;
	DefaultWalkArea.R = 0.0f;
	DefaultWalkArea.G = 192.0f;
	DefaultWalkArea.B = 255.0f;
	DefaultWalkArea.DebugColor = duRGBA2(0, 192, 255, 255);

	NavAreaDefinition DefaultCrouchArea;
	DefaultCrouchArea.NavAreaIndex = 2;
	DefaultCrouchArea.AreaName = "Crouch";
	DefaultCrouchArea.TechnicalName = "NAV_AREA_CROUCH";
	DefaultCrouchArea.AreaId = 2;
	DefaultCrouchArea.FlagIndex = 2;
	DefaultCrouchArea.bCustom = false;
	DefaultCrouchArea.R = 9.0f;
	DefaultCrouchArea.G = 130.0f;
	DefaultCrouchArea.B = 150.0f;
	DefaultCrouchArea.DebugColor = duRGBA2(9, 130, 150, 255);

	NavAreaDefinition DefaultObstructedArea;
	DefaultObstructedArea.NavAreaIndex = 3;
	DefaultObstructedArea.AreaName = "Obstructed";
	DefaultObstructedArea.TechnicalName = "NAV_AREA_OBSTRUCTED";
	DefaultObstructedArea.AreaId = 3;
	DefaultObstructedArea.FlagIndex = 3;
	DefaultObstructedArea.bCustom = false;
	DefaultObstructedArea.R = 255.0f;
	DefaultObstructedArea.G = 64.0f;
	DefaultObstructedArea.B = 64.0f;
	DefaultObstructedArea.DebugColor = duRGBA2(130, 130, 32, 255);

	NavAreaDefinition DefaultHazardArea;
	DefaultHazardArea.NavAreaIndex = 4;
	DefaultHazardArea.AreaName = "Hazard";
	DefaultHazardArea.TechnicalName = "NAV_AREA_HAZARD";
	DefaultHazardArea.AreaId = 4;
	DefaultHazardArea.FlagIndex = 1;
	DefaultHazardArea.bCustom = false;
	DefaultHazardArea.R = 192.0f;
	DefaultHazardArea.G = 32.0f;
	DefaultHazardArea.B = 32.0f;
	DefaultHazardArea.DebugColor = duRGBA2(192, 32, 32, 255);

	NavAreaDefinition DefaultTeleportArea;
	DefaultTeleportArea.NavAreaIndex = 5;
	DefaultTeleportArea.AreaName = "Teleport";
	DefaultTeleportArea.TechnicalName = "NAV_AREA_TELEPORT";
	DefaultTeleportArea.AreaId = 5;
	DefaultTeleportArea.FlagIndex = 6;
	DefaultTeleportArea.bCustom = false;
	DefaultTeleportArea.R = 255.0f;
	DefaultTeleportArea.G = 255.0f;
	DefaultTeleportArea.B = 255.0f;
	DefaultTeleportArea.DebugColor = duRGBA2(255, 255, 255, 255);

	NewProfile.AreaDefinitions.push_back(DefaultNullArea);
	NewProfile.AreaDefinitions.push_back(DefaultWalkArea);
	NewProfile.AreaDefinitions.push_back(DefaultCrouchArea);
	NewProfile.AreaDefinitions.push_back(DefaultObstructedArea);
	NewProfile.AreaDefinitions.push_back(DefaultHazardArea);
	NewProfile.AreaDefinitions.push_back(DefaultTeleportArea);

	NavFlagDefinition DisabledFlagDef;
	DisabledFlagDef.NavFlagIndex = 31;
	DisabledFlagDef.FlagName = "Disabled";
	DisabledFlagDef.TechnicalName = "NAV_FLAG_DISABLED";
	DisabledFlagDef.FlagId = 1 << 31;
	DisabledFlagDef.bCustom = false;
	DisabledFlagDef.R = 8.0f;
	DisabledFlagDef.G = 8.0f;
	DisabledFlagDef.B = 8.0f;
	DisabledFlagDef.DebugColor = duRGBA2(8, 8, 8, 255);

	NavFlagDefinition WalkFlagDef;
	WalkFlagDef.NavFlagIndex = 0;
	WalkFlagDef.FlagName = "Walk";
	WalkFlagDef.TechnicalName = "NAV_FLAG_WALK";
	WalkFlagDef.FlagId = 1 << 0;
	WalkFlagDef.bCustom = false;
	WalkFlagDef.R = 0.0f;
	WalkFlagDef.G = 192.0f;
	WalkFlagDef.B = 255.0f;
	WalkFlagDef.DebugColor = duRGBA2(0, 192, 255, 255);

	NavFlagDefinition CrouchFlagDef;
	CrouchFlagDef.NavFlagIndex = 1;
	CrouchFlagDef.FlagName = "Crouch";
	CrouchFlagDef.TechnicalName = "NAV_FLAG_CROUCH";
	CrouchFlagDef.FlagId = 1 << 1;
	CrouchFlagDef.bCustom = false;
	CrouchFlagDef.R = 9.0f;
	CrouchFlagDef.G = 130.0f;
	CrouchFlagDef.B = 150.0f;
	CrouchFlagDef.DebugColor = duRGBA2(9, 130, 150, 255);

	NavFlagDefinition JumpFlagDef;
	JumpFlagDef.NavFlagIndex = 2;
	JumpFlagDef.FlagName = "Jump";
	JumpFlagDef.TechnicalName = "NAV_FLAG_JUMP";
	JumpFlagDef.FlagId = 1 << 2;
	JumpFlagDef.bCustom = false;
	JumpFlagDef.R = 200.0f;
	JumpFlagDef.G = 200.0f;
	JumpFlagDef.B = 0.0f;
	JumpFlagDef.DebugColor = duRGBA2(200, 200, 0, 255);

	NavFlagDefinition LadderFlagDef;
	LadderFlagDef.NavFlagIndex = 3;
	LadderFlagDef.FlagName = "Ladder";
	LadderFlagDef.TechnicalName = "NAV_FLAG_LADDER";
	LadderFlagDef.FlagId = 1 << 3;
	LadderFlagDef.bCustom = false;
	LadderFlagDef.R = 64.0f;
	LadderFlagDef.G = 64.0f;
	LadderFlagDef.B = 255.0f;
	LadderFlagDef.DebugColor = duRGBA2(64, 64, 255, 255);

	NavFlagDefinition FallFlagDef;
	FallFlagDef.NavFlagIndex = 4;
	FallFlagDef.FlagName = "Fall";
	FallFlagDef.TechnicalName = "NAV_FLAG_FALL";
	FallFlagDef.FlagId = 1 << 4;
	FallFlagDef.bCustom = false;
	FallFlagDef.R = 255.0f;
	FallFlagDef.G = 64.0f;
	FallFlagDef.B = 64.0f;
	FallFlagDef.DebugColor = duRGBA2(255, 64, 64, 255);

	NavFlagDefinition PlatformFlagDef;
	PlatformFlagDef.NavFlagIndex = 5;
	PlatformFlagDef.FlagName = "Platform";
	PlatformFlagDef.TechnicalName = "NAV_FLAG_PLATFORM";
	PlatformFlagDef.FlagId = 1 << 5;
	PlatformFlagDef.bCustom = false;
	PlatformFlagDef.R = 255.0f;
	PlatformFlagDef.G = 32.0f;
	PlatformFlagDef.B = 255.0f;
	PlatformFlagDef.DebugColor = duRGBA2(255, 32, 255, 255);

	NavFlagDefinition TeleportFlagDef;
	TeleportFlagDef.NavFlagIndex = 6;
	TeleportFlagDef.FlagName = "Teleport";
	TeleportFlagDef.TechnicalName = "NAV_FLAG_TELEPORT";
	TeleportFlagDef.bTeleport = true;
	TeleportFlagDef.FlagId = 1 << 6;
	TeleportFlagDef.bCustom = false;
	TeleportFlagDef.R = 255.0f;
	TeleportFlagDef.G = 255.0f;
	TeleportFlagDef.B = 255.0f;
	TeleportFlagDef.DebugColor = duRGBA2(255, 255, 255, 255);

	NewProfile.FlagDefinitions.push_back(DisabledFlagDef);
	NewProfile.FlagDefinitions.push_back(WalkFlagDef);
	NewProfile.FlagDefinitions.push_back(CrouchFlagDef);
	NewProfile.FlagDefinitions.push_back(JumpFlagDef);
	NewProfile.FlagDefinitions.push_back(LadderFlagDef);
	NewProfile.FlagDefinitions.push_back(FallFlagDef);
	NewProfile.FlagDefinitions.push_back(PlatformFlagDef);
	NewProfile.FlagDefinitions.push_back(TeleportFlagDef);

	NavOffMeshConnectionDefinition WalkMoveDef;
	WalkMoveDef.ConnIndex = 0;
	WalkMoveDef.ConnName = "Walk";
	WalkMoveDef.AreaIndex = 1;
	WalkMoveDef.FlagIndex = 0;
	WalkMoveDef.bCustom = false;

	NavOffMeshConnectionDefinition CrouchMoveDef;
	CrouchMoveDef.ConnIndex = 1;
	CrouchMoveDef.ConnName = "Crouch";
	CrouchMoveDef.AreaIndex = 2;
	CrouchMoveDef.FlagIndex = 1;
	CrouchMoveDef.bCustom = false;

	NavOffMeshConnectionDefinition JumpMoveDef;
	JumpMoveDef.ConnIndex = 2;
	JumpMoveDef.ConnName = "Jump";
	JumpMoveDef.AreaIndex = 1;
	JumpMoveDef.FlagIndex = 2;
	JumpMoveDef.bCustom = false;

	NavOffMeshConnectionDefinition LadderMoveDef;
	LadderMoveDef.ConnIndex = 3;
	LadderMoveDef.ConnName = "Ladder";
	LadderMoveDef.AreaIndex = 1;
	LadderMoveDef.FlagIndex = 3;
	LadderMoveDef.bCustom = false;

	NavOffMeshConnectionDefinition FallMoveDef;
	FallMoveDef.ConnIndex = 4;
	FallMoveDef.ConnName = "Fall";
	FallMoveDef.AreaIndex = 1;
	FallMoveDef.FlagIndex = 4;
	FallMoveDef.bCustom = false;

	NavOffMeshConnectionDefinition PlatformMoveDef;
	PlatformMoveDef.ConnIndex = 5;
	PlatformMoveDef.ConnName = "Platform";
	PlatformMoveDef.AreaIndex = 1;
	PlatformMoveDef.FlagIndex = 5;
	PlatformMoveDef.bCustom = false;

	NavOffMeshConnectionDefinition TeleportMoveDef;
	TeleportMoveDef.ConnIndex = 6;
	TeleportMoveDef.ConnName = "Teleport";
	TeleportMoveDef.AreaIndex = 5;
	TeleportMoveDef.FlagIndex = 6;
	TeleportMoveDef.bCustom = false;

	NewProfile.ConnectionDefinitions.push_back(WalkMoveDef);
	NewProfile.ConnectionDefinitions.push_back(CrouchMoveDef);
	NewProfile.ConnectionDefinitions.push_back(JumpMoveDef);
	NewProfile.ConnectionDefinitions.push_back(LadderMoveDef);
	NewProfile.ConnectionDefinitions.push_back(FallMoveDef);
	NewProfile.ConnectionDefinitions.push_back(PlatformMoveDef);
	NewProfile.ConnectionDefinitions.push_back(TeleportMoveDef);
		
	NavMeshDefinition DefaultMesh;
	NewProfile.MeshDefinitions.push_back(DefaultMesh);

	NavAgentProfile DefaultProfile;
	DefaultProfile.ProfileName = "Base Player";
	DefaultProfile.MovementFlags |= WalkFlagDef.FlagId;
	DefaultProfile.MovementFlags |= CrouchFlagDef.FlagId;
	DefaultProfile.MovementFlags |= JumpFlagDef.FlagId;
	DefaultProfile.MovementFlags |= FallFlagDef.FlagId;
	DefaultProfile.MovementFlags |= LadderFlagDef.FlagId;
	DefaultProfile.MovementFlags |= PlatformFlagDef.FlagId;
	DefaultProfile.MovementFlags |= TeleportFlagDef.FlagId;
	
	for (int i = 0; i < 32; i++)
	{
		DefaultProfile.AreaCosts[i] = 1.0f;
	}

	DefaultProfile.AreaCosts[2] = 2.0f;
	DefaultProfile.AreaCosts[3] = 2.0f;
	DefaultProfile.AreaCosts[4] = 10.0f;
	DefaultProfile.AreaCosts[5] = 0.1f;

	NewProfile.ProfileDefinitions.push_back(DefaultProfile);

	NavGameProfiles.push_back(NewProfile);

	return &(*(prev(NavGameProfiles.end())));
}

NavAreaDefinition* CreateNewAreaDefinition()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	int NewAreaIndex = GetIndexForNewArea();

	NavAreaDefinition NewAreaDef;
	NewAreaDef.AreaName = "New Area";
	NewAreaDef.TechnicalName = "NAV_AREA_CUSTOM";
	NewAreaDef.AreaId = NewAreaIndex;
	NewAreaDef.NavAreaIndex = NewAreaIndex;
	NewAreaDef.bCustom = true;

	CurrentProfile->AreaDefinitions.push_back(NewAreaDef);

	return &(*(prev(CurrentProfile->AreaDefinitions.end())));
}

int GetIndexForNewNavHint()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return 0; }

	int Result = 0;
	bool bFound = false;

	while (!bFound)
	{
		bFound = true;
		for (auto it = CurrentProfile->NavHints.begin(); it != CurrentProfile->NavHints.end(); it++)
		{
			if (it->HintIndex == Result)
			{
				bFound = false;
				Result++;
				break;
			}
		}
	}

	return Result;
}

int GetIndexForNewFlag()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return 0; }

	int Result = 0;
	bool bFound = false;

	while (!bFound)
	{
		bFound = true;
		for (auto it = CurrentProfile->FlagDefinitions.begin(); it != CurrentProfile->FlagDefinitions.end(); it++)
		{
			if (it->NavFlagIndex == Result)
			{
				bFound = false;
				Result++;
				break;
			}
		}
	}

	return Result;
}

int GetIndexForNewConnectionType()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return 0; }

	int Result = 0;
	bool bFound = false;

	while (!bFound)
	{
		bFound = true;
		for (auto it = CurrentProfile->ConnectionDefinitions.begin(); it != CurrentProfile->ConnectionDefinitions.end(); it++)
		{
			if (it->ConnIndex == Result)
			{
				bFound = false;
				Result++;
				break;
			}
		}
	}

	return Result;
}


int GetIndexForNewArea()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return 0; }

	int Result = 0;
	bool bFound = false;

	while (!bFound)
	{
		bFound = true;
		for (auto it = CurrentProfile->AreaDefinitions.begin(); it != CurrentProfile->AreaDefinitions.end(); it++)
		{
			if (it->NavAreaIndex == Result)
			{
				bFound = false;
				Result++;
				break;
			}
		}
	}

	return Result;
}

NavFlagDefinition* GetFlagAtIndex(unsigned int Index)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	for (auto it = CurrentProfile->FlagDefinitions.begin(); it != CurrentProfile->FlagDefinitions.end(); it++)
	{
		if (it->NavFlagIndex == Index)
		{
			return &(*it);
		}
	}

	return nullptr;
}

NavAreaDefinition* GetAreaAtIndex(unsigned int Index)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	for (auto it = CurrentProfile->AreaDefinitions.begin(); it != CurrentProfile->AreaDefinitions.end(); it++)
	{
		if (it->NavAreaIndex == Index)
		{
			return &(*it);
		}
	}

	return nullptr;
}

NavAgentProfile* GetAgentProfileAtIndex(unsigned int Index)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	if (Index < 0 || Index >= CurrentProfile->ProfileDefinitions.size()) { return nullptr; }

	return &CurrentProfile->ProfileDefinitions[Index];
}

NavHintDefinition* GetNavHintAtIndex(unsigned int Index)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	if (Index < 0 || Index >= CurrentProfile->NavHints.size()) { return nullptr; }

	return &CurrentProfile->NavHints[Index];
}

NavMeshDefinition* GetMeshAtIndex(unsigned int Index)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	if (Index < 0 || Index >= CurrentProfile->MeshDefinitions.size()) { return nullptr; }

	return &CurrentProfile->MeshDefinitions[Index];
}

int GetNumNavMeshes()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return 0; }

	return CurrentProfile->MeshDefinitions.size();
}

int GetNumAgentProfiles()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return 0; }

	return CurrentProfile->ProfileDefinitions.size();
}

NavOffMeshConnectionDefinition* GetConnectionAtIndex(unsigned int Index)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	if (Index < 0 || Index >= CurrentProfile->ConnectionDefinitions.size()) { return nullptr; }

	return &CurrentProfile->ConnectionDefinitions[Index];
}

vector<NavAreaDefinition> GetAllNavAreaDefinitions()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return vector<NavAreaDefinition>(); }

	return CurrentProfile->AreaDefinitions;
}

vector<NavFlagDefinition> GetAllNavFlagDefinitions()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return vector<NavFlagDefinition>(); }

	return CurrentProfile->FlagDefinitions;
}

vector<NavOffMeshConnectionDefinition> GetAllConnectionDefinitions()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return vector<NavOffMeshConnectionDefinition>(); }

	return CurrentProfile->ConnectionDefinitions;
}

vector<NavMeshDefinition> GetAllMeshDefinitions()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return vector<NavMeshDefinition>(); }

	return CurrentProfile->MeshDefinitions;
}

vector<NavAgentProfile> GetAllAgentProfileDefinitions()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return vector<NavAgentProfile>(); }

	return CurrentProfile->ProfileDefinitions;
}

vector<NavHintDefinition> GetAllNavHintDefinitions()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return vector<NavHintDefinition>(); }

	return CurrentProfile->NavHints;
}

NavHintDefinition* CreateNewNavHintType()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	int NewHintIndex = GetIndexForNewNavHint();

	NavHintDefinition NewHintDef;
	NewHintDef.HintIndex = NewHintIndex;
	NewHintDef.HintName = "Custom Hint";
	NewHintDef.TechnicalName = "NAV_HINT_CUSTOM";
	NewHintDef.HintId = 1 << NewHintIndex;

	CurrentProfile->NavHints.push_back(NewHintDef);

	return &(*(prev(CurrentProfile->NavHints.end())));
}

NavFlagDefinition* CreateNewMovementFlag()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	int NewFlagIndex = GetIndexForNewFlag();

	NavFlagDefinition NewFlagDef;
	NewFlagDef.NavFlagIndex = NewFlagIndex;
	NewFlagDef.FlagName = "New Flag";
	NewFlagDef.TechnicalName = "NAV_FLAG_CUSTOM";
	NewFlagDef.FlagId = 1 << NewFlagIndex;
	NewFlagDef.bCustom = true;

	CurrentProfile->FlagDefinitions.push_back(NewFlagDef);

	return &(*(prev(CurrentProfile->FlagDefinitions.end())));
}

bool CanRemoveMovementArea(unsigned int RemoveIndex)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return false; }

	if (RemoveIndex >= CurrentProfile->AreaDefinitions.size()) { return false; }

	return CurrentProfile->AreaDefinitions[RemoveIndex].bCustom;
}

bool CanRemoveMovementFlag(unsigned int RemoveIndex)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return false; }

	if (RemoveIndex >= CurrentProfile->FlagDefinitions.size()) { return false; }

	return CurrentProfile->FlagDefinitions[RemoveIndex].bCustom;
}

bool CanRemoveConnectionType(unsigned int RemoveIndex)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return false; }

	if (RemoveIndex >= CurrentProfile->ConnectionDefinitions.size()) { return false; }

	return CurrentProfile->ConnectionDefinitions[RemoveIndex].bCustom;
}

void RemoveMovementArea(unsigned int RemoveIndex)
{
	if (!CanRemoveMovementArea(RemoveIndex)) { return; }

	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return; }

	CurrentProfile->AreaDefinitions.erase((CurrentProfile->AreaDefinitions.begin() + RemoveIndex));
}

void RemoveMovementFlag(unsigned int RemoveIndex)
{
	if (!CanRemoveMovementFlag(RemoveIndex)) { return; }

	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return; }

	CurrentProfile->FlagDefinitions.erase((CurrentProfile->FlagDefinitions.begin() + RemoveIndex));
}

void RemoveAgentProfile(unsigned int RemoveIndex)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile || CurrentProfile->ProfileDefinitions.size() <= 1 || RemoveIndex >= CurrentProfile->ProfileDefinitions.size()) { return; }

	CurrentProfile->ProfileDefinitions.erase((CurrentProfile->ProfileDefinitions.begin() + RemoveIndex));
}

void RemoveMeshDefinition(unsigned int RemoveIndex)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile || CurrentProfile->MeshDefinitions.size() <= 1 || RemoveIndex >= CurrentProfile->MeshDefinitions.size()) { return; }

	CurrentProfile->MeshDefinitions.erase((CurrentProfile->MeshDefinitions.begin() + RemoveIndex));
}

void RemoveHintType(unsigned int RemoveIndex)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return; }

	if (RemoveIndex >= CurrentProfile->NavHints.size()) { return; }

	CurrentProfile->NavHints.erase((CurrentProfile->NavHints.begin() + RemoveIndex));
}

void RemoveConnectionType(unsigned int RemoveIndex)
{
	if (!CanRemoveConnectionType(RemoveIndex)) { return; }

	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return; }

	CurrentProfile->ConnectionDefinitions.erase((CurrentProfile->ConnectionDefinitions.begin() + RemoveIndex));
}

NavMeshDefinition* CreateNewNavMesh()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	NavMeshDefinition NewMeshDef;

	CurrentProfile->MeshDefinitions.push_back(NewMeshDef);

	return &(*(prev(CurrentProfile->MeshDefinitions.end())));
}

NavAgentProfile* CreateNewAgentProfile()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	NavAgentProfile NewAgentProfile;

	vector<NavFlagDefinition> AllFlags = GetAllNavFlagDefinitions();

	for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
	{
		if (!it->bCustom)
		{
			NewAgentProfile.MovementFlags |= it->FlagId;
		}
	}

	for (int i = 0; i < 32; i++)
	{
		NewAgentProfile.AreaCosts[i] = 1.0f;
	}

	CurrentProfile->ProfileDefinitions.push_back(NewAgentProfile);

	return &(*(prev(CurrentProfile->ProfileDefinitions.end())));
}

NavOffMeshConnectionDefinition* CreateConnectionType()
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	int NewConnectionIndex = GetIndexForNewConnectionType();

	NavOffMeshConnectionDefinition NewMoveDef;
	NewMoveDef.ConnIndex = NewConnectionIndex;
	NewMoveDef.ConnName = "New Connection Type";
	NewMoveDef.AreaIndex = 0;
	NewMoveDef.FlagIndex = 0;
	NewMoveDef.bCustom = true;

	CurrentProfile->ConnectionDefinitions.push_back(NewMoveDef);

	return &(*(prev(CurrentProfile->ConnectionDefinitions.end())));
}

NavGameProfile* GetCurrentGameProfile()
{
	if (NavGameProfiles.size() == 0)
	{
		CurrentGameProfileIndex = 0;
		return nullptr;
	}

	if (CurrentGameProfileIndex > NavGameProfiles.size() - 1)
	{
		CurrentGameProfileIndex = NavGameProfiles.size() - 1;
	}

	return &(*(NavGameProfiles.begin() + CurrentGameProfileIndex));
}

vector<NavGameProfile> GetAllGameProfiles()
{
	return NavGameProfiles;
}

void DeleteGameProfile(unsigned int ProfileIndex)
{
	if (ProfileIndex >= NavGameProfiles.size()) { return; }

	NavGameProfiles.erase(NavGameProfiles.begin() + ProfileIndex);

	if (CurrentGameProfileIndex == ProfileIndex)
	{
		if (CurrentGameProfileIndex > 0)
		{
			CurrentGameProfileIndex--;
		}
	}
}

string* GetCurrentlyModifiedString()
{
	return CurrentlyModifiedString;
}

void SetCurrentlyModifiedString(string* NewModifiedString)
{
	CurrentlyModifiedString = NewModifiedString;
}

void EndModifyString()
{
	CurrentlyModifiedString = nullptr;
}

void SetProfileModifier(int NewIndex)
{
	ResetProfileMenus();
	ProfileModifier = NewIndex;
}

int GetProfileModifier()
{
	return ProfileModifier;
}

void SetAreaFlagModifier(int NewIndex)
{
	ResetProfileMenus();
	AreaFlagModifier = NewIndex;
}

void SetConnAreaModifier(int NewIndex)
{
	ResetProfileMenus();
	ConnAreaModifier = NewIndex;
}

void SetConnFlagModifier(int NewIndex)
{
	ResetProfileMenus();
	ConnFlagModifier = NewIndex;
}

void SetProfileMeshModifier(int NewIndex)
{
	ResetProfileMenus();
	ProfileMeshModifier = NewIndex;
}

bool GetShowFlagList()
{
	return AreaFlagModifier > -1;
}

bool GetShowConnAreaList()
{
	return ConnAreaModifier > -1;
}

int GetAreaFlagModifier()
{
	return AreaFlagModifier;
}

int GetConnAreaModifier()
{
	return ConnAreaModifier;
}

int GetConnFlagModifier()
{
	return ConnFlagModifier;
}

int GetProfileMeshModifier()
{
	return ProfileMeshModifier;
}

unsigned int* GetFlagColorModifier()
{
	return FlagColorModifier;
}

void ResetProfileMenus()
{
	EndModifyString();
	AreaFlagModifier = -1;
	ConnAreaModifier = -1;
	ConnFlagModifier = -1;

	FlagColorModifier = nullptr;
}

NavAreaDefinition* GetNavAreaById(unsigned short Id)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	for (auto it = CurrentProfile->AreaDefinitions.begin(); it != CurrentProfile->AreaDefinitions.end(); it++)
	{
		if (it->AreaId == Id)
		{
			return &(*it);
		}
	}

	return nullptr;
}

NavFlagDefinition* GetFlagByFlagId(unsigned int Id)
{
	NavGameProfile* CurrentProfile = GetCurrentGameProfile();

	if (!CurrentProfile) { return nullptr; }

	for (auto it = CurrentProfile->FlagDefinitions.begin(); it != CurrentProfile->FlagDefinitions.end(); it++)
	{
		if (it->FlagId == Id)
		{
			return &(*it);
		}
	}

	return nullptr;
}

void SetFlagColorModifier(unsigned int* NewColorIndex)
{
	FlagColorModifier = NewColorIndex;
}

// trim from start (in place)
inline void ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}));
}

// trim from end (in place)
inline void rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}).base(), s.end());
}

inline void trim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}));

	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}).base(), s.end());
}

void LoadProfileConfig(string ProfileName)
{
	if (ProfileName.empty()) { return; }

	const char* filename = ProfileName.c_str();

	std::ifstream cFile(filename);
	if (cFile.is_open())
	{
		NavGameProfile* LoadedProfile = CreateNewBlankGameProfile();

		void* CurrentItem = nullptr;
		int CurrentItemIndex = 0;

		// 1 = flags, 2 = areas, 3 = connections, 4 = meshes, 5 = agent profiles, 6 = Nav Hints
		int mode = 0;

		std::string line;
		int CurrSkillIndex = -1;
		bool bSkillLevelWarningGiven = false;

		while (getline(cFile, line))
		{
			if (line.empty()) { continue; }

			trim(line);

			auto delimiterPos = line.find(":");

			if (delimiterPos == std::string::npos) { continue; }

			string key = line.substr(0, delimiterPos);
			string value = line.substr(delimiterPos + 1);

			trim(key);
			trim(value);

			auto keyChar = key.c_str();
			auto valueChar = value.c_str();

			if (!_stricmp(keyChar, "game_name"))
			{
				LoadedProfile->GameName = valueChar;
				continue;
			}

			if (!_stricmp(keyChar, "game_directory"))
			{
				LoadedProfile->GameDirectory = valueChar;
				continue;
			}

			if (!_stricmp(keyChar, "flags"))
			{
				mode = 1;
				continue;
			}

			if (!_stricmp(keyChar, "areas"))
			{
				mode = 2;
				continue;
			}

			if (!_stricmp(keyChar, "connections"))
			{
				mode = 3;
				continue;
			}

			if (!_stricmp(keyChar, "meshes"))
			{
				mode = 4;
				continue;
			}

			if (!_stricmp(keyChar, "agent_profiles"))
			{
				mode = 5;
				continue;
			}

			if (!_stricmp(keyChar, "nav_hints"))
			{
				mode = 6;
				continue;
			}

			if (!_stricmp(keyChar, "id"))
			{
				CurrentItemIndex = (int)atoi(valueChar);

				switch (mode)
				{
					case 1:
					{
						NavFlagDefinition NewFlag;
						NewFlag.NavFlagIndex = CurrentItemIndex;
						LoadedProfile->FlagDefinitions.push_back(NewFlag);
						CurrentItem = &(*prev(LoadedProfile->FlagDefinitions.end()));
					}
					break;
					case 2:
					{
						NavAreaDefinition NewArea;
						NewArea.NavAreaIndex = CurrentItemIndex;
						LoadedProfile->AreaDefinitions.push_back(NewArea);
						CurrentItem = &(*prev(LoadedProfile->AreaDefinitions.end()));
					}
					break;
					case 3:
					{
						NavOffMeshConnectionDefinition NewConnection;
						NewConnection.ConnIndex = CurrentItemIndex;
						LoadedProfile->ConnectionDefinitions.push_back(NewConnection);
						CurrentItem = &(*prev(LoadedProfile->ConnectionDefinitions.end()));
					}
					break;
					case 4:
					{
						NavMeshDefinition NewMesh;
						LoadedProfile->MeshDefinitions.push_back(NewMesh);
						CurrentItem = &(*prev(LoadedProfile->MeshDefinitions.end()));
					}
					break;
					case 5:
					{
						NavAgentProfile NewProfile;
						for (int i = 0; i < 32; i++)
						{
							NewProfile.AreaCosts[i] = 1.0f;
						}
						LoadedProfile->ProfileDefinitions.push_back(NewProfile);
						CurrentItem = &(*prev(LoadedProfile->ProfileDefinitions.end()));
					}
					break;
					case 6:
					{
						NavHintDefinition NewHint;
						LoadedProfile->NavHints.push_back(NewHint);
						CurrentItem = &(*prev(LoadedProfile->NavHints.end()));
					}
					break;
					default:
						break;
				}

				continue;
			}

			if (mode == 1)
			{
				NavFlagDefinition* CurrFlag = (NavFlagDefinition*)CurrentItem;

				if (!CurrFlag) { continue; }

				if (!_stricmp(keyChar, "flag_index"))
				{
					CurrFlag->NavFlagIndex = (unsigned int)atoi(valueChar);
					CurrFlag->FlagId = 1 << CurrFlag->NavFlagIndex;
					continue;
				}

				if (!_stricmp(keyChar, "flag_name"))
				{
					CurrFlag->FlagName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "flag_tech_name"))
				{
					CurrFlag->TechnicalName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "is_teleport"))
				{
					CurrFlag->bTeleport = (!_stricmp(valueChar, "true"));
					continue;
				}

				if (!_stricmp(keyChar, "is_custom"))
				{
					CurrFlag->bCustom = (!_stricmp(valueChar, "true"));
					continue;
				}

				if (!_stricmp(keyChar, "R"))
				{
					CurrFlag->R = (float)atof(valueChar);
					CurrFlag->DebugColor = duRGBA2((int)CurrFlag->R, (int)CurrFlag->G, (int)CurrFlag->B, 255);
					continue;
				}

				if (!_stricmp(keyChar, "G"))
				{
					CurrFlag->G = (float)atof(valueChar);
					CurrFlag->DebugColor = duRGBA2((int)CurrFlag->R, (int)CurrFlag->G, (int)CurrFlag->B, 255);
					continue;
				}

				if (!_stricmp(keyChar, "B"))
				{
					CurrFlag->B = (float)atof(valueChar);
					CurrFlag->DebugColor = duRGBA2((int)CurrFlag->R, (int)CurrFlag->G, (int)CurrFlag->B, 255);
					continue;
				}

				continue;
			}

			if (mode == 2)
			{
				NavAreaDefinition* CurrArea = (NavAreaDefinition*)CurrentItem;

				if (!CurrArea) { continue; }

				if (!_stricmp(keyChar, "area_id"))
				{
					CurrArea->AreaId = (unsigned char)atoi(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "area_name"))
				{
					CurrArea->AreaName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "area_tech_name"))
				{
					CurrArea->TechnicalName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "is_custom"))
				{
					CurrArea->bCustom = (!_stricmp(valueChar, "true"));
					continue;
				}

				if (!_stricmp(keyChar, "flag_index"))
				{
					CurrArea->FlagIndex = (unsigned int)atoi(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "R"))
				{
					CurrArea->R = (float)atof(valueChar);
					CurrArea->DebugColor = duRGBA2((int)CurrArea->R, (int)CurrArea->G, (int)CurrArea->B, 255);
					continue;
				}

				if (!_stricmp(keyChar, "G"))
				{
					CurrArea->G = (float)atof(valueChar);
					CurrArea->DebugColor = duRGBA2((int)CurrArea->R, (int)CurrArea->G, (int)CurrArea->B, 255);
					continue;
				}

				if (!_stricmp(keyChar, "B"))
				{
					CurrArea->B = (float)atof(valueChar);
					CurrArea->DebugColor = duRGBA2((int)CurrArea->R, (int)CurrArea->G, (int)CurrArea->B, 255);
					continue;
				}

				continue;
			}

			if (mode == 3)
			{
				NavOffMeshConnectionDefinition* CurrConnection = (NavOffMeshConnectionDefinition*)CurrentItem;

				if (!CurrConnection) { continue; }

				if (!_stricmp(keyChar, "connection_name"))
				{
					CurrConnection->ConnName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "area_index"))
				{
					CurrConnection->AreaIndex = (unsigned int)atoi(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "flag_index"))
				{
					CurrConnection->FlagIndex = (unsigned int)atoi(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "is_custom"))
				{
					CurrConnection->bCustom = (!_stricmp(valueChar, "true"));
					continue;
				}

				continue;
			}

			if (mode == 4)
			{
				NavMeshDefinition* CurrMesh = (NavMeshDefinition*)CurrentItem;

				if (!CurrMesh) { continue; }

				if (!_stricmp(keyChar, "mesh_name"))
				{
					CurrMesh->NavMeshName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "technical_name"))
				{
					CurrMesh->TechnicalName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "agent_radius"))
				{
					CurrMesh->AgentRadius = (float)atof(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "agent_stand_height"))
				{
					CurrMesh->AgentStandingHeight = (float)atof(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "agent_crouch_height"))
				{
					CurrMesh->AgentCrouchingHeight = (float)atof(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "max_step"))
				{
					CurrMesh->MaxStep = (float)atof(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "max_slope"))
				{
					CurrMesh->MaxSlope = (float)atof(valueChar);
					continue;
				}

				continue;
			}

			if (mode == 5)
			{
				NavAgentProfile* CurrAgent = (NavAgentProfile*)CurrentItem;

				if (!CurrAgent) { continue; }

				if (!_stricmp(keyChar, "agent_name"))
				{
					CurrAgent->ProfileName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "technical_name"))
				{
					CurrAgent->TechnicalName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "mesh_index"))
				{
					CurrAgent->NavMeshIndex = (unsigned int)atoi(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "area_costs"))
				{
					int Index = 0;

					line.erase(remove(line.begin(), line.end(), '{'), line.end());
					line.erase(remove(line.begin(), line.end(), '}'), line.end());

					std::stringstream s_stream(line);

					while (s_stream.good() && Index < 32)
					{
						string substr;
						getline(s_stream, substr, ','); //get first string delimited by comma
						float NewCost = (float)atof(substr.c_str());

						CurrAgent->AreaCosts[Index] = NewCost;
						Index++;
					}

					continue;
				}

				if (!_stricmp(keyChar, "movement_flags"))
				{
					CurrAgent->MovementFlags = (unsigned int)atoi(valueChar);
					continue;
				}

				continue;
			}

			if (mode == 6)
			{
				NavHintDefinition* CurrHint = (NavHintDefinition*)CurrentItem;

				if (!CurrHint) { continue; }

				if (!_stricmp(keyChar, "hint_index"))
				{
					CurrHint->HintIndex = (unsigned int)atoi(valueChar);
					continue;
				}

				if (!_stricmp(keyChar, "hint_name"))
				{
					CurrHint->HintName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "hint_technical_name"))
				{
					CurrHint->TechnicalName = valueChar;
					continue;
				}

				if (!_stricmp(keyChar, "hint_id"))
				{
					CurrHint->HintId = (unsigned int)atoi(valueChar);
					continue;
				}

				continue;
			}

		}

	}
}

void OutputProfileConfig(NavGameProfile* Profile)
{
	if (!Profile) { return; }

	string ProfileFileName = Profile->FileName;

	if (ProfileFileName.empty())
	{
		ProfileFileName = Profile->GameName;

		ProfileFileName.erase(std::remove_if(ProfileFileName.begin(), ProfileFileName.end(),
			[](auto const& c) -> bool { return !std::isalnum(c); }), ProfileFileName.end());
	}

	if (ProfileFileName.empty())
	{
		ProfileFileName = "profile";
	}

	ProfileFileName.append(".yaml");

	string FileName = "Profiles/" + ProfileFileName;

	FILE* fp = fopen(FileName.c_str(), "w+");

	if (!fp) { return; }

	fprintf(fp, "---\n");
	fprintf(fp, "game_name: %s\n", Profile->GameName.c_str());
	fprintf(fp, "game_directory: %s\n\n", Profile->GameDirectory.c_str());

	fprintf(fp, "flag_count: %u\n", Profile->FlagDefinitions.size());
	fprintf(fp, "flags:\n");

	int FlagIndex = 0;

	for (auto it = Profile->FlagDefinitions.begin(); it != Profile->FlagDefinitions.end(); it++)
	{
		fprintf(fp, "\tid: %d\n", FlagIndex);
		fprintf(fp, "\t\tflag_index: %u\n", it->NavFlagIndex);
		fprintf(fp, "\t\tflag_id: %u\n", it->FlagId);
		fprintf(fp, "\t\tflag_name: %s\n", it->FlagName.c_str());
		fprintf(fp, "\t\tflag_tech_name: %s\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\tis_teleport: %s\n", (it->bTeleport) ? "true" : "false");
		fprintf(fp, "\t\tis_custom: %s\n", (it->bCustom) ? "true" : "false");
		fprintf(fp, "\t\tR: %.1f\n", it->R);
		fprintf(fp, "\t\tG: %.1f\n", it->G);
		fprintf(fp, "\t\tB: %.1f\n", it->B);

		FlagIndex++;
	}

	fprintf(fp, "\n");

	fprintf(fp, "area_count: %u\n", Profile->AreaDefinitions.size());
	fprintf(fp, "areas:\n");

	int AreaIndex = 0;

	for (auto it = Profile->AreaDefinitions.begin(); it != Profile->AreaDefinitions.end(); it++)
	{
		fprintf(fp, "\tid: %d\n", AreaIndex);
		fprintf(fp, "\t\tarea_id: %d\n", it->AreaId);
		fprintf(fp, "\t\tarea_name: %s\n", it->AreaName.c_str());
		fprintf(fp, "\t\tarea_tech_name: %s\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\tis_custom: %s\n", (it->bCustom) ? "true" : "false");
		fprintf(fp, "\t\tflag_index: %d\n", it->FlagIndex);
		fprintf(fp, "\t\tR: %.1f\n", it->R);
		fprintf(fp, "\t\tG: %.1f\n", it->G);
		fprintf(fp, "\t\tB: %.1f\n", it->B);

		AreaIndex++;
	}

	fprintf(fp, "\n");

	fprintf(fp, "connection_count: %u\n", Profile->AreaDefinitions.size());
	fprintf(fp, "connections:\n");

	int ConnIndex = 0;

	for (auto it = Profile->ConnectionDefinitions.begin(); it != Profile->ConnectionDefinitions.end(); it++)
	{
		fprintf(fp, "\tid: %d\n", ConnIndex);
		fprintf(fp, "\t\tconnection_name: %s\n", it->ConnName.c_str());
		fprintf(fp, "\t\tarea_index: %u\n", it->AreaIndex);
		fprintf(fp, "\t\tflag_index: %u\n", it->FlagIndex);
		fprintf(fp, "\t\tis_custom: %s\n", (it->bCustom) ? "true" : "false");

		ConnIndex++;
	}

	fprintf(fp, "\n");

	fprintf(fp, "mesh_count: %u\n", Profile->MeshDefinitions.size());
	fprintf(fp, "meshes:\n");

	int MeshIndex = 0;

	for (auto it = Profile->MeshDefinitions.begin(); it != Profile->MeshDefinitions.end(); it++)
	{
		fprintf(fp, "\tid: %d\n", MeshIndex);
		fprintf(fp, "\t\tmesh_name: %s\n", it->NavMeshName.c_str());
		fprintf(fp, "\t\ttechnical_name: %s\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\tagent_radius: %.1f\n", it->AgentRadius);
		fprintf(fp, "\t\tagent_stand_height: %.1f\n", it->AgentStandingHeight);
		fprintf(fp, "\t\tagent_crouch_height: %.1f\n", it->AgentCrouchingHeight);
		fprintf(fp, "\t\tmax_step: %.1f\n", it->MaxStep);
		fprintf(fp, "\t\tmax_slope: %.1f\n", it->MaxSlope);

		MeshIndex++;
	}

	fprintf(fp, "\n");

	fprintf(fp, "agent_profile_count: %u\n", Profile->ProfileDefinitions.size());
	fprintf(fp, "agent_profiles:\n");

	int ProfileIndex = 0;

	for (auto it = Profile->ProfileDefinitions.begin(); it != Profile->ProfileDefinitions.end(); it++)
	{
		fprintf(fp, "\tid: %d\n", ProfileIndex);
		fprintf(fp, "\t\tagent_name: %s\n", it->ProfileName.c_str());
		fprintf(fp, "\t\ttechnical_name: %s\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\tmesh_index: %d\n", it->NavMeshIndex);
		fprintf(fp, "\t\tarea_costs: {%.1f", it->AreaCosts[0]);

		for (int i = 1; i < 32; i++)
		{
			fprintf(fp, ",%.1f", it->AreaCosts[i]);
		}

		fprintf(fp, "}\n");
		fprintf(fp, "\t\tmovement_flags: %u\n", it->MovementFlags);

		ProfileIndex++;
	}

	fprintf(fp, "\n");

	fprintf(fp, "nav_hint_count: %u\n", Profile->NavHints.size());
	fprintf(fp, "nav_hints:\n");

	int HintIndex = 0;

	for (auto it = Profile->NavHints.begin(); it != Profile->NavHints.end(); it++)
	{
		fprintf(fp, "\tid: %d\n", HintIndex);
		fprintf(fp, "\t\thint_index: %u\n", it->HintIndex);
		fprintf(fp, "\t\thint_name: %s\n", it->HintName.c_str());
		fprintf(fp, "\t\thint_technical_name: %s\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\thint_id: %u\n", it->HintId);

		HintIndex++;
	}

	fflush(fp);
	fclose(fp);
}

void OutputIncludeHeader(NavGameProfile* Profile)
{
	if (!Profile) { return; }

	string HeaderFileName = Profile->FileName;

	if (HeaderFileName.empty())
	{
		HeaderFileName = Profile->GameName;

		HeaderFileName.erase(std::remove_if(HeaderFileName.begin(), HeaderFileName.end(),
			[](auto const& c) -> bool { return !std::isalnum(c); }), HeaderFileName.end());
	}

	if (HeaderFileName.empty())
	{
		HeaderFileName = "profile";
	}
	
	HeaderFileName.append(".h");

	string FileName = "Profiles/" + HeaderFileName;

	FILE* fp = fopen(FileName.c_str(), "w+");

	if (!fp) { return; }

	fprintf(fp, "#pragma once\n\n");

	fprintf(fp, "#ifndef NAV_CONSTANTS_H\n");
	fprintf(fp, "#define NAV_CONSTANTS_H\n\n");

	fprintf(fp, "#define NAV_MESH_DEFAULT 0\n\n");
	fprintf(fp, "#define NUM_NAV_MESHES %u\n\n", (unsigned int)GetNumNavMeshes());

	fprintf(fp, "#include <vector>\n\n");

	fprintf(fp, "// Possible movement types. Defines the actions the bot needs to take to traverse this node\n");
	fprintf(fp, "enum NavMovementFlag\n");
	fprintf(fp, "{\n");
	
	vector<NavFlagDefinition> AllFlags = GetAllNavFlagDefinitions();

	for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
	{
		fprintf(fp, "\t%s = 1 << %u,\t\t// %s\n", it->TechnicalName.c_str(), it->NavFlagIndex, it->FlagName.c_str());
	}

	fprintf(fp, "\tNAV_FLAG_ALL = -1\t\t// All flags\n");

	fprintf(fp, "};\n\n");

	fprintf(fp, "// Nav hint types\n");
	fprintf(fp, "enum NavHintType\n");
	fprintf(fp, "{\n");

	vector<NavHintDefinition> AllNavHints = GetAllNavHintDefinitions();

	for (auto it = AllNavHints.begin(); it != AllNavHints.end(); it++)
	{
		fprintf(fp, "\t%s = 1 << %u,\t\t// %s\n", it->TechnicalName.c_str(), it->HintIndex, it->HintName.c_str());
	}

	fprintf(fp, "\tNAV_HINT_ANY = -1\t\t// Any hint type\n");

	fprintf(fp, "};\n\n");

	fprintf(fp, "// Area types. Defines the cost of movement through an area and which flag to use\n");
	fprintf(fp, "enum NavArea\n");
	fprintf(fp, "{\n");

	fprintf(fp, "\tNAV_AREA_NULL = 0,\t\t// Null area, cuts a hole in the mesh\n");

	vector<NavAreaDefinition> AllAreas = GetAllNavAreaDefinitions();

	for (auto it = AllAreas.begin(); it != AllAreas.end(); it++)
	{
		fprintf(fp, "\t%s = %u,\t\t// %s\n", it->TechnicalName.c_str(), it->AreaId, it->AreaName.c_str());
	}

	fprintf(fp, "};\n\n");

	fprintf(fp, "// Profile indices. Use these when retrieving base agent profile information\n");
	fprintf(fp, "enum NavProfileIndex\n");
	fprintf(fp, "{\n");

	vector<NavAgentProfile> AllProfiles = GetAllAgentProfileDefinitions();
	int ProfileIndex = 0;

	for (auto it = AllProfiles.begin(); it != AllProfiles.end(); it++)
	{
		fprintf(fp, "\t%s = %u,\t\t// %s\n", it->TechnicalName.c_str(), ProfileIndex, it->ProfileName.c_str());
		ProfileIndex++;
	}

	fprintf(fp, "\t%s = %u,\t\t// Default profile which has all capabilities except disabled flags, and 1.0 area costs for everything \n", "NAV_PROFILE_DEFAULT", ProfileIndex);

	fprintf(fp, "};\n\n");

	fprintf(fp, "// Profile indices. Use these when retrieving base agent profile information\n");
	fprintf(fp, "enum NavMeshIndex\n");
	fprintf(fp, "{\n");

	vector<NavMeshDefinition> AllMeshes = GetAllMeshDefinitions();
	int MeshIndex = 0;

	for (auto it = AllMeshes.begin(); it != AllMeshes.end(); it++)
	{
		fprintf(fp, "\t%s = %u,\t\t// %s\n", it->TechnicalName.c_str(), MeshIndex, it->NavMeshName.c_str());
		MeshIndex++;
	}

	fprintf(fp, "};\n\n");

	fprintf(fp, "// Agent profile definition. Holds all information an agent needs when querying the nav mesh\n");
	fprintf(fp, "typedef struct _NAV_AGENT_PROFILE\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tunsigned int NavMeshIndex = 0;\n");
	fprintf(fp, "\tclass dtQueryFilter Filters;\n");
	fprintf(fp, "\tbool bFlyingProfile = false;\n");
	fprintf(fp, "} NavAgentProfile;\n\n");

	fprintf(fp, "// Declared in DTNavigation.cpp\n");
	fprintf(fp, "// List of base agent profiles\n");
	fprintf(fp, "extern std::vector<NavAgentProfile> BaseAgentProfiles;\n\n");

	fprintf(fp, "// Agent profile definition. Holds all information an agent needs when querying the nav mesh\n");
	fprintf(fp, "typedef struct _NAV_HINT\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tunsigned int NavMeshIndex = 0;\n");
	fprintf(fp, "\tunsigned int HintTypes = 0;\n");
	fprintf(fp, "\tVector Position;\n");
	fprintf(fp, "} NavHint;\n\n");

	fprintf(fp, "// Retrieve appropriate flag for area (See process() in the MeshProcess struct)\n");
	fprintf(fp, "inline NavMovementFlag GetFlagForArea(NavArea Area)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tswitch(Area)\n");
	fprintf(fp, "\t{\n");

	for (auto it = AllAreas.begin(); it != AllAreas.end(); it++)
	{
		NavFlagDefinition* Flag = GetFlagAtIndex(it->FlagIndex);

		fprintf(fp, "\t\tcase %s:\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\t\treturn %s;\n", Flag->TechnicalName.c_str());
	}

	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\treturn NAV_FLAG_DISABLED;\n");
	

	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n\n");

	fprintf(fp, "// Get appropriate debug colour for the area. Returns RGB as 3 unsigned chars encoded into a single unsigned int\n");
	fprintf(fp, "inline void GetDebugColorForArea(NavArea Area, unsigned char& R, unsigned char& G, unsigned char& B)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tswitch(Area)\n");
	fprintf(fp, "\t{\n");

	fprintf(fp, "\t\tcase NAV_AREA_NULL:\n");
	fprintf(fp, "\t\t\tR = 128;\n");
	fprintf(fp, "\t\t\tG = 128;\n");
	fprintf(fp, "\t\t\tB = 128;\n");
	fprintf(fp, "\t\t\tbreak;\n");

	for (auto it = AllAreas.begin(); it != AllAreas.end(); it++)
	{
		fprintf(fp, "\t\tcase %s:\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\t\tR = %u;\n", (unsigned char)it->R);
		fprintf(fp, "\t\t\tG = %u;\n", (unsigned char)it->G);
		fprintf(fp, "\t\t\tB = %u;\n", (unsigned char)it->B);
		fprintf(fp, "\t\t\tbreak;\n");
	}

	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\tR = 255;\n");
	fprintf(fp, "\t\t\tG = 255;\n");
	fprintf(fp, "\t\t\tB = 255;\n");
	fprintf(fp, "\t\t\tbreak;\n");


	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n\n");

	fprintf(fp, "// Get appropriate debug colour for the movement flag. Returns RGB as 3 unsigned chars encoded into a single unsigned int\n");
	fprintf(fp, "inline void GetDebugColorForFlag(NavMovementFlag Flag, unsigned char& R, unsigned char& G, unsigned char& B)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tswitch(Flag)\n");
	fprintf(fp, "\t{\n");

	for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
	{
		fprintf(fp, "\t\tcase %s:\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\t\tR = %u;\n", (unsigned char)it->R);
		fprintf(fp, "\t\t\tG = %u;\n", (unsigned char)it->G);
		fprintf(fp, "\t\t\tB = %u;\n", (unsigned char)it->B);
		fprintf(fp, "\t\t\tbreak;\n");
	}

	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\tR = 255;\n");
	fprintf(fp, "\t\t\tG = 255;\n");
	fprintf(fp, "\t\t\tB = 255;\n");
	fprintf(fp, "\t\t\tbreak;\n");


	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n\n");

	fprintf(fp, "// Return name of a flag for debugging purposes\n");
	fprintf(fp, "inline void GetFlagName(NavMovementFlag Flag, char* outName)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tif (!outName) { return; }\n\n");
	fprintf(fp, "\tswitch(Flag)\n");
	fprintf(fp, "\t{\n");

	for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
	{
		fprintf(fp, "\t\tcase %s:\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\t\tsprintf(outName, \"%s\");\n", it->FlagName.c_str());
		fprintf(fp, "\t\t\tbreak;\n");
	}

	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\tsprintf(outName, \"Undefined\");\n");
	fprintf(fp, "\t\t\tbreak;\n");
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n\n");

	fprintf(fp, "// Returns true if this flag is a teleport move (i.e. not affected by doors or other obstacles)\n");
	fprintf(fp, "inline bool IsFlagTeleportType(NavMovementFlag Flag)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tswitch(Flag)\n");
	fprintf(fp, "\t{\n");

	for (auto it = AllFlags.begin(); it != AllFlags.end(); it++)
	{
		fprintf(fp, "\t\tcase %s:\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\t\treturn %s;\n", (it->bTeleport) ? "true" : "false");
	}

	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\treturn false;\n");
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n\n");

	fprintf(fp, "// Return name of a flag for debugging purposes\n");
	fprintf(fp, "inline void GetAreaName(NavArea Area, char* outName)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tif (!outName) { return; }\n\n");
	fprintf(fp, "\tswitch(Area)\n");
	fprintf(fp, "\t{\n");

	for (auto it = AllAreas.begin(); it != AllAreas.end(); it++)
	{
		fprintf(fp, "\t\tcase %s:\n", it->TechnicalName.c_str());
		fprintf(fp, "\t\t\tsprintf(outName, \"%s\");\n", it->AreaName.c_str());
		fprintf(fp, "\t\t\tbreak;\n");
	}

	fprintf(fp, "\t\tdefault:\n");
	fprintf(fp, "\t\t\tsprintf(outName, \"Undefined\");\n");
	fprintf(fp, "\t\t\tbreak;\n");


	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n\n");


	fprintf(fp, "// Populate the base nav profiles. Should be called once after loading the navigation data\n");
	fprintf(fp, "inline void PopulateBaseAgentProfiles()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tBaseAgentProfiles.clear();\n\n");

	int CurrIndex = 0;

	for (auto it = AllProfiles.begin(); it != AllProfiles.end(); it++)
	{
		fprintf(fp, "\tNavAgentProfile NewProfile%d;\n", CurrIndex);
		fprintf(fp, "\tNewProfile%d.NavMeshIndex = %u;\n", CurrIndex, it->NavMeshIndex);
		fprintf(fp, "\tNewProfile%d.Filters.setIncludeFlags(%u);\n", CurrIndex, it->MovementFlags);
		fprintf(fp, "\tNewProfile%d.Filters.setExcludeFlags(NAV_FLAG_DISABLED);\n", CurrIndex);

		for (auto areaIt = AllAreas.begin(); areaIt != AllAreas.end(); areaIt++)
		{
			fprintf(fp, "\tNewProfile%d.Filters.setAreaCost(%u, %.1f);\n", CurrIndex, areaIt->NavAreaIndex, it->AreaCosts[areaIt->NavAreaIndex]);			
		}

		fprintf(fp, "\tBaseAgentProfiles.push_back(NewProfile%d);\n\n", CurrIndex);

		CurrIndex++;		
	}

	fprintf(fp, "\tNavAgentProfile DefaultProfile;\n");
	fprintf(fp, "\tDefaultProfile.NavMeshIndex = 0;\n");
	fprintf(fp, "\tDefaultProfile.Filters.setIncludeFlags(0x7fffffff);\n");
	fprintf(fp, "\tDefaultProfile.Filters.setExcludeFlags(NAV_FLAG_DISABLED);\n");

	for (auto areaIt = AllAreas.begin(); areaIt != AllAreas.end(); areaIt++)
	{
		fprintf(fp, "\tDefaultProfile.Filters.setAreaCost(%u, 1.0);\n", areaIt->NavAreaIndex);
	}

	fprintf(fp, "\tBaseAgentProfiles.push_back(DefaultProfile);\n\n");

	fprintf(fp, "}\n\n");


	fprintf(fp, "// Return the appropriate base nav profile information\n");
	fprintf(fp, "inline const NavAgentProfile GetBaseAgentProfile(const NavProfileIndex Index)\n");
	fprintf(fp, "{\n");

	fprintf(fp, "\treturn BaseAgentProfiles[Index];\n");

	fprintf(fp, "}\n\n");



	fprintf(fp, "#endif // NAV_CONSTANTS_H");



	fclose(fp);

}