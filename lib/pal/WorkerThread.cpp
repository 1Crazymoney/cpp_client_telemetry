// clang-format off
#include "pal/WorkerThread.hpp"
#include "pal/PAL.hpp"

#if defined(MATSDK_PAL_CPP11) || defined(MATSDK_PAL_WIN32)

/* Maximum scheduler interval for SDK is 1 hour required for clamping in case of monotonic clock drift */
#define MAX_FUTURE_DELTA_MS (60 * 60 * 1000)

// Polling interval for task cancellation can be customized at compile-time
#ifndef TASK_CANCEL_WAIT_MS
#define TASK_CANCEL_WAIT_MS 50
#endif

namespace PAL_NS_BEGIN {

    class WorkerThreadShutdownItem : public Task
    {
    public:
        WorkerThreadShutdownItem() :
            Task()
        {
            Type = MAT::Task::Shutdown;
        }
    };

    class WorkerThread : public ITaskDispatcher
    {
    protected:
        std::thread           m_hThread;

        // TODO: [MG] - investigate all the cases why we need recursive here
        std::recursive_mutex  m_lock;

        std::list<MAT::Task*> m_queue;
        std::list<MAT::Task*> m_timerQueue;
        Event                 m_event;
        MAT::Task*            m_itemInProgress;
        int count = 0;
    
    public:

        WorkerThread()
        {
            m_hThread = std::thread(WorkerThread::threadFunc, static_cast<void*>(this));
            LOG_INFO("Started new thread %u", m_hThread.get_id());
        }

        ~WorkerThread()
        {
            Join();
        }

        void Join() override
        {
            auto item = new WorkerThreadShutdownItem();
            Queue(item);
            std::thread::id this_id = std::this_thread::get_id();
            try {
                if (m_hThread.joinable() && (m_hThread.get_id() != this_id))
                    m_hThread.join();
                else
                    m_hThread.detach();
            }
            catch (...) {};

            // TODO: [MG] - investigate how often that happens.
            // Side-effect is that we have a queued work item discarded on shutdown.
            if (!m_queue.empty())
            {
                LOG_WARN("m_queue is not empty!");
            }
            if (!m_timerQueue.empty())
            {
                LOG_WARN("m_timerQueue is not empty!");
            }
        }

        void Queue(MAT::Task* item) override
        {
            // TODO: [MG] - show item type
            LOG_INFO("queue item=%p", &item);
            LOCKGUARD(m_lock);
            if (item->Type == MAT::Task::TimedCall) {
                auto it = m_timerQueue.begin();
                while (it != m_timerQueue.end() && (*it)->TargetTime < item->TargetTime) {
                    ++it;
                }
                m_timerQueue.insert(it, item);
            }
            else {
                m_queue.push_back(item);
            }
            count++;
            m_event.post();
        }

        // TODO: [maxgolov] - method has to be refactored to allow cancellation by Task tid.
        // Otherwise we may run into a situation where we wait on a 'wrong task' completion.
        // In case if the task we were supposed to be waiting on is done running and destroyed,
        // but while we idle-wait for TASK_CANCEL_WAIT_MS - a new task obj got allocated on
        // heap with the same exact raw ptr. The chance of that collision is slim and the only
        // side-effect is we'd idle-wait slightly longer until the new task is done.
        //
        // It would also make sense to return the following cancellation status:
        // - task not found
        // - task found and cancelled
        // - task found, but not cancelled - ran to completion.
        // - task found, but not cancelled - still running.
        //
        bool Cancel(MAT::Task* item, uint64_t waitTime) override
        {
            if (item == nullptr)
            {
                return false;
            }

            if (m_itemInProgress == item)
            {
                /* Can't recursively wait on completion of our own thread */
                if (m_hThread.get_id() != std::this_thread::get_id())
                {
                    LOCKGUARD(m_lock);
                    while ((waitTime > TASK_CANCEL_WAIT_MS) && (m_itemInProgress == item))
                    {
                        PAL::sleep(TASK_CANCEL_WAIT_MS);
                        waitTime -= TASK_CANCEL_WAIT_MS;
                    }
                }
                /* Either waited long enough or the task is still executing. Return:
                 *  true    - if item in progress is different than item (other task)
                 *  false   - if item in progress is still the same (didn't wait long enough)
                 */
                return (m_itemInProgress != item);
            }

            {
                LOCKGUARD(m_lock);
                auto it = std::find(m_timerQueue.begin(), m_timerQueue.end(), item);
                if (it != m_timerQueue.end()) {
                    // Still in the queue
                    m_timerQueue.erase(it);
                    delete item;
                    return true;
                }
            }
#if 0
            for (;;) {
                {
                    LOCKGUARD(m_lock);
                    if (item->Type == MAT::Task::Done) {
                        return;
                    }
                }
                Sleep(10);
            }
#endif
            return false;
        }

    protected:
        static void threadFunc(void* lpThreadParameter)
        {
            uint64_t wakeupCount = 0;

            WorkerThread* self = reinterpret_cast<WorkerThread*>(lpThreadParameter);
            LOG_INFO("Running thread %u", std::this_thread::get_id());

            for (;;) {
                std::unique_ptr<MAT::Task> item = nullptr;
                wakeupCount++;
                unsigned nextTimerInMs = MAX_FUTURE_DELTA_MS;
                {
                    LOCKGUARD(self->m_lock);

                    auto now = getMonotonicTimeMs();
                    if (!self->m_timerQueue.empty()) {
                        const auto currTargetTime = self->m_timerQueue.front()->TargetTime;
                        if (currTargetTime <= now) {
                            // process the item at the front immediately
                            item = std::unique_ptr<MAT::Task>(self->m_timerQueue.front());
                            self->m_timerQueue.pop_front();
                        } else {
                           // timed call in future, we need to resort the items in the queue
                           const auto delta = currTargetTime - now;
                           if (delta > MAX_FUTURE_DELTA_MS) {
                               const auto itemPtr = self->m_timerQueue.front();
                               self->m_timerQueue.pop_front();
                               itemPtr->TargetTime = now + MAX_FUTURE_DELTA_MS;
                               self->Queue(itemPtr);
                               continue;
                           }
                           // value used for sleep in case if m_queue ends up being empty
                           nextTimerInMs = static_cast<unsigned>(delta);
                        }
                    }

                    if (!self->m_queue.empty() && !item) {
                        item = std::unique_ptr<MAT::Task>(self->m_queue.front());
                        self->m_queue.pop_front();
                    }
                }

                if (!item) {
                    if (!self->m_event.Reset())
                        self->m_event.wait(nextTimerInMs);
                    continue;
                }

                if (item->Type == MAT::Task::Shutdown) {
                    item.reset();
                    break;
                }
                
                LOG_TRACE("%10llu Execute item=%p type=%s\n", wakeupCount, item.get(), item.get()->TypeName.c_str() );
                self->m_itemInProgress = item.get();
                (*item)();
                self->m_itemInProgress = nullptr;

                if (item.get()) {
                    item->Type = MAT::Task::Done;
                    item.reset();
                }
            }
        }
    };

    namespace WorkerThreadFactory {
        ITaskDispatcher* Create()
        {
            return new WorkerThread();
        }
    }

} PAL_NS_END

#endif
