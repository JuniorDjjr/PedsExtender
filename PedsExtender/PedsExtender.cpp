#include "plugin.h"
#include "CStreaming.h"
#include "CPedModelInfo.h"
#include "CModelInfo.h"
#include "CGeneral.h"
#include "eVehicleClass.h"
#include "CGangWars.h"
#include "CPopulation.h"
#include "CTheZones.h"
#include "CWeather.h"
#include "CTimer.h"
#include "CPopCycle.h"
#include "..\injector\assembly.hpp"

using namespace plugin;
using namespace std;
using namespace injector;

fstream lg;

short(*pedGroups)[21];

bool bInitialized = false;

int iTimeLastCdeput = -1;

int iModelCdeput = -1;
int iModelDsher = -1;
int iModelsfpdm1 = -1;
int iModellvpdm1 = -1;
int iModelcspdm1 = -1;
int iModeldspdm1 = -1;
int iModelwmycd2 = -1;

int iModelsCopLS[9];
int iModelsCopLSCount = 0;
int iModelsCopSF[9];
int iModelsCopSFCount = 0;
int iModelsCopLV[9];
int iModelsCopLVCount = 0;
int iModelsCopC[9];
int iModelsCopCCount = 0;
int iModelsCopD[9];
int iModelsCopDCount = 0;

int ConsiderModelIdVariation(int iModel)
{
	int iResultModel = iModel;
	switch (iModel)
	{
	case 280:
		if (iModelsCopLSCount > 0) iResultModel = iModelsCopLS[CGeneral::GetRandomNumberInRange(0, iModelsCopLSCount)];
		break;
	case 281:
		if (iModelsCopSFCount > 0) iResultModel = iModelsCopSF[CGeneral::GetRandomNumberInRange(0, iModelsCopSFCount)];
		break;
	case 282:
		if (iModelsCopLVCount > 0) iResultModel = iModelsCopLV[CGeneral::GetRandomNumberInRange(0, iModelsCopLVCount)];
		break;
	case 283:
		if (iModelsCopCCount > 0) iResultModel = iModelsCopC[CGeneral::GetRandomNumberInRange(0, iModelsCopCCount)];
		break;
	case 288:
		if (iModelsCopDCount > 0) iResultModel = iModelsCopD[CGeneral::GetRandomNumberInRange(0, iModelsCopDCount)];
		break;
	}
	return iResultModel;
}

int CustomGetCopModelByZone()
{
	int iModel = 0;
	// countryside or desert
	if ( CTheZones::m_CurrLevel == eLevelName::LEVEL_NAME_COUNTRY_SIDE)
	{
		lg << "region " << CPopCycle::m_nCurrentZoneType << endl;
		//desert
		if (CPopCycle::m_nCurrentZoneType == 1)
		{
			iModel = iModelDsher;
		}
		else
		{
			if (iModelCdeput > 0 && (CTimer::m_snTimeInMilliseconds - iTimeLastCdeput) > 30000 && CGeneral::GetRandomNumberInRange(0, 5) == 1)
			{
				iTimeLastCdeput = CTimer::m_snTimeInMilliseconds;
				iModel = iModelCdeput;
			}
		}
	}
	if (iModel <= 0)
	{
		// default
		iModel = CallAndReturn<int, 0x407C00>();
	}
	return ConsiderModelIdVariation(iModel);
}

int GetBikerModel()
{
	int iModel = 0;

	if (CTheZones::m_CurrLevel == eLevelName::LEVEL_NAME_COUNTRY_SIDE)
	{
		if (CPopCycle::m_nCurrentZoneType == 1)
		{
			iModel = iModeldspdm1;
		}
		else
		{
			iModel = iModelcspdm1;
		}
	}
	if (CTheZones::m_CurrLevel == eLevelName::LEVEL_NAME_SAN_FIERRO)
	{
		iModel = iModelsfpdm1;
	}
	if (CTheZones::m_CurrLevel == eLevelName::LEVEL_NAME_LAS_VENTURAS)
	{
		iModel = iModellvpdm1;
	}
	if (iModel <= 0)
	{
		iModel = 284; // lapdm1
	}
	return ConsiderModelIdVariation(iModel);
}

