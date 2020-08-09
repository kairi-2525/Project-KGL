#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <memory>
#include <mutex>

namespace KGL
{
	inline namespace HELPER
	{
		template <class _Ty>
		class SceneBase
		{
		private:
			bool								m_move_allow;
			bool								m_loaded;
			std::mutex							m_loaded_mutex;
			bool								m_load_started;
			std::shared_ptr<SceneBase<_Ty>>		m_next_scene;
		private:
			HRESULT BaseLoad(_Ty desc) noexcept
			{
				HRESULT hr = Load(desc);
				std::lock_guard<std::mutex> lock(m_loaded_mutex);
				m_loaded = true;
				return hr;
			}
		public:
			explicit SceneBase() noexcept : m_loaded(false), m_load_started(false), m_move_allow(true) {}
			virtual ~SceneBase() = default;
			HRESULT virtual Load(const _Ty& desc) = 0;
			HRESULT virtual Init(const _Ty& desc) { return S_OK; }
			HRESULT virtual Update(const _Ty& desc, float elapsed_time) = 0;
			HRESULT virtual UnInit(const _Ty& desc, std::shared_ptr<SceneBase<_Ty>> next_scene) { return S_OK; }

			bool IsLoaded() noexcept { std::lock_guard<std::mutex> lock(m_loaded_mutex); return m_loaded; };
			bool IsLoadStarted() const noexcept { return m_load_started; }
			template<class _Scene>
			void SetNextScene(const _Ty& desc, bool single_thread = false) noexcept
			{
				if (m_next_scene) return;
				m_next_scene = std::make_shared<_Scene>();
				if (!single_thread)
				{
					m_next_scene->m_load_started = true;
					UnInit(desc, m_next_scene);
					std::thread th(&SceneBase<_Ty>::BaseLoad, m_next_scene.get(), desc);
					th.detach();
				}
			}
			void SetMoveSceneFlg(bool allow) noexcept { m_move_allow = allow; };
			bool IsAllowMoveScene() const noexcept { return m_move_allow; }
			const std::shared_ptr<SceneBase>& GetNextScene() noexcept { return m_next_scene; }
		};

		template <class _Ty>
		class SceneManager
		{
		private:
			std::shared_ptr<SceneBase<_Ty>> m_scene;
		public:
			template<class _Scene>
			HRESULT Init(const _Ty& desc)
			{
				m_scene = std::make_shared<_Scene>();
				auto hr = m_scene->Load(desc);
				if (FAILED(hr)) return hr;
				return m_scene->Init(desc);
			}
			HRESULT UnInit(const _Ty& desc, std::shared_ptr<SceneBase<_Ty>> next_scene)
			{
				if (m_scene) return m_scene->UnInit(desc, next_scene);
				return S_OK;
			}
			HRESULT Update(const _Ty& desc, float elapsed_time)
			{
				return m_scene->Update(desc, elapsed_time);
			}
			HRESULT SceneChangeUpdate(const _Ty& desc)
			{
				HRESULT hr = S_OK;
				auto next_scene = m_scene->GetNextScene();
				if (next_scene)
				{
					if (!next_scene->IsLoadStarted())
					{
						m_scene->UnInit(desc, next_scene);
						hr = next_scene->Load(desc);
						if (FAILED(hr)) return hr;
					}
					else if (!next_scene->IsLoaded() || !m_scene->IsAllowMoveScene())
						return hr;

					desc.app->GetQueue()->Signal();
					desc.app->GetQueue()->Wait();

					m_scene.reset();
					m_scene = next_scene;
					m_scene->Init(desc);
				}
				return hr;
			}
		};
	}
}