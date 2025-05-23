#include "StdInc.h"

#include "TaskComplexJump.h"

#include "TaskSimpleJump.h"
#include "TaskSimpleHitHead.h"
#include "TaskComplexInAirAndLand.h"
#include "TaskSimpleClimb.h"

void CTaskComplexJump::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexJump, 0x870570, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x67A030);
    RH_ScopedInstall(CreateSubTask, 0x67D980);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x67FD10);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x67FC00);
    RH_ScopedVMTInstall(Clone, 0x67C5A0);
    RH_ScopedVMTInstall(MakeAbortable, 0x67A070);
}

CTaskComplexJump* CTaskComplexJump::Constructor(eForceClimb jumpType) {
    this->CTaskComplexJump::CTaskComplexJump(jumpType);
    return this;
}

// 0x67A030
CTaskComplexJump::CTaskComplexJump(eForceClimb forceClimb) : CTaskComplex() {
    m_ForceClimb = forceClimb;
    m_UsePlayerLaunchForce = false;
}

// 0x67C5A0
CTask* CTaskComplexJump::Clone() const {
    auto newTask = new CTaskComplexJump(m_ForceClimb);
    newTask->m_UsePlayerLaunchForce = this->m_UsePlayerLaunchForce;
    return newTask;
}

// 0x67FD10
CTask* CTaskComplexJump::CreateFirstSubTask(CPed* ped) {
    return CreateSubTask(TASK_SIMPLE_JUMP, ped);
}

// 0x67FC00
CTask* CTaskComplexJump::CreateNextSubTask(CPed* ped) {
    eTaskType subTaskType = m_pSubTask->GetTaskType();

    if (subTaskType == TASK_SIMPLE_CLIMB)
        return CreateSubTask(ped->bIsInTheAir ? TASK_COMPLEX_IN_AIR_AND_LAND : TASK_FINISHED, ped);

    if (subTaskType == TASK_SIMPLE_HIT_HEAD || subTaskType == TASK_COMPLEX_IN_AIR_AND_LAND) {
        ped->bIsLanding = false;
        return CreateSubTask(TASK_FINISHED, ped);
    }

    if (subTaskType == TASK_SIMPLE_JUMP) {
        auto jumpTask = reinterpret_cast<CTaskSimpleJump*>(m_pSubTask);

        if (!jumpTask->m_bLaunchAnimStarted) {
            ped->bIsLanding = false;
            return CreateSubTask(TASK_FINISHED, ped);
        } if (jumpTask->m_bIsJumpBlocked) {
            ped->bIsLanding = true;
            return CreateSubTask(TASK_SIMPLE_HIT_HEAD, ped);
        } else if (jumpTask->m_pClimbEntity && m_ForceClimb != eForceClimb::DISABLE) {
            ped->bIsInTheAir = true;
            return CreateSubTask(TASK_SIMPLE_CLIMB, ped);
        } else {
            ped->bIsInTheAir = true;
            return CreateSubTask(TASK_COMPLEX_IN_AIR_AND_LAND, ped);
        }
    }

    return nullptr;
}

// 0x67A070
bool CTaskComplexJump::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority == ABORT_PRIORITY_URGENT && event) {
        if (event->GetEventType() == EVENT_DAMAGE) {
            const auto pDamageEvent = static_cast<const CEventDamage*>(event);
            if (pDamageEvent->m_weaponType == WEAPON_FALL && pDamageEvent->m_damageResponse.m_bHealthZero && pDamageEvent->m_bAddToEventGroup) {
                ped->bIsInTheAir = false;
                ped->bIsLanding = false;
                return true;
            }
        } else if (event->GetEventType() == EVENT_DEATH) {
            ped->bIsInTheAir = false;
            ped->bIsLanding = false;
            return true;
        }
    }

    if (m_pSubTask->MakeAbortable(ped, priority, event)) {
        ped->bIsInTheAir = false;
        ped->bIsLanding = false;
        return true;
    }

    return false;
}

// 0x67D980
CTask* CTaskComplexJump::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_HIT_HEAD:
        return new CTaskSimpleHitHead();
    case TASK_FINISHED:
        ped->bIsInTheAir = false;
        return nullptr;
    case TASK_SIMPLE_JUMP: {
        auto t = new CTaskSimpleJump(m_ForceClimb == eForceClimb::FORCE);
        if (m_UsePlayerLaunchForce || CPedGroups::IsInPlayersGroup(ped))
            t->m_bHighJump = true;
        return t;
    }
    case TASK_SIMPLE_CLIMB: {
        if (const auto* const tJump = notsa::dyn_cast_if_present<CTaskSimpleJump>(m_pSubTask)) {
            return new CTaskSimpleClimb(
                tJump->m_pClimbEntity,
                tJump->m_vecClimbPos,
                tJump->m_fClimbAngle,
                tJump->m_nClimbSurfaceType,
                tJump->m_vecClimbPos.z - ped->GetPosition().z < CTaskSimpleClimb::ms_fMinForStretchGrab ? CLIMB_PULLUP : CLIMB_GRAB,
                m_ForceClimb == eForceClimb::FORCE
            );
        }
        return new CTaskComplexInAirAndLand(true, false);
    }
    case TASK_COMPLEX_IN_AIR_AND_LAND: {
        auto t = new CTaskComplexInAirAndLand(true, false);
        if (const auto tClimb = notsa::dyn_cast_if_present<CTaskSimpleClimb>(m_pSubTask)) {
            t->m_bInvalidClimb = tClimb->GetIsInvalidClimb();
        }
        return t;
    }
    default:
        return nullptr;
    }
}