int LoadSomePedModel(int gangId, bool loadNow)
{
	int model = MODEL_MALE01;
	if (gangId >= 0)
	{
		if (plugin::CallAndReturn<bool, 0x4439D0, int>(gangId)) // any ped loaded for this gang
		{
			if (CGangWars::PickStreamedInPedForThisGang(gangId, &model))
			{
				lg << "auto fixed gang " << model << " gang id " << gangId << endl;
				return model;
			}
		}
		model = pedGroups[CPopulation::m_TranslationArray[gangId + 18].pedGroupId][0];
		lg << "manual fixed gang " << model << " gang id " << gangId << endl;
	}
	else
	{
		CPedModelInfo *modelInfo;
		int tries = 0;
		do
		{
			tries++;
			if (tries > 30)
			{
				model = MODEL_MALE01;
				break;
			}
			model = plugin::CallAndReturn<int, 0x60FFD0, CVector*>(&FindPlayerPed(-1)->GetPosition());
			modelInfo = (CPedModelInfo *)CModelInfo::GetModelInfo(model);
			if (!modelInfo) continue;
		} while (
			!modelInfo ||
			(modelInfo->m_nStatType >= 4 && modelInfo->m_nStatType <= 13) ||
			(modelInfo->m_nStatType <= 26 && modelInfo->m_nStatType <= 32) ||
			(modelInfo->m_nStatType <= 38 && modelInfo->m_nStatType <= 41)
			);
	}

	if (model <= 0) model = MODEL_MALE01;

	if (loadNow && model != MODEL_MALE01)
	{
		CStreaming::RequestModel(model, eStreamingFlags::GAME_REQUIRED);
		CStreaming::LoadAllRequestedModels(false);
		CStreaming::SetModelIsDeletable(model);
		CStreaming::SetModelTxdIsDeletable(model);
	}
	return model;
}

bool RequestModelIfExists(char *name, int *index)
{
	lg << "Trying to find " << name << endl;
	CModelInfo::GetModelInfo(name, index);
	if (*index > 0)
	{
		lg << "Model found " << name << endl;
		// We are not on PS2 anymore, just keep the ped loaded on memory
		CStreaming::RequestModel(*index, eStreamingFlags::GAME_REQUIRED | eStreamingFlags::KEEP_IN_MEMORY);
		return true;
	}
	return false;
}

