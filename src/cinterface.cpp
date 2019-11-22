#include "include/macros.h"
#include "common.h"
#include "include/datastructures.h"
#include "include/session.h"

extern "C"
{
	
Coyote::Session *CoyoteSession_New(const char *Host)
{
	return new Coyote::Session(Host);
}

void CoyoteSession_Destroy(Coyote::Session *Sess)
{
	delete Sess;
}

Coyote::StatusCode Coyote_GetTimeCode(Coyote::Session *Sess, const int32_t PK, Coyote_TimeCode *OutTC)
{
	assert(Sess != nullptr);
	assert(OutTC != nullptr);

	Coyote::TimeCode TC{};
	
	const Coyote::StatusCode Status = Sess->GetTimeCode(TC, PK);
	
	*OutTC = TC; //Slice that thing up
	
	return Status;
}

Coyote::StatusCode Coyote_Take(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);

	return Sess->Take(PK);
}

Coyote::StatusCode Coyote_Pause(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);

	return Sess->Pause(PK);
}

Coyote::StatusCode Coyote_SetPause(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);

	return Sess->SetPause(PK);
}

Coyote::StatusCode Coyote_UnsetPause(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);

	return Sess->UnsetPause(PK);
}

Coyote::StatusCode Coyote_SetPausedState(Coyote::Session *Sess, const int32_t PK, const bool Value)
{
	assert(Sess != nullptr);

	return Sess->SetPausedState(PK, Value);
}

Coyote::StatusCode Coyote_End(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);

	return Sess->End(PK);
}

Coyote::StatusCode Coyote_SelectPrev(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->SelectPrev();
}

Coyote::StatusCode Coyote_SelectNext(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->SelectNext();
}
Coyote::StatusCode Coyote_TakePrev(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->TakePrev();
}

Coyote::StatusCode Coyote_TakeNext(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->TakeNext();
}

Coyote::StatusCode Coyote_RestartService(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->RestartService();
}

Coyote::StatusCode Coyote_ShutdownCoyote(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->ShutdownCoyote();
}

Coyote::StatusCode Coyote_BeginUpdate(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->BeginUpdate();
}

Coyote::StatusCode Coyote_SoftRebootCoyote(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->SoftRebootCoyote();
}
Coyote::StatusCode Coyote_RebootCoyote(Coyote::Session *Sess)
{
	assert(Sess != nullptr);

	return Sess->RebootCoyote();
}

Coyote::StatusCode Coyote_SeekTo(Coyote::Session *Sess, const int32_t PK, const uint32_t TimeIndex)
{
	assert(Sess != nullptr);

	return Sess->SeekTo(PK, TimeIndex);
}

void CoyoteObject_Destroy(void *Object)
{
	Coyote::BaseObject *Obj = static_cast<Coyote::BaseObject*>(Object);
	
	delete Obj;
}

Coyote::StatusCode Coyote_GetIP(Coyote::Session *Sess, const int32_t AdapterID, Coyote_NetworkInfo *Out)
{
	assert(Sess != nullptr);

	Coyote::NetworkInfo *NetInfo = new Coyote::NetworkInfo{};
	
	const Coyote::StatusCode Status = Sess->GetIP(AdapterID, *NetInfo);
	
	if (Status != Coyote::COYOTE_STATUS_OK)
	{
		delete NetInfo;
		return Status;
	}
	
	*Out = *NetInfo;
	
	return Status;
}

Coyote::StatusCode Coyote_SetIP(Coyote::Session *Sess, const Coyote_NetworkInfo *Input)
{
	const Coyote::NetworkInfo *NetInfo = static_cast<const Coyote::NetworkInfo*>(Input);
	
	return Sess->SetIP(*NetInfo);
}

Coyote::StatusCode Coyote_GetServerVersion(Coyote::Session *Sess, const char **const VersionStringOut)
{
	assert(Sess != nullptr);

	std::string VersionString;
	
	const Coyote::StatusCode Status = Sess->GetServerVersion(VersionString);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	*VersionStringOut = strdup(VersionString.c_str());
	
	return Status;
}

Coyote::StatusCode Coyote_DetectUpdate(Coyote::Session *Sess, bool *IsDetected, char *OutBuf, const size_t OutBufCapacity)
{
	assert(Sess != nullptr);

	bool Detected = false;
	std::string Version;
	
	const Coyote::StatusCode Code = Sess->DetectUpdate(Detected, &Version);
	
	if (Code != Coyote::COYOTE_STATUS_OK) return Code;
	
	if (IsDetected)
	{
		*IsDetected = Detected;
	}
	
	if (OutBuf && OutBufCapacity)
	{
		strncpy(OutBuf, Version.c_str(), OutBufCapacity - 1);
		OutBuf[OutBufCapacity - 1] = '\0';
	}
	
	return Code;
}

Coyote::StatusCode Coyote_GetHardwareState(Coyote::Session *Sess, Coyote_HardwareState **StateOut)
{
	assert(Sess != nullptr);

	Coyote::HardwareState *State = new Coyote::HardwareState{};
	
	const Coyote::StatusCode Status = Sess->GetHardwareState(*State);
	
	if (Status != Coyote::COYOTE_STATUS_OK)
	{
		delete State;
		return Status;
	}
	
	*StateOut = State;
	
	return Status;
}

