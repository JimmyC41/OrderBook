#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <iostream>
#include <future>

#include "QueueEvent.h"

class QueueManager
{
public:
	explicit QueueManager(std::function<void(const QueueEvent&)> eventHandler)
		: stopQueueManager_(false)
		, eventHandler_(std::move(eventHandler))
	{
		std::promise<void> readyPromise;
		std::future<void> readyFuture = readyPromise.get_future();

		workerThread_ = std::thread([this, promise = std::move(readyPromise)]() mutable
		{
			promise.set_value();
			HandleEvents();
		});

		readyFuture.wait();
	}

	~QueueManager()
	{
		{
			std::lock_guard<std::mutex> lock(queueMutex_);
			stopQueueManager_ = true;
		}

		condition_.notify_all();
		if (workerThread_.joinable()) workerThread_.join();
	}

	QueueManager(const QueueManager&) = delete;
	QueueManager(QueueManager&&) = delete;
	QueueManager& operator=(const QueueManager&) = delete;
	QueueManager& operator=(QueueManager&&) = delete;

	void EnqueueEvent(const QueueEvent& event)
	{
		{
			std::scoped_lock<std::mutex> lock(queueMutex_);
			eventQueue_.push(event);
		}
		condition_.notify_one();
	}

	void WaitForAllEvents() const
	{
		std::unique_lock<std::mutex> lock(queueMutex_);
		condition_.wait(lock, [this]() { return eventQueue_.empty(); });
	}

private:
	std::queue<QueueEvent> eventQueue_;
	mutable std::mutex queueMutex_;
	mutable std::condition_variable condition_;
	std::thread workerThread_;
	bool stopQueueManager_;
	std::function<void(const QueueEvent&)> eventHandler_;

	void HandleEvents()
	{
		while (true)
		{
			// Fetch the next event from the queue

			std::unique_lock<std::mutex> lock(queueMutex_);
			condition_.wait(lock, [this]() { return stopQueueManager_ || !eventQueue_.empty(); });

			if (stopQueueManager_ && eventQueue_.empty())
				break;

			QueueEvent event = std::move(eventQueue_.front());
			eventQueue_.pop();
			lock.unlock();

			eventHandler_(event);

			// Notify waiting threads if queue is empty

			{
				std::lock_guard<std::mutex> lock(queueMutex_);
				if (eventQueue_.empty())
					condition_.notify_all();
			}
		}
	}
};