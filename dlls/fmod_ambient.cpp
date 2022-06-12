#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "UserMessages.h"
#include <string>
#include <iostream>

class CFmodAmbient : public CBaseEntity
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
	float min_atten;
	float max_atten;
	float pitch;
};

LINK_ENTITY_TO_CLASS(fmod_ambient, CFmodAmbient);

#define FMOD_AMBIENT_LOOPING 1
#define FMOD_AMBIENT_PLAYONSTART 2

void CFmodAmbient::Spawn()
{
	if (FStringNull(pev->message))
	{
		ALERT(at_error, "EMPTY FMOD_AMBIENT AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FBitSet(pev->spawnflags, FMOD_AMBIENT_LOOPING)) 
		m_fLooping = true;

	if (FBitSet(pev->spawnflags, FMOD_AMBIENT_PLAYONSTART))
		m_fPlayOnStart = true;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (m_fPlayOnStart)
	{
		SetThink(&CFmodAmbient::SendMsg);
		pev->nextthink = gpGlobals->time + 0.2f; // TODO: Make sure this is a long enough delay or the message might get dropped
	}
}

void CFmodAmbient::Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value)
{
	SendMsg();
}

void CFmodAmbient::SendMsg(void)
{
	// IF YOU MODIFY THIS DON'T FORGET TO UPDATE MsgFunc_FmodSave/Load
	// Name of the channel and path of the sound
	std::string msg = STRING(pev->targetname) + std::string("\n") + STRING(pev->message);
	// TODO: Figure out if we can truly only write one string in a message or if I'm doing something wrong
	MESSAGE_BEGIN(MSG_ALL, gmsgFmodAmb, NULL);
	WRITE_STRING(msg.c_str());
	WRITE_BYTE(m_fLooping);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(volume); // Default: 1.0
	WRITE_COORD(min_atten); // Default: 40.0
	WRITE_COORD(max_atten); // Default: 40000.0
	WRITE_COORD(pitch); // Default: 1.0 (2.0 = one octave up, 0.5 = one octave down)
	MESSAGE_END();

	// TODO: sanitize inputs
}

// Load key/value pairs
bool CFmodAmbient::KeyValue(KeyValueData* pkvd)
{
	// volume
	if (FStrEq(pkvd->szKeyName, "volume"))
	{
		volume = atof(pkvd->szValue);
		return true;
	}

	// minatten
	if (FStrEq(pkvd->szKeyName, "minatten"))
	{
		min_atten = atof(pkvd->szValue);
		return true;
	}

	// maxatten
	else if (FStrEq(pkvd->szKeyName, "maxatten"))
	{
		max_atten = atof(pkvd->szValue);
		return true;
	}

	// pitch
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pitch = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

// -----------------------------------------------------------------------------------------------

class CFmodPause : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(fmod_pause, CFmodPause);

void CFmodPause::Spawn()
{
	if (FStringNull(pev->targetname) || FStringNull(pev->target))
	{
		ALERT(at_error, "EMPTY FMOD_PAUSE AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}
}

void CFmodPause::Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgFmodPause, NULL);
	WRITE_STRING(STRING(pev->target));
	MESSAGE_END();
}

// -----------------------------------------------------------------------------------------------

class CFmodSeek : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

private:
	float seek_to;
};

LINK_ENTITY_TO_CLASS(fmod_seek, CFmodSeek);

void CFmodSeek::Spawn()
{
	if (FStringNull(pev->targetname) || FStringNull(pev->target))
	{
		ALERT(at_error, "EMPTY FMOD_SEEK AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}
}

void CFmodSeek::Use(CBaseEntity* pActivator, CBaseEntity* pOther, USE_TYPE useType, float value)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgFmodSeek, NULL);
	WRITE_STRING(STRING(pev->target));
	WRITE_COORD(seek_to);
	MESSAGE_END();
}

bool CFmodSeek::KeyValue(KeyValueData* pkvd)
{
	// seek
	if (FStrEq(pkvd->szKeyName, "seek"))
	{
		seek_to = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}