class FixMALE01
{
public:
    FixMALE01()
	{
		lg.open("PedsExtender.SA.log", fstream::out | fstream::trunc);
		lg << "v1.0 by Junior_Djjr - MixMods.com.br" << endl;

		if (GetModuleHandleA("FixMALE01.SA.asi")) {
			lg << "ERROR: PedsExtender is a new version of 'FixMALE01.SA.asi'. Please delete 'FixMALE01.SA.asi'." << endl;
			MessageBoxA(0, "ERROR: PedsExtender is a new version of 'FixMALE01.SA.asi'. Please delete 'FixMALE01.SA.asi'.", "PedsExtender.SA.asi", 0);
			return;
		}

		//CPopulation::m_PedGroups (limit adjuster compatibility)
		pedGroups = ReadMemory<short(*)[21]>(0x40AB69 + 4, true);
		
		// Ped
		MakeInline<0x613157, 0x613157 + 5>([](injector::reg_pack& regs)
		{
			int pedStats = *(int*)(regs.esp + 0x18 + 0x14);
			regs.esi = LoadSomePedModel(pedStats - 4, true);
			lg << "fixed ped to " << regs.esi << endl;
			*(uintptr_t*)(regs.esp - 0x4) = 0x613164;
		});

		// Vehicle
		MakeInline<0x6133D6, 0x6133D6 + 5>([](injector::reg_pack& regs)
		{
			//regs.esi = LoadSomePedModel();
			regs.esi = -1; // to use FixModel01ForVehicle; it's expected to return -1 in vanilla too
			lg << "will fix vehicle " << endl;
			*(uintptr_t*)(regs.esp - 0x4) = 0x6133E1;
		});

		struct FixModel01ForVehicle
		{
			void operator()(reg_pack& regs)
			{
				CVehicle* vehicle = reinterpret_cast<CVehicle*>(regs.edi);
				int type = *(int*)(regs.esp + 0x30);

				lg << "-- START fix for vehicle id " << vehicle->m_nModelIndex << " type " << type << endl;

				regs.eax = MODEL_MALE01;

				if (type >= 14 && type <= 23)
				{
					regs.eax = LoadSomePedModel(type - 14, false);
				}
				else
				{
					switch (vehicle->m_nModelIndex)
					{
					case MODEL_COMBINE:
					case MODEL_TRACTOR:
						regs.eax = LoadSomePedModel(-1, false);
						break;
					case MODEL_FREIGHT:
					case MODEL_STREAK:
						regs.eax = MODEL_BMOSEC;
						break;
					case MODEL_FREEWAY:
						regs.eax = CGeneral::GetRandomNumberInRange(MODEL_BIKERA, MODEL_BIKERB + 1);
						break;
					}

					CVehicleModelInfo *modelInfo = (CVehicleModelInfo *)CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
					if (regs.eax == MODEL_MALE01)
					{
						switch (modelInfo->m_nVehicleClass)
						{
						case eVehicleClass::CLASS_TAXI:
							regs.eax = CStreaming::GetDefaultCabDriverModel();
							break;
						case eVehicleClass::CLASS_WORKER:
						case eVehicleClass::CLASS_WORKERBOAT:
							if (vehicle->m_nModelIndex == MODEL_WALTON || vehicle->m_nModelIndex == MODEL_JOURNEY || vehicle->m_nModelIndex == MODEL_BOBCAT)
							{
								regs.eax = LoadSomePedModel(-1, false);
							}
							else
							{
								regs.eax = MODEL_WMYMECH;
							}
							break;
						}
					}
					lg << regs.eax << " for vehicle, class " << (int)modelInfo->m_nVehicleClass << endl;
				}

				if (regs.eax != MODEL_MALE01)
				{
					CStreaming::RequestModel(regs.eax, eStreamingFlags::GAME_REQUIRED);
					CStreaming::LoadAllRequestedModels(false);
					CStreaming::SetModelIsDeletable(regs.eax);
					CPedModelInfo *modelInfo = (CPedModelInfo *)CModelInfo::GetModelInfo(regs.eax);
					CStreaming::SetModelTxdIsDeletable(regs.eax);
					if (modelInfo && modelInfo->m_pRwObject)
					{
						regs.esi = modelInfo->m_nPedType;
					}
					else
					{
						regs.eax = MODEL_MALE01;
					}
					lg << "-- FINAL " << regs.eax << endl;
				}
				else
				{
					lg << "-- NO FIX " << regs.eax << endl;
				}
			}
		};
		MakeInline<FixModel01ForVehicle>(0x613B3E, 0x613B3E + 5);
		MakeInline<FixModel01ForVehicle>(0x613B51, 0x613B51 + 5);
		MakeInline<FixModel01ForVehicle>(0x613B71, 0x613B71 + 5);

		patch::RedirectCall(0x5DDCA8, CustomGetCopModelByZone, true);

		Events::initGameEvent.after += []
		{
			if (bInitialized) return;
			bInitialized = true;

			RequestModelIfExists("cdeput", &iModelCdeput);
			RequestModelIfExists("dsher", &iModelDsher);
			RequestModelIfExists("sfpdm1", &iModelsfpdm1);
			RequestModelIfExists("lvpdm1", &iModellvpdm1);
			RequestModelIfExists("cspdm1", &iModelcspdm1);
			RequestModelIfExists("dspdm1", &iModeldspdm1);
			RequestModelIfExists("wmycd2", &iModelwmycd2);

			char tempName[8] = { 0 };

			strcpy_s(tempName, 7, "lapd1");
			for (int i = 0; i < 9; ++i) {
				tempName[4] = (char)(50 + i);
				if (!RequestModelIfExists(tempName, &iModelsCopLS[i])) break;
				++iModelsCopLSCount;
			}
			if (iModelsCopLSCount > 0) { iModelsCopLS[iModelsCopLSCount] = 280; iModelsCopLSCount++; }

			strcpy_s(tempName, 7, "sfpd1");
			for (int i = 0; i < 9; ++i) {
				tempName[4] = (char)(50 + i);
				if (!RequestModelIfExists(tempName, &iModelsCopSF[i])) break;
				++iModelsCopSFCount;
			}
			if (iModelsCopSFCount > 0) { iModelsCopSF[iModelsCopSFCount] = 281; iModelsCopSFCount++; }

			strcpy_s(tempName, 7, "lvpd1");
			for (int i = 0; i < 9; ++i) {
				tempName[4] = (char)(50 + i);
				if (!RequestModelIfExists(tempName, &iModelsCopSF[i])) break;
				++iModelsCopLVCount;
			}
			if (iModelsCopLVCount > 0) { iModelsCopSF[iModelsCopLVCount] = 282; iModelsCopLVCount++; }

			strcpy_s(tempName, 7, "csher1");
			for (int i = 0; i < 10; ++i) {
				tempName[5] = (char)(49 + i);
				if (!RequestModelIfExists(tempName, &iModelsCopC[i])) break;
				++iModelsCopCCount;
			}
			if (iModelsCopCCount > 0) { iModelsCopC[iModelsCopCCount] = 283; iModelsCopCCount++; }

			strcpy_s(tempName, 7, "dsher1");
			for (int i = 0; i < 10; ++i) {
				tempName[5] = (char)(49 + i);
				if (!RequestModelIfExists(tempName, &iModelsCopD[i])) break;
				++iModelsCopDCount;
			}
			if (iModelsCopDCount > 0) { iModelsCopD[iModelsCopDCount] = 288; iModelsCopDCount++; }

			CStreaming::LoadAllRequestedModels(false);
			lg << "Ok" << endl;
		};

		//WriteMemory<uint32_t>(0x8A5AB0, 277, false);

		Events::processScriptsEvent += []
		{
			WriteMemory<uint8_t>(0x5DDD85 + 0, 0x68, true);
			WriteMemory<uint32_t>(0x5DDD85 + 1, GetBikerModel(), true);

			if (iModelwmycd2 > 0)
			{
				WriteMemory<uint32_t>(0x8A5AF4, (CTimer::m_FrameCounter % 2 == 0) ? 262 : iModelwmycd2, true);
				WriteMemory<uint32_t>(0x8A5AF8, (CTimer::m_FrameCounter % 2 == 0) ? 261 : iModelwmycd2, true);
			}
		};

		/*Events::onPauseAllSounds += [] {
			lg.flush();
		};*/
    }
} fixMALE01;
