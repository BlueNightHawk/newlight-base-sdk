#include "cl_dll.h"
#include "cl_util.h"
#include "extdll.h"
#include "hud.h" // CHudFmodPlayer declared in hud.h
#include "parsemsg.h"
#include "fmod_manager.h"
#include "FMOD/fmod_errors.h"

#include <map>
#include <fstream>
#include <iostream>

DECLARE_MESSAGE(m_Fmod, FmodCache)
DECLARE_MESSAGE(m_Fmod, FmodAmb)
DECLARE_MESSAGE(m_Fmod, FmodTrk)
DECLARE_MESSAGE(m_Fmod, FmodRev)
DECLARE_MESSAGE(m_Fmod, FmodPause)
DECLARE_MESSAGE(m_Fmod, FmodSeek)
DECLARE_MESSAGE(m_Fmod, FmodSave)
DECLARE_MESSAGE(m_Fmod, FmodLoad)

bool CHudFmodPlayer::Init()
{
	HOOK_MESSAGE(FmodCache);
	HOOK_MESSAGE(FmodAmb);
	HOOK_MESSAGE(FmodTrk);
	HOOK_MESSAGE(FmodRev);
	HOOK_MESSAGE(FmodPause);
	HOOK_MESSAGE(FmodSeek);
	HOOK_MESSAGE(FmodSave);
	HOOK_MESSAGE(FmodLoad);

	gHUD.AddHudElem(this);
	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodSave(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string filename = std::string(READ_STRING());

	_Fmod_Report("INFO", "Saving the following file: " + filename);

	std::ofstream save_file;
	save_file.open(filename);

	// fmod_current_track entry: SOUND_NAME MODE VOLUME PITCH POSITION PAUSED

	FMOD::Sound *sound;
	fmod_current_track->getCurrentSound(&sound);
	std::string current_track_sound_name = "";
	
	// Do a quick and dirty find by value
	auto tracks_it = fmod_tracks.begin();
	while (tracks_it != fmod_tracks.end())
	{
		if (tracks_it->second == sound)
		{
			current_track_sound_name = tracks_it->first;
			break;
		}

		tracks_it++;
	}

	if (tracks_it == fmod_tracks.end())
	{
		_Fmod_Report("ERROR", "Could not find fmod_current_track for savefile!");
		return false;
	}

	FMOD_MODE current_track_mode;
	float current_track_volume = 0.0f;
	float current_track_pitch = 1.0f;
	unsigned int current_track_position = 0;
	bool current_track_paused = true;

	fmod_current_track->getMode(&current_track_mode);
	fmod_current_track->getVolume(&current_track_volume);
	fmod_current_track->getPitch(&current_track_pitch);
	fmod_current_track->getPosition(&current_track_position, FMOD_TIMEUNIT_PCM);
	fmod_current_track->getPaused(&current_track_paused);

	save_file << current_track_sound_name << " " << current_track_mode << " " << current_track_volume << " " << current_track_pitch
			  << " " << current_track_position << " " << current_track_paused << std::endl;

	auto channels_it = fmod_channels.begin();
	while (channels_it != fmod_channels.end())
	{
		// channel entry: ENT_NAME SOUND_NAME MODE X Y Z VOLUME MINDIST MAXDIST PITCH POSITION PAUSED
		std::string ent_name = channels_it->first;
		FMOD::Channel *channel = channels_it->second;

		FMOD_MODE mode;
		FMOD_VECTOR pos;
		FMOD_VECTOR vel; // unused
		float volume = 0.0f;
		float minDist = 40.0f;
		float maxDist = 400000.0f;
		float pitch = 1.0f;
		unsigned int position = 0;
		bool paused = true;

		channel->getMode(&mode);
		channel->get3DAttributes(&pos, &vel);
		channel->getVolume(&volume);
		channel->get3DMinMaxDistance(&minDist, &maxDist);
		channel->getPitch(&pitch);
		channel->getPosition(&position, FMOD_TIMEUNIT_PCM);
		channel->getPaused(&paused);

		// Find sound path
		std::string sound_name = "";
		FMOD::Sound *sound;
		channel->getCurrentSound(&sound);

		// Do quick and dirty find by value
		auto sounds_it = fmod_cached_sounds.begin();
		while (sounds_it != fmod_cached_sounds.end())
		{
			if (sounds_it->second == sound)
			{
				sound_name = sounds_it->first;
				break;
			}

			sounds_it++;
		}

		if (sounds_it == fmod_cached_sounds.end())
		{
			_Fmod_Report("ERROR", "Could not find sound for ent " + ent_name + " for savefile!");
			return false;
		}

		save_file << ent_name << " " << sound_name << " " << mode << " " << pos.x << " " << pos.y << " " << pos.z
			<< " " << volume << " " << minDist << " " << maxDist << " " << pitch << " " << position << " " << paused;

		channels_it++;

		if (channels_it != fmod_channels.end())
			save_file << std::endl;
	}

	save_file.close();

	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodLoad(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string filename = std::string(READ_STRING());

	_Fmod_Report("INFO", "Loading the following file: " + filename);

	bool mp3_paused = false;
	bool sfx_paused = false;

	fmod_mp3_group->getPaused(&mp3_paused);
	fmod_sfx_group->getPaused(&sfx_paused);

	fmod_mp3_group->setPaused(true);
	fmod_sfx_group->setPaused(true);
	//Fmod_Release_Channels();

	std::ifstream save_file;
	save_file.open(filename);

	fmod_current_track->stop();

	std::string current_track_sound_name = "";
	FMOD_MODE current_track_mode;
	float current_track_volume = 0.0f;
	float current_track_pitch = 1.0f;
	unsigned int current_track_position = 0;
	bool current_track_paused = true;

	save_file >> current_track_sound_name >> current_track_mode >> current_track_volume >> current_track_pitch >> current_track_position >> current_track_paused;
	// TODO: verify we didn't reach EOF

	auto it = fmod_tracks.find(current_track_sound_name);

	if (it == fmod_tracks.end())
	{
		_Fmod_Report("ERROR", "Could not find track " + current_track_sound_name + " while loading savefile!");
		return false;
	}

	FMOD::Sound *current_track_sound = it->second;

	FMOD_RESULT result = fmod_system->playSound(current_track_sound, fmod_mp3_group, true, &fmod_current_track);
	if (!_Fmod_Result_OK(&result))
		return false; // TODO: investigate if a failure here might create a memory leak

	fmod_current_track->setMode(current_track_mode);
	fmod_current_track->setVolume(current_track_volume);
	fmod_current_track->setPitch(current_track_pitch);
	fmod_current_track->setPosition(current_track_position, FMOD_TIMEUNIT_PCM);
	fmod_current_track->setPaused(current_track_paused);

	while (!save_file.eof())
	{
		std::string ent_name = "";
		std::string sound_name = "";
		FMOD_MODE mode;
		FMOD_VECTOR pos;
		FMOD_VECTOR vel; // unused
		float volume = 0.0f;
		float minDist = 40.0f;
		float maxDist = 400000.0f;
		float pitch = 1.0f;
		unsigned int position = 0;
		bool paused = true;

		save_file >> ent_name >> sound_name >> mode >> pos.x >> pos.y >> pos.z >> volume >> minDist >> maxDist >> pitch >> position >> paused;
		vel.x = 0; vel.y = 0; vel.z = 0;

		FMOD::Sound *sound = nullptr;
		auto sound_it = fmod_cached_sounds.find(sound_name);

		if (sound_it == fmod_cached_sounds.end())
		{
			// TODO: Precaching is breaking on load for some reason. Fix it.
			/*_Fmod_Report("ERROR", "Could not find sound " + sound_name + " for ent " + ent_name + " from savefile!");
			return false;*/

			// HACK: For now, force sound to be cached here
			sound = Fmod_CacheSound(sound_name.c_str(), false);
		}
		else
		{
			sound = sound_it->second;
		}

		FMOD::Channel *channel = Fmod_CreateChannel(sound, ent_name.c_str(), fmod_sfx_group, false, volume);
		channel->setMode(mode);
		channel->set3DAttributes(&pos, &vel);
		channel->set3DMinMaxDistance(minDist, maxDist);
		channel->setPitch(pitch);
		channel->setPosition(position, FMOD_TIMEUNIT_PCM);
		channel->setPaused(paused);
	}

	save_file.close();

	fmod_mp3_group->setPaused(mp3_paused);
	fmod_sfx_group->setPaused(sfx_paused);

	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodCache(const char* pszName, int iSize, void* pbuf)
{
	std::string gamedir = gEngfuncs.pfnGetGameDirectory();
	std::string level_name = gEngfuncs.pfnGetLevelName();
	std::string soundcache_path = gamedir + "/" + level_name + "_soundcache.txt";

	Fmod_Release_Channels();
	//Fmod_Release_Sounds(); // We now intelligently only release sounds that aren't used in the current map

	// Get all sound path strings from file
	std::vector<std::string> sound_paths;
	std::ifstream soundcache_file;

	sound_paths.reserve(100);
	soundcache_file.open(soundcache_path);

	if (!soundcache_file.is_open())
	{
		_Fmod_Report("WARNING", "Could not open soundcache file " + soundcache_path + ". No sounds were precached!");
		Fmod_Release_Sounds(); // Release all sounds
		return true;
	}
	else _Fmod_Report("INFO", "Precaching sounds from file: " + soundcache_path);

	std::string filename;
	while (std::getline(soundcache_file, filename)) sound_paths.push_back(filename);

	if (!soundcache_file.eof())
		_Fmod_Report("WARNING", "Stopped reading soundcache file " + soundcache_path + " before the end of file due to error.");

	soundcache_file.close();

	// Create a vector of all sounds files, cached or to be loaded, with no duplicates
	std::vector<std::string> all_sounds(sound_paths);
	auto sounds_it = fmod_cached_sounds.begin();
	while (sounds_it != fmod_cached_sounds.end())
	{
		// Check if path is already in all_sounds
		auto sound_path = std::find(all_sounds.begin(), all_sounds.end(), sounds_it->first);
		if (sound_path != all_sounds.end())
		{
			// If it's not already in all_sounds, add it
			all_sounds.push_back(sounds_it->first);
		}

		sounds_it++;
	}

	// For each sound path in all_sounds, check if it's already loaded and if we're trying to load it
	for (size_t i = 0; i < all_sounds.size(); i++)
	{
		auto cached_sound = fmod_cached_sounds.find(all_sounds[i]);
		auto load_sound = std::find(sound_paths.begin(), sound_paths.end(), all_sounds[i]);

		// If the sound is already cached and we want to load it, remove it from sound_paths so we don't load it again
		if (cached_sound != fmod_cached_sounds.end() && load_sound != sound_paths.end())
			sound_paths.erase(load_sound);

		// If the sound is cached but we don't want to load it, uncache it
		else if (cached_sound != fmod_cached_sounds.end() && load_sound == sound_paths.end())
		{
			cached_sound->second->release();
			fmod_cached_sounds.erase(cached_sound);
		}

		// If the sound is not cached but we want to load it, do nothing; it will remain in sound_paths and be loaded
	}

	// Finally, load any remaining sounds that we didn't carry over from the previous map
	for (size_t i = 0; i < sound_paths.size(); i++) // array-style access is faster than access by iterator
	{
		FMOD::Sound *sound = Fmod_CacheSound(sound_paths[i].c_str(), false);
		if (!sound)
			_Fmod_Report("WARNING", "Error occured while attempting to precache " + sound_paths[i] + ". Sound was not precached.");
	}

	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodAmb(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string msg = std::string(READ_STRING());
	bool looping = READ_BYTE();
	// TODO: Clean this up and put all the reads together for more visual clarity in the code
	Vector pos;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();

	FMOD_VECTOR fmod_pos = _Fmod_HLVecToFmodVec(pos);

	FMOD_VECTOR vel;
	vel.x = 0;
	vel.y = 0;
	vel.z = 0;

	float volume = READ_COORD();

	float min_atten = READ_COORD();
	float max_atten = READ_COORD();
	float pitch = READ_COORD();

	// TODO: sanitize inputs

	std::string channel_name = msg.substr(0, msg.find('\n'));
	std::string sound_path = msg.substr(msg.find('\n') + 1, std::string::npos);

	FMOD::Sound* sound = NULL;
	FMOD::Channel* channel = NULL;

	// TODO: Separate finding the sound into its own function
	auto sound_iter = fmod_cached_sounds.find(sound_path);

	if (sound_iter == fmod_cached_sounds.end())
	{
		_Fmod_Report("WARNING", "Entity " + channel_name + " playing uncached sound " + sound_path +
									". Add the sound to your [MAPNAME].bsp_soundcache.txt file.");
		_Fmod_Report("INFO", "Attempting to cache and play sound " + sound_path);
		sound = Fmod_CacheSound(sound_path.c_str(), false);
		if (!sound) return false; // Error will be reported by Fmod_CacheSound
	}
	else
		sound = sound_iter->second;

	// Non-looping sounds need a new channel every time they play
	// TODO: Consider destroying the old channel with the same name.
	// Right now, if an fmod_ambient is triggered before it finishes playing, it'll play twice (offset by the time between triggers).
	// If we destroy the old channel with the same name, it'll stop playback of the old channel before playing the new channel.
	if (!looping)
	{
		channel = Fmod_CreateChannel(sound, channel_name.c_str(), fmod_sfx_group, false, volume);
		if (!channel) return false; // TODO: Report warning about failure to play here

		channel->set3DAttributes(&fmod_pos, &vel);
		channel->set3DMinMaxDistance(min_atten, max_atten);
		channel->setPitch(pitch);
		channel->setPaused(false);
	}

	// If looping, find the existing channel
	else
	{
		auto channel_iter = fmod_channels.find(channel_name);

		if (channel_iter == fmod_channels.end())
		{
			// TODO: send looping and volume info from entity
			channel = Fmod_CreateChannel(sound, channel_name.c_str(), fmod_sfx_group, true, volume);
			if (!channel) return false; // TODO: Report warning about failure to play here
		}
		else channel = channel_iter->second;

		channel->set3DAttributes(&fmod_pos, &vel);
		channel->set3DMinMaxDistance(min_atten, max_atten);
		channel->setPitch(pitch);

		// When a looping fmod_ambient gets used, by default it'll flip the status of paused
		bool paused = false;
		channel->getPaused(&paused);
		channel->setPaused(!paused);
	}

	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodTrk(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string sound_path = std::string(READ_STRING());
	bool looping = READ_BYTE();
	// TODO: Clean this up and put all the reads together for more visual clarity in the code

	float volume = READ_COORD();
	float pitch = READ_COORD();

	// TODO: sanitize inputs

	if (strlen(sound_path.c_str()) < 1)
	{
		if (fmod_current_track)
			fmod_current_track->stop();
		return 0;
	};
	FMOD::Sound* sound = NULL;

	// TODO: Separate finding the sound into its own function
	auto sound_iter = fmod_tracks.find(sound_path);

	if (sound_iter == fmod_tracks.end())
	{
		_Fmod_Report("WARNING", "Attempting to play track " + sound_path + " without caching it. Add it to your tracks.txt!");
		_Fmod_Report("INFO", "Attempting to cache and play track " + sound_path);
		sound = Fmod_CacheSound(sound_path.c_str(), true);
		if (!sound)
		{
			sound_path = "../valve/" + sound_path;
			sound = Fmod_CacheSound(sound_path.c_str(), true);
			if (!sound)
				return false; // Error will be reported by Fmod_CacheSound
		}
	}
	else sound = sound_iter->second;

	// Create a fresh channel every time a track plays
	if (fmod_current_track) fmod_current_track->stop();
	FMOD_RESULT result = fmod_system->playSound(sound, fmod_mp3_group, true, &fmod_current_track);
	if (!_Fmod_Result_OK(&result)) return false; // TODO: investigate if a failure here might create a memory leak

    // Set channel properties
	fmod_current_track->setVolume(volume);
	if (looping)
	{
		// TODO: See why this doesn't work as expected
		/*FMOD_MODE mode;
		fmod_current_track->getMode(&mode);
		fmod_current_track->setMode(mode | FMOD_LOOP_NORMAL);*/

		fmod_current_track->setMode(FMOD_LOOP_NORMAL);
	}

	fmod_current_track->setPitch(pitch);
	fmod_current_track->setPaused(false);

	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodRev(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Vector pos;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();

	float min_dist = READ_COORD();
	float max_dist = READ_COORD();

	int preset = READ_BYTE();

	FMOD_VECTOR fmod_pos = _Fmod_HLVecToFmodVec(pos);

	Fmod_CreateReverbSphere(&fmod_reverb_properties[preset], &fmod_pos, min_dist, max_dist);

	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodPause(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string channel_name = std::string(READ_STRING());

	FMOD::Channel *channel = NULL;

	if (channel_name == "fmod_current_track") 
	{
		channel = fmod_current_track;
	}

	else
	{
		auto it = fmod_channels.find(channel_name);
		if (it == fmod_channels.end())
		{
			_Fmod_Report("WARNING", "Tried to play/pause unknown channel " + channel_name);
			return false;
		}
		channel = it->second;
	}

	if (channel)
	{
		bool paused = false;
		channel->getPaused(&paused);
		channel->setPaused(!paused);
	}
	return true;
}

bool CHudFmodPlayer::MsgFunc_FmodSeek(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string channel_name = std::string(READ_STRING());
	float seek_to = READ_COORD();
	unsigned int position = (unsigned int)(seek_to * 1000); // convert seconds to milliseconds

	FMOD::Channel *channel = NULL;

	// TODO: Maybe consolidate finding the channel into a reusable function
	if (channel_name == "fmod_current_track") 
	{
		channel = fmod_current_track;
	}

	else
	{
		auto it = fmod_channels.find(channel_name);
		if (it == fmod_channels.end())
		{
			_Fmod_Report("WARNING", "Tried to play/pause unknown channel " + channel_name);
			return false;
		}
		channel = it->second;
	}

	if (channel)
	{
		channel->setPosition(position, FMOD_TIMEUNIT_MS);
	}
	return true;
}

void CHudFmodPlayer::PlayAmbSound(const char* sample, bool looping, Vector pos, float volume, float min_atten, float max_atten, float pitch, Vector vvel)
{
	std::string msg = std::string(sample);

	FMOD_VECTOR fmod_pos = _Fmod_HLVecToFmodVec(pos);

	// TODO: sanitize inputs

	FMOD_VECTOR vel;
	vel.x = vvel.x;
	vel.y = vvel.y;
	vel.z = vvel.z;
	std::string channel_name = msg.substr(0, msg.find('\n'));
	std::string sound_path = msg.substr(msg.find('\n') + 1, std::string::npos);

	FMOD::Sound* sound = NULL;
	FMOD::Channel* channel = NULL;

	// TODO: Separate finding the sound into its own function
	auto sound_iter = fmod_cached_sounds.find(sound_path);

	if (sound_iter == fmod_cached_sounds.end())
	{
		_Fmod_Report("WARNING", "Entity " + channel_name + " playing uncached sound " + sound_path +
									". Add the sound to your [MAPNAME].bsp_soundcache.txt file.");
		_Fmod_Report("INFO", "Attempting to cache and play sound " + sound_path);
		sound = Fmod_CacheSound(sound_path.c_str(), false);
		if (!sound)
			return; // Error will be reported by Fmod_CacheSound
	}
	else
		sound = sound_iter->second;

	// Non-looping sounds need a new channel every time they play
	// TODO: Consider destroying the old channel with the same name.
	// Right now, if an fmod_ambient is triggered before it finishes playing, it'll play twice (offset by the time between triggers).
	// If we destroy the old channel with the same name, it'll stop playback of the old channel before playing the new channel.
	if (!looping)
	{
		channel = Fmod_CreateChannel(sound, channel_name.c_str(), fmod_sfx_group, false, volume);
		if (!channel)
			return; // TODO: Report warning about failure to play here

		channel->set3DAttributes(&fmod_pos, &vel);
		channel->set3DMinMaxDistance(min_atten, max_atten);
		channel->setPitch(pitch);
		channel->setPaused(false);
	}

	// If looping, find the existing channel
	else
	{
		auto channel_iter = fmod_channels.find(channel_name);

		if (channel_iter == fmod_channels.end())
		{
			// TODO: send looping and volume info from entity
			channel = Fmod_CreateChannel(sound, channel_name.c_str(), fmod_sfx_group, true, volume);
			if (!channel)
				return; // TODO: Report warning about failure to play here
		}
		else
			channel = channel_iter->second;

		channel->set3DAttributes(&fmod_pos, &vel);
		channel->set3DMinMaxDistance(min_atten, max_atten);
		channel->setPitch(pitch);

		// When a looping fmod_ambient gets used, by default it'll flip the status of paused
		bool paused = false;
		channel->getPaused(&paused);
		channel->setPaused(!paused);
	}
}

bool CHudFmodPlayer::VidInit() { return true; }
bool CHudFmodPlayer::Draw(float flTime) { return true; }
void CHudFmodPlayer::Reset() { return; }