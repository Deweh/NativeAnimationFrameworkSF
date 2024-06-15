#pragma once

namespace CCF
{
	//This file can be copy-pasted into other plugins to utilize the Custom Command Framework API.

	template <typename T>
	class simple_array
	{
	public:
		simple_array() {}

		simple_array(std::vector<T>& a_dataSource) {
			data = a_dataSource.data();
			count = a_dataSource.size();
		}

		uint64_t size() const {
			return count;
		}

		T& operator[](uint64_t a_idx) {
			return data[a_idx];
		}

		T& operator[](uint64_t a_idx) const {
			return data[a_idx];
		}
	private:
		T* data = nullptr;
		uint64_t count = 0;
	};

	class simple_string_view
	{
	public:
		inline simple_string_view() {}

		inline simple_string_view(const char* a_begin, uint64_t a_size) {
			data = a_begin;
			size = a_size;
		}

		inline simple_string_view(const std::string_view& a_sv) {
			from_sv(a_sv);
		}

		inline simple_string_view(const char* a_str) {
			from_sv(a_str);
		}

		inline simple_string_view(const std::string& a_str) {
			data = a_str.data();
			size = a_str.size();
		}

		inline std::string_view get() const {
			return std::string_view(data, size);
		}

		inline operator std::string_view() const {
			return get();
		}

	private:
		void from_sv(const std::string_view& a_sv) {
			data = a_sv.data();
			size = a_sv.size();
		}

		const char* data = nullptr;
		uint64_t size = 0;
	};

	class ConsoleInterface
	{
	public:
		virtual RE::NiPointer<RE::TESObjectREFR> GetSelectedReference() = 0;

		//Converts a hexadecimal string to its corresponding form (i.e. 00000014 for player, leading zeros are optional).
		//Returns nullptr if string does not point to a valid form.
		virtual RE::TESForm* HexStrToForm(const simple_string_view& a_str) = 0;

		virtual void PrintLn(const simple_string_view& a_txt) = 0;

		//This will prevent the command text from being appended to the console log.
		//i.e. usually if you enter a command like "tfc 1" into the console, the text "tfc 1" would be printed to the console.
		//Note: Must be called before the first call to PrintLn.
		virtual void PreventDefaultPrint() = 0;
	};

	typedef void (*CommandCallback)(const simple_array<simple_string_view>& a_args, const char* a_fullString, ConsoleInterface* a_intfc);

	namespace detail
	{
		typedef void (*RegCommand_Def)(const char* a_name, CCF::CommandCallback a_func);
	}

	inline bool RegisterCommand(const char* a_name, CommandCallback a_func) {
		const auto hndl = GetModuleHandleA("CustomCommandFramework.dll");
		if (hndl != NULL) {
			const auto addr = GetProcAddress(hndl, "RegisterCommand");
			if (addr != NULL) {
				(reinterpret_cast<detail::RegCommand_Def>(addr))(a_name, a_func);
				return true;
			}
		}
		return false;
	}
}