#include <Dx12/CommandQueue.hpp>
#include <Helper\ThrowAssert.hpp>

using namespace KGL;

CommandQueue::CommandQueue(ComPtrC<ID3D12CommandQueue> queue,
	ComPtrC<ID3D12Fence> fence) noexcept
{
	assert(queue);
	m_queue = queue;
	m_fence = fence;
	m_value = m_fence->GetCompletedValue();
}

CommandQueue::CommandQueue(ComPtrC<ID3D12Device> device,
	const D3D12_COMMAND_QUEUE_DESC& desc) noexcept
	: m_value(0ULL)
{
	assert(device);
	HRESULT hr = S_OK;
	hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_queue.GetAddressOf()));
	RCHECK(FAILED(hr), "コマンドキューの生成に失敗！");
	hr = device->CreateFence(m_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
	RCHECK(FAILED(hr), "フェンスの生成に失敗！");
}

HRESULT CommandQueue::Signal() noexcept
{
	return m_queue->Signal(m_fence.Get(), ++m_value);
}

HRESULT CommandQueue::Wait(UINT64 value) const noexcept
{
	auto event = CreateEvent(nullptr, false, false, nullptr);
	HRESULT hr = m_fence->SetEventOnCompletion(value, event);
	RCHECK(FAILED(hr), "SetEventOnCompletionに失敗", hr);

	// イベントが派生するまで待ち続ける (INFINITE)
	WaitForSingleObject(event, INFINITE);

	// イベントハンドルを閉じる
	CloseHandle(event);
	return hr;
}