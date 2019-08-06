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

Coyote::StatusCode Coyote_Take(Coyote::Session *Sess, const int32_t PK)
{
	return Sess->Take(PK);
}

Coyote::StatusCode Coyote_Pause(Coyote::Session *Sess, const int32_t PK)
{
	return Sess->Pause(PK);
}

Coyote::StatusCode Coyote_End(Coyote::Session *Sess, const int32_t PK)
{
	return Sess->End(PK);
}

Coyote::StatusCode Coyote_TakePrev(Coyote::Session *Sess)
{
	return Sess->TakePrev();
}

Coyote::StatusCode Coyote_RestartService(Coyote::Session *Sess)
{
	return Sess->RestartService();
}

Coyote::StatusCode Coyote_TakeNext(Coyote::Session *Sess)
{
	return Sess->TakeNext();
}

Coyote::StatusCode Coyote_ShutdownCoyote(Coyote::Session *Sess)
{
	return Sess->ShutdownCoyote();
}

Coyote::StatusCode Coyote_BeginUpdate(Coyote::Session *Sess)
{
	return Sess->BeginUpdate();
}

Coyote::StatusCode Coyote_SoftRebootCoyote(Coyote::Session *Sess)
{
	return Sess->SoftRebootCoyote();
}
Coyote::StatusCode Coyote_RebootCoyote(Coyote::Session *Sess)
{
	return Sess->RebootCoyote();
}

Coyote::StatusCode Coyote_SeekTo(Coyote::Session *Sess, const int32_t PK, const uint32_t TimeIndex)
{
	return Sess->SeekTo(PK, TimeIndex);
}

void CoyoteObject_Destroy(void *Object)
{
	Coyote::BaseObject *Obj = static_cast<Coyote::BaseObject*>(Object);
	
	delete Obj;
}

Coyote::StatusCode Coyote_GetIP(Coyote::Session *Sess, const int32_t AdapterID, Coyote_NetworkInfo **Out)
{
	Coyote::NetworkInfo *NetInfo = new Coyote::NetworkInfo{};
	
	const Coyote::StatusCode Status = Sess->GetIP(AdapterID, *NetInfo);
	
	if (Status != Coyote::COYOTE_STATUS_OK)
	{
		delete NetInfo;
		return Status;
	}
	
	*Out = NetInfo;
	
	return Status;
}

Coyote::StatusCode Coyote_SetIP(Coyote::Session *Sess, const Coyote_NetworkInfo *Input)
{
	const Coyote::NetworkInfo *NetInfo = static_cast<const Coyote::NetworkInfo*>(Input);
	
	return Sess->SetIP(*NetInfo);
}

Coyote::StatusCode Coyote_GetServerVersion(Coyote::Session *Sess, const char **const VersionStringOut)
{
	std::string VersionString;
	
	const Coyote::StatusCode Status = Sess->GetServerVersion(VersionString);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	*VersionStringOut = strdup(VersionString.c_str());
	
	return Status;
}

Coyote::StatusCode Coyote_DetectUpdate(Coyote::Session *Sess, bool *IsDetected, char *OutBuf, const size_t OutBufCapacity)
{
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

Coyote::StatusCode Coyote_GetPresets(Coyote::Session *Sess, Coyote_PresetArray *PresetsOut, const int32_t TimeoutMS)
{
	std::vector<Coyote::Preset> *Presets = new std::vector<Coyote::Preset>{};
	
	const Coyote::StatusCode Status = Sess->GetPresets(*Presets, TimeoutMS);
	
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
	return Sess->InstallAsset(AssetPath);
}
Coyote::StatusCode Coyote_DeleteAsset(Coyote::Session *Sess, const char *AssetPath)
{
	return Sess->DeleteAsset(AssetPath);
}

Coyote::StatusCode Coyote_RenameAsset(Coyote::Session *Sess, const char *CurrentName, const char *NewName)
{
	return Sess->RenameAsset(CurrentName, NewName);
}

Coyote::StatusCode Coyote_ReorderPresets(Coyote::Session *Sess, const int32_t PK1, const int32_t PK2)
{
	return Sess->ReorderPresets(PK1, PK2);
}

Coyote::StatusCode Coyote_DeletePreset(Coyote::Session *Sess, const int32_t PK)
{
	return Sess->DeletePreset(PK);
}

Coyote::StatusCode Coyote_LoadPreset(Coyote::Session *Sess, const Coyote_Preset *Input_)
{
	const Coyote::Preset *Input = static_cast<const Coyote::Preset*>(Input_);
	
	return Sess->LoadPreset(*Input);
}

Coyote::StatusCode Coyote_UpdatePreset(Coyote::Session *Sess, const Coyote_Preset *Input_)
{
	const Coyote::Preset *Input = static_cast<const Coyote::Preset*>(Input_);
	
	return Sess->UpdatePreset(*Input);
}

Coyote::StatusCode Coyote_CreatePreset(Coyote::Session *Sess, const Coyote_Preset *Input_)
{
	const Coyote::Preset *Input = static_cast<const Coyote::Preset*>(Input_);
	
	return Sess->CreatePreset(*Input);
}


Coyote::StatusCode Coyote_GetAssets(Coyote::Session *Sess, Coyote_AssetArray *PresetsOut, const int32_t TimeoutMS)
{
	std::vector<Coyote::Asset> *Assets = new std::vector<Coyote::Asset>{};
	
	const Coyote::StatusCode Status = Sess->GetAssets(*Assets, TimeoutMS);
	
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

} ///end of extern "C"
