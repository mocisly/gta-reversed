#pragma once

#include "EventGroup.h"
#include <Vector.h>

class CPedGroup;
class CEntity;
class CPed;

class NOTSA_EXPORT_VTABLE CEventGlobalGroup : public CEventGroup {
public:
    static void InjectHooks();

    CEventGlobalGroup(CPed* ped) : CEventGroup(ped) {};
    ~CEventGlobalGroup() override = default;

    float GetSoundLevel(CEntity* entity, CVector& position);
    void AddEventsToPed(CPed* ped);
    void AddEventsToGroup(CPedGroup* pedGroup);
};

