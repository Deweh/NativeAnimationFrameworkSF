#include "VM.h"

namespace Util::VM
{
	namespace detail
	{
		FunctionCallback::FunctionCallback(const std::function<void(RE::BSScript::Variable&)>& a_func) { _impl = a_func; }
		void FunctionCallback::CallQueued() {}
		void FunctionCallback::CallCanceled() {}
		void FunctionCallback::StartMultiDispatch() {}
		void FunctionCallback::EndMultiDispatch() {}
		void FunctionCallback::operator()(RE::BSScript::Variable a_res) { _impl(a_res); }
		bool FunctionCallback::CanSave() { return false; }
	}
}