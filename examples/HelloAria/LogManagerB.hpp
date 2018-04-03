#pragma once

#include "LogManager.hpp"
using namespace std;

namespace ARIASDK_NS_BEGIN {
    class ModuleB : public ILogConfiguration {};
    class LogManagerB : public LogManagerBase<ModuleB> {};
    DEFINE_LOGMANAGER(LogManagerB, ModuleB);
} ARIASDK_NS_END
