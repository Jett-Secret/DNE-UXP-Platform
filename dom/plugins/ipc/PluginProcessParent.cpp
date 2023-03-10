/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/plugins/PluginProcessParent.h"

#include "base/string_util.h"
#include "base/process_util.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/plugins/PluginMessageUtils.h"
#include "nsThreadUtils.h"

using std::vector;
using std::string;

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::GeckoChildProcessHost;
using mozilla::plugins::LaunchCompleteTask;
using mozilla::plugins::PluginProcessParent;
using base::ProcessArchitecture;

PluginProcessParent::PluginProcessParent(const std::string& aPluginFilePath) :
    GeckoChildProcessHost(GeckoProcessType_Plugin),
    mPluginFilePath(aPluginFilePath),
    mTaskFactory(this),
    mMainMsgLoop(MessageLoop::current()),
    mRunCompleteTaskImmediately(false)
{
}

PluginProcessParent::~PluginProcessParent()
{
}

bool
PluginProcessParent::Launch(mozilla::UniquePtr<LaunchCompleteTask> aLaunchCompleteTask,
                            int32_t aSandboxLevel)
{
    if (aSandboxLevel != 0) {
        MOZ_ASSERT(false,
                   "Can't enable an NPAPI process sandbox for platform/build.");
    }

    ProcessArchitecture currentArchitecture = base::GetCurrentProcessArchitecture();
    uint32_t containerArchitectures = GetSupportedArchitecturesForProcessType(GeckoProcessType_Plugin);

    uint32_t pluginLibArchitectures = currentArchitecture;
#ifdef XP_MACOSX
    nsresult rv = GetArchitecturesForBinary(mPluginFilePath.c_str(), &pluginLibArchitectures);
    if (NS_FAILED(rv)) {
        // If the call failed just assume that we want the current architecture.
        pluginLibArchitectures = currentArchitecture;
    }
#endif

    ProcessArchitecture selectedArchitecture = currentArchitecture;
    if (!(pluginLibArchitectures & containerArchitectures & currentArchitecture)) {
        // Prefererence in order: x86_64, i386, PPC. The only particularly important thing
        // about this order is that we'll prefer 64-bit architectures first.
        if (base::PROCESS_ARCH_X86_64 & pluginLibArchitectures & containerArchitectures) {
            selectedArchitecture = base::PROCESS_ARCH_X86_64;
        }
        else if (base::PROCESS_ARCH_I386 & pluginLibArchitectures & containerArchitectures) {
            selectedArchitecture = base::PROCESS_ARCH_I386;
        }
        else if (base::PROCESS_ARCH_PPC & pluginLibArchitectures & containerArchitectures) {
            selectedArchitecture = base::PROCESS_ARCH_PPC;
        }
        else if (base::PROCESS_ARCH_ARM & pluginLibArchitectures & containerArchitectures) {
          selectedArchitecture = base::PROCESS_ARCH_ARM;
        }
        else if (base::PROCESS_ARCH_MIPS & pluginLibArchitectures & containerArchitectures) {
          selectedArchitecture = base::PROCESS_ARCH_MIPS;
        }
        else {
            return false;
        }
    }

    mLaunchCompleteTask = mozilla::Move(aLaunchCompleteTask);

    vector<string> args;
    args.push_back(MungePluginDsoPath(mPluginFilePath));

    bool result = AsyncLaunch(args, selectedArchitecture);
    if (!result) {
        mLaunchCompleteTask = nullptr;
    }
    return result;
}

void
PluginProcessParent::Delete()
{
  MessageLoop* currentLoop = MessageLoop::current();
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();

  if (currentLoop == ioLoop) {
      delete this;
      return;
  }

  ioLoop->PostTask(NewNonOwningRunnableMethod(this, &PluginProcessParent::Delete));
}

void
PluginProcessParent::SetCallRunnableImmediately(bool aCallImmediately)
{
    mRunCompleteTaskImmediately = aCallImmediately;
}

/**
 * This function exists so that we may provide an additional level of
 * indirection between the task being posted to main event loop (a
 * RunnableMethod) and the launch complete task itself. This is needed
 * for cases when both WaitUntilConnected or OnChannel* race to invoke the
 * task.
 */
void
PluginProcessParent::RunLaunchCompleteTask()
{
    if (mLaunchCompleteTask) {
        mLaunchCompleteTask->Run();
        mLaunchCompleteTask = nullptr;
    }
}

bool
PluginProcessParent::WaitUntilConnected(int32_t aTimeoutMs)
{
    bool result = GeckoChildProcessHost::WaitUntilConnected(aTimeoutMs);
    if (mRunCompleteTaskImmediately && mLaunchCompleteTask) {
        if (result) {
            mLaunchCompleteTask->SetLaunchSucceeded();
        }
        RunLaunchCompleteTask();
    }
    return result;
}

void
PluginProcessParent::OnChannelConnected(int32_t peer_pid)
{
    GeckoChildProcessHost::OnChannelConnected(peer_pid);
    if (mLaunchCompleteTask && !mRunCompleteTaskImmediately) {
        mLaunchCompleteTask->SetLaunchSucceeded();
        mMainMsgLoop->PostTask(mTaskFactory.NewRunnableMethod(
                                   &PluginProcessParent::RunLaunchCompleteTask));
    }
}

void
PluginProcessParent::OnChannelError()
{
    GeckoChildProcessHost::OnChannelError();
    if (mLaunchCompleteTask && !mRunCompleteTaskImmediately) {
        mMainMsgLoop->PostTask(mTaskFactory.NewRunnableMethod(
                                   &PluginProcessParent::RunLaunchCompleteTask));
    }
}

bool
PluginProcessParent::IsConnected()
{
    mozilla::MonitorAutoLock lock(mMonitor);
    return mProcessState == PROCESS_CONNECTED;
}

