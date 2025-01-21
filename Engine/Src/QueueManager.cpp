#include "../Include/Queue/QueueManager.h"

QueueManager::QueueManager(std::function<void(const QueueEvent&)> eventHandler)
	: stopQueueManager_(false)
	, eventHandler_(std::move(eventHandler))
{
	// Wait for worker thread handling events to start
	
	std::promise<void> readyPromise;
	std::future<void> readyFuture = readyPromise.get_future();

	workerThread_ = std::thread([this, promise = std::move(readyPromise)]() mutable
	{
		promise.set_value();
		HandleEvents();
	});

	readyFuture.wait();
}

QueueManager::~QueueManager()
{
	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		stopQueueManager_ = true;
	}

	condition_.notify_all();
	if (workerThread_.joinable()) workerThread_.join();
}

void QueueManager::EnqueueEvent(const QueueEvent& event)
{
	{
		std::scoped_lock<std::mutex> lock(queueMutex_);
		eventQueue_.push(event);
	}
	condition_.notify_one();
}

void QueueManager::WaitForAllEvents() const
{
	std::unique_lock<std::mutex> lock(queueMutex_);
	condition_.wait(lock, [this]() { return eventQueue_.empty(); });
}

void QueueManager::HandleEvents()
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

		// Event is handled by the OrderBook

		eventHandler_(event);

		// Notify waiting threads if queue is empty

		{
			std::lock_guard<std::mutex> lock(queueMutex_);
			if (eventQueue_.empty())
				condition_.notify_all();
		}
	}
}