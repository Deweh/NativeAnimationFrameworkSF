#pragma once

namespace Util::VM
{
	namespace detail
	{
		class FunctionCallback : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			FunctionCallback(const std::function<void(RE::BSScript::Variable&)>& a_func);

			virtual void CallQueued();
			virtual void CallCanceled();
			virtual void StartMultiDispatch();
			virtual void EndMultiDispatch();
			virtual void operator()(RE::BSScript::Variable a_res);
			virtual bool CanSave();

		private:
			std::function<void(RE::BSScript::Variable&)> _impl;
		};

		template <class... Args>
		RE::BSScrapArray<RE::BSScript::Variable> PackVariables(Args&&... a_args)
		{
			constexpr auto size = sizeof...(a_args);
			auto args = std::make_tuple(std::forward<Args>(a_args)...);
			RE::BSScrapArray<RE::BSScript::Variable> result{ size };
			[&]<std::size_t... p>(std::index_sequence<p...>) {
				((result[p] = std::get<p>(args)), ...);
			}(std::make_index_sequence<size>{});
			return result;
		}
	}

	template <class... Args>
	void CallFunction(void* a_obj, const std::string_view a_scriptName, const std::string a_funcName, const std::function<void(RE::BSScript::Variable&)>& a_resultCallback, Args&&... a_args)
	{
		auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		uint32_t typeId;

		if (!vm->GetTypeIDForScriptObject(a_scriptName, typeId)) {
			return;
		}

		size_t hndl = vm->GetObjectHandlePolicy().GetHandleForObject(typeId, a_obj);
		if (!hndl) {
			return;
		}

		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback(nullptr);
		if (a_resultCallback) {
			callback.reset(new detail::FunctionCallback(a_resultCallback));
		}

		auto argArray = detail::PackVariables(a_args...);

		std::function<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> argFunctor([&argArray](RE::BSScrapArray<RE::BSScript::Variable>& a_out) {
			a_out.reserve(argArray.size());
			for (const auto& v : argArray) {
				a_out.emplace_back(v);
			}
			return true;
		});

		vm->DispatchMethodCall(hndl, a_scriptName, a_funcName, argFunctor, callback, 0);
	}
}