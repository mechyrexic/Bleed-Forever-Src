//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "filesystem.h"
#include "KeyValues.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLogicExternalData : public CLogicalEntity
{
	DECLARE_CLASS( CLogicExternalData, CLogicalEntity );
	DECLARE_DATADESC();

public:
	~CLogicExternalData();

	void LoadFile();
	void SaveFile();
	void SetBlock(string_t iszNewTarget, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL);

	void Activate();

	// Inputs
	void InputWriteKeyValue( inputdata_t &inputdata );
	void InputRemoveKeyValue( inputdata_t &inputdata );
	void InputReadKey( inputdata_t &inputdata );
	void InputSetBlock( inputdata_t &inputdata );
	void InputSave( inputdata_t &inputdata );
	void InputReload( inputdata_t &inputdata );

	char m_iszFile[MAX_PATH];

	// Root file
	KeyValues *m_pRoot;

	// Our specific block
	KeyValues *m_pBlock;
	//string_t m_iszBlock; // Use m_target

	bool m_bSaveEachChange;
	bool m_bReloadBeforeEachAction;
	string_t m_iszMapname;

	COutputString m_OutValue;
};

LINK_ENTITY_TO_CLASS(logic_externaldata, CLogicExternalData);

BEGIN_DATADESC( CLogicExternalData )

	// Keys
	//DEFINE_KEYFIELD( m_iszBlock, FIELD_STRING, "Block" ),
	DEFINE_KEYFIELD( m_bSaveEachChange, FIELD_BOOLEAN, "SaveEachChange" ),
	DEFINE_KEYFIELD( m_bReloadBeforeEachAction, FIELD_BOOLEAN, "ReloadBeforeEachAction" ),
	DEFINE_KEYFIELD( m_iszMapname, FIELD_STRING, "Mapname" ),

	// This should be cached each load
	//DEFINE_ARRAY( m_iszFile, FIELD_CHARACTER, MAX_PATH ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "WriteKeyValue", InputWriteKeyValue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RemoveKeyValue", InputRemoveKeyValue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ReadKey", InputReadKey ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlock", InputSetBlock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Save", InputSave ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Reload", InputReload ),

	// Outputs
	DEFINE_OUTPUT(m_OutValue, "OutValue"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLogicExternalData::~CLogicExternalData()
{
	if (m_pRoot)
		m_pRoot->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::LoadFile()
{
	if (m_pRoot)
		m_pRoot->deleteThis();

	m_pRoot = new KeyValues( m_iszFile );
	m_pRoot->LoadFromFile(g_pFullFileSystem, m_iszFile, "MOD");

	// This shold work even if the file didn't load.
	if (m_target != NULL_STRING)
	{
		m_pBlock = m_pRoot->FindKey(STRING(m_target), true);
	}
	else
	{
		// Just do things from root
		m_pBlock = m_pRoot;
	}

	if (!m_pBlock)
	{
		Warning("WARNING! %s has NULL m_pBlock! Removing...\n", GetDebugName());
		UTIL_Remove(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::SaveFile()
{
	DevMsg("Saving to %s...\n", m_iszFile);
	m_pRoot->SaveToFile(g_pFullFileSystem, m_iszFile, "MOD", false, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::SetBlock(string_t iszNewTarget, CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	if (STRING(iszNewTarget)[0] == '!')
	{
		if (FStrEq(STRING(iszNewTarget), "!self"))
			iszNewTarget = GetEntityName();
		else if (pActivator && FStrEq(STRING(iszNewTarget), "!activator"))
			iszNewTarget = pActivator->GetEntityName();
		else if (pCaller && FStrEq(STRING(iszNewTarget), "!caller"))
			iszNewTarget = pCaller->GetEntityName();
	}

	m_target = iszNewTarget;
	LoadFile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::Activate()
{
	BaseClass::Activate();

	if (m_iszMapname == NULL_STRING || STRING(m_iszMapname)[0] == '\0')
		m_iszMapname = gpGlobals->mapname;

	Q_snprintf(m_iszFile, sizeof(m_iszFile), "maps/%s_externaldata.txt", STRING(m_iszMapname));
	DevMsg("LOGIC_EXTERNALDATA: %s\n", m_iszFile);
	
	// This handles !self, etc. even though the end result could just be assigning to itself.
	// Also calls LoadFile() for initial load.
	SetBlock(m_target);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::InputWriteKeyValue( inputdata_t &inputdata )
{
	const char *szValue = inputdata.value.String();
	char key[256];
	char value[256];

	// Separate key from value
	char *delimiter = Q_strstr(szValue, " ");
	if (delimiter)
	{
		Q_strncpy(key, szValue, MIN((delimiter - szValue) + 1, sizeof(key)));
		Q_strncpy(value, delimiter + 1, sizeof(value));
	}
	else
	{
		// Assume the value is just supposed to be null
		Q_strncpy(key, szValue, sizeof(key));
	}

	if (m_bReloadBeforeEachAction)
		LoadFile();

	m_pBlock->SetString(key, value);

	if (m_bSaveEachChange)
		SaveFile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::InputRemoveKeyValue( inputdata_t &inputdata )
{
	if (m_bReloadBeforeEachAction)
		LoadFile();

	KeyValues *pKV = m_pBlock->FindKey(inputdata.value.String());
	if (pKV)
	{
		m_pBlock->RemoveSubKey(pKV);

		if (m_bSaveEachChange)
			SaveFile();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::InputReadKey( inputdata_t &inputdata )
{
	if (m_bReloadBeforeEachAction)
		LoadFile();

	m_OutValue.Set(AllocPooledString(m_pBlock->GetString(inputdata.value.String())), inputdata.pActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::InputSetBlock( inputdata_t &inputdata )
{
	SetBlock(inputdata.value.StringID(), inputdata.pActivator, inputdata.pCaller);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::InputSave( inputdata_t &inputdata )
{
	SaveFile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicExternalData::InputReload( inputdata_t &inputdata )
{
	LoadFile();
}
