#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "UserMessages.h"
#include <string>
#include <iostream>

class CFmodTrack : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT SendMsg(void);

	bool m_fLooping;
	bool m_fPlayOnStart;

private:
	float volume;
	float pitch;
};

LINK_ENTITY_TO_CLASS(fmod_track, CFmodTrack);

#define FMOD_TRACK_LOOPING 1
#define FMOD_TRACK_PLAYONSTART 2

void CFmodTrack::Spawn()
{
	if (FStringNull(pev->message))
	{
		ALERT(at_error, "EMPTY FMOD_TRACK AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FBitSet(pev->spawnflags, FMOD_TRACK_LOOPING))
		m_fLooping = true;
	if (FBitSet(pev->spawnflags, FMOD_TRACK_PLAYONSTART))
		m_fPlayOnStart = true;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (m_fPlayOnStart)
	{
		SetThink(&CFmodTrack::SendMsg);
		pev->nextthink = gpGlobals->time + 0.2f; // TODO: Make sure this is a long enough delay or the message might get dropped
	}
}

void CFmodTrack::Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value)
{
	SendMsg();
}

void CFmodTrack::SendMsg(void)
{
	// IF YOU MODIFY THIS DON'T FORGET TO UPDATE MsgFunc_FmodSave/Load
	MESSAGE_BEGIN(MSG_ALL, gmsgFmodTrk, NULL);
	WRITE_STRING(STRING(pev->message));
	WRITE_BYTE(m_fLooping);
	WRITE_COORD(volume); // Default: 1.0
	WRITE_COORD(pitch);	 // Default: 1.0 (2.0 = one octave up, 0.5 = one octave down)
	MESSAGE_END();

	// TODO: sanitize inputs
}

// Load key/value pairs
bool CFmodTrack::KeyValue(KeyValueData* pkvd)
{
	// volume
	if (FStrEq(pkvd->szKeyName, "volume"))
	{
		volume = atof(pkvd->szValue);
		return true;
	}

	// pitch
	if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pitch = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}