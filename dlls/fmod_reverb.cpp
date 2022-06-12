#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "UserMessages.h"
#include <string>
#include <iostream>

class CFmodReverb : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT SendMsg(void);

	bool m_fActivateOnStart;

private:
	float min_dist;
	float max_dist;
	int   preset;
};

LINK_ENTITY_TO_CLASS(fmod_reverb, CFmodReverb);

#define FMOD_REVERB_ACTIVATEONSTART 1

void CFmodReverb::Spawn()
{
	if (FBitSet(pev->spawnflags, FMOD_REVERB_ACTIVATEONSTART))
		m_fActivateOnStart = true;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (m_fActivateOnStart)
	{
		SetThink(&CFmodReverb::SendMsg);
		pev->nextthink = gpGlobals->time + 0.25f; // TODO: Make sure this is a long enough delay or the message might get dropped
	}
}

void CFmodReverb::Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value)
{
	SendMsg();
}

void CFmodReverb::SendMsg(void)
{
	// IF YOU MODIFY THIS DON'T FORGET TO UPDATE MsgFunc_FmodSave/Load
	MESSAGE_BEGIN(MSG_ALL, gmsgFmodRev, NULL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(min_dist);
	WRITE_COORD(max_dist);
	WRITE_BYTE(preset);
	MESSAGE_END();

	// TODO: sanitize inputs
}

// Load key/value pairs
bool CFmodReverb::KeyValue(KeyValueData* pkvd)
{
	// mindist
	if (FStrEq(pkvd->szKeyName, "mindist"))
	{
		min_dist = atof(pkvd->szValue);
		return true;
	}

	// maxdist
	else if (FStrEq(pkvd->szKeyName, "maxdist"))
	{
		max_dist = atof(pkvd->szValue);
		return true;
	}

	// preset
	else if (FStrEq(pkvd->szKeyName, "preset"))
	{
		preset = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}