Coyote::StatusCode Coyote_GetPresets(Coyote::Session *Sess, Coyote_PresetArray *PresetsOut)
{
	assert(Sess != nullptr);

	std::vector<Coyote::Preset> *Presets = new std::vector<Coyote::Preset>{};
	
	const Coyote::StatusCode Status = Sess->GetPresets(*Presets);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	PresetsOut->_Handle = Presets;
	PresetsOut->Data = (Coyote_Preset**)calloc(Presets->size(), sizeof(Coyote_Preset*));
	PresetsOut->Length = Presets->size();
	
	Coyote_Preset **Worker = PresetsOut->Data;
	
	for (Coyote::Preset &Item : *Presets)
	{
		*(Worker++) = &Item;
	}
	return Status;
}

Coyote::StatusCode Coyote_InstallAsset(Coyote::Session *Sess, const char *AssetPath)
{
	assert(Sess != nullptr);

	return Sess->InstallAsset(AssetPath);
}
Coyote::StatusCode Coyote_DeleteAsset(Coyote::Session *Sess, const char *AssetPath)
{
	assert(Sess != nullptr);

	return Sess->DeleteAsset(AssetPath);
}

Coyote::StatusCode Coyote_RenameAsset(Coyote::Session *Sess, const char *CurrentName, const char *NewName)
{
	assert(Sess != nullptr);

	return Sess->RenameAsset(CurrentName, NewName);
}

Coyote::StatusCode Coyote_ReorderPresets(Coyote::Session *Sess, const int32_t PK1, const int32_t PK2)
{
	assert(Sess != nullptr);

	return Sess->ReorderPresets(PK1, PK2);
}

Coyote::StatusCode Coyote_SelectPreset(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);
	
	return Sess->SelectPreset(PK);
}

Coyote::StatusCode Coyote_DeletePreset(Coyote::Session *Sess, const int32_t PK)
{
	assert(Sess != nullptr);

	return Sess->DeletePreset(PK);
}

Coyote::StatusCode Coyote_LoadPreset(Coyote::Session *Sess, const Coyote_Preset *Input_)
{
	assert(Sess != nullptr);

	const Coyote::Preset *Input = static_cast<const Coyote::Preset*>(Input_);
	
	return Sess->LoadPreset(*Input);
}

Coyote::StatusCode Coyote_UpdatePreset(Coyote::Session *Sess, const Coyote_Preset *Input_)
{
	assert(Sess != nullptr);

	const Coyote::Preset *Input = static_cast<const Coyote::Preset*>(Input_);
	
	return Sess->UpdatePreset(*Input);
}

Coyote::StatusCode Coyote_CreatePreset(Coyote::Session *Sess, const Coyote_Preset *Input_)
{
	assert(Sess != nullptr);

	const Coyote::Preset *Input = static_cast<const Coyote::Preset*>(Input_);
	
	return Sess->CreatePreset(*Input);
}


Coyote::StatusCode Coyote_GetAssets(Coyote::Session *Sess, Coyote_AssetArray *PresetsOut)
{
	assert(Sess != nullptr);

	std::vector<Coyote::Asset> *Assets = new std::vector<Coyote::Asset>{};
	
	const Coyote::StatusCode Status = Sess->GetAssets(*Assets);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	PresetsOut->_Handle = Assets;
	PresetsOut->Data = (Coyote_Asset**)calloc(Assets->size(), sizeof(Coyote_Asset*));
	PresetsOut->Length = Assets->size();
	
	Coyote_Asset **Worker = PresetsOut->Data;
	
	for (Coyote::Asset &Item : *Assets)
	{
		*(Worker++) = &Item;
	}
	return Status;
}

Coyote::StatusCode Coyote_GetDisks(Coyote::Session *Sess, const char **OutString)
{
	assert(Sess != nullptr);

	std::vector<std::string> Disks;
	
	const Coyote::StatusCode Status = Sess->GetDisks(Disks);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::string TempString;
	TempString.reserve(Disks.size() * 2);
	
	for (const std::string &String : Disks)
	{
		TempString += String + ',';
	}
	
	TempString.pop_back(); //Delete trailing comma.
	
	*OutString = strdup(TempString.c_str());
	
	return Status;
}

Coyote::StatusCode Coyote_EjectDisk(Coyote::Session *Sess, const char *DriveLetter)
{
	assert(Sess != nullptr);

	return Sess->EjectDisk(DriveLetter);
}

void Coyote_AssetArray_Destroy(Coyote_AssetArray *Arr)
{
	auto Ptr = static_cast<std::vector<Coyote::Asset>* >(Arr->_Handle);
	
	//Delete underlying vector
	delete Ptr;
	
	//Free C array of pointers
	free(Arr->Data);
	
	//Wipe it clean to prevent memory bugs with use-after-free etc. Better than corruption.
	memset(Arr, 0, sizeof *Arr);
}

void Coyote_PresetArray_Destroy(Coyote_PresetArray *Arr)
{
	auto Ptr = static_cast<std::vector<Coyote::Preset>* >(Arr->_Handle);
	
	//Delete underlying vector
	delete Ptr;
	
	//Free C array of pointers
	free(Arr->Data);
	
	//Wipe it clean to prevent memory bugs with use-after-free etc. Better than corruption.	
	memset(Arr, 0, sizeof *Arr);
}

Coyote::StatusCode Coyote_SetHardwareMode(Coyote::Session *Sess, const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate)
{
	assert(Sess != nullptr);
	
	return Sess->SetHardwareMode(Resolution, RefreshRate);
}

Coyote::StatusCode Coyote_InitializeCoyote(Coyote::Session *Sess, const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate)
{
	assert(Sess != nullptr);
	
	return Sess->InitializeCoyote(Resolution, RefreshRate);
}

} //End of C linkage
