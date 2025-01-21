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

	explicit QueueManager(std::function<void(const QueueEvent&)> eventHandler);
	~QueueManager();

	QueueManager(const QueueManager&) = delete;
	QueueManager(QueueManager&&) = delete;
	QueueManager& operator=(const QueueManager&) = delete;
	QueueManager& operator=(QueueManager&&) = delete;

	// Enqueues an order request to the queue
	void EnqueueEvent(const QueueEvent& event);

	// Blocks until all events in the queue have been processed
	void WaitForAllEvents() const;

private:

	std::queue<QueueEvent> eventQueue_;
	mutable std::mutex queueMutex_;
	mutable std::condition_variable condition_;
	std::thread workerThread_;
	bool stopQueueManager_;

	// Callback function provided by OrderBook to process QueueEvent objects
	std::function<void(const QueueEvent&)> eventHandler_;

	// Loop for worker threads to fetch and process QueueEvent objects
	void HandleEvents();
};