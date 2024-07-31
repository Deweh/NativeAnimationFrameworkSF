#pragma once
#include "Util/General.h"

namespace Tasks
{
	class Input
	{
	public:
		enum BS_BUTTON_CODE : uint32_t
		{
			kBackspace = 0x08,
			kTab = 0x09,
			kEnter = 0x0D,
			kCapsLock = 0x14,
			kEscape = 0x1B,
			kSpace = 0x20,
			kPageUp = 0x21,
			kPageDown = 0x22,
			kEnd = 0x23,
			kHome = 0x24,
			kLeft = 0x25,
			kUp = 0x26,
			kRight = 0x27,
			kDown = 0x28,
			kInsert = 0x2D,
			kDelete = 0x2E,
			k0 = 0x30,
			k1 = 0x31,
			k2 = 0x32,
			k3 = 0x33,
			k4 = 0x34,
			k5 = 0x35,
			k6 = 0x36,
			k7 = 0x37,
			k8 = 0x38,
			k9 = 0x39,
			kA = 0x41,
			kB = 0x42,
			kC = 0x43,
			kD = 0x44,
			kE = 0x45,
			kF = 0x46,
			kG = 0x47,
			kH = 0x48,
			kI = 0x49,
			kJ = 0x4A,
			kK = 0x4B,
			kL = 0x4C,
			kM = 0x4D,
			kN = 0x4E,
			kO = 0x4F,
			kP = 0x50,
			kQ = 0x51,
			kR = 0x52,
			kS = 0x53,
			kT = 0x54,
			kU = 0x55,
			kV = 0x56,
			kW = 0x57,
			kX = 0x58,
			kY = 0x59,
			kZ = 0x5A,
			kNumpad_0 = 0x60,
			kNumpad_1 = 0x61,
			kNumpad_2 = 0x62,
			kNumpad_3 = 0x63,
			kNumpad_4 = 0x64,
			kNumpad_5 = 0x65,
			kNumpad_6 = 0x66,
			kNumpad_7 = 0x67,
			kNumpad_8 = 0x68,
			kNumpad_9 = 0x69,
			kNumpad_Multiply = 0x6A,
			kNumpad_Plus = 0x6B,
			kNumpad_Minus = 0x6D,
			kNumpad_Period = 0x6E,
			kNumpad_Divide = 0x6F,
			kF1 = 0x70,
			kF2 = 0x71,
			kF3 = 0x72,
			kF4 = 0x73,
			kF5 = 0x74,
			kF6 = 0x75,
			kF7 = 0x76,
			kF8 = 0x77,
			kF9 = 0x78,
			kF10 = 0x79,
			kF11 = 0x7A,
			kF12 = 0x7B,
			kSemicolon = 0xBA,
			kComma = 0xBC,
			kEquals = 0xBB,
			kMinus = 0xBD,
			kPeriod = 0xBE,
			kDivide = 0xBF,
			kLBracket = 0xDB,
			kBackslash = 0xDC,
			kRBracket = 0xDD,
			kApostrophe = 0xDE,
			kLShift = 0xA0,
			kRShift = 0xA1,
			kLControl = 0xA2,
			kRControl = 0xA3,
			kLAlt = 0xA4,
			kRAlt = 0xA5,

			kGamepad = 0x10000,
			kDPAD_Up = 0x10001,
			kDPAD_Down = 0x10002,
			kDPAD_Left = 0x10004,
			kDPAD_Right = 0x10008,
			kLTrigger = 0x10009,
			kRTrigger = 0x1000A,
			kSelect = 0x10020,
			kLStick = 0x10040,
			kRStick = 0x10080,
			kLShoulder = 0x10100,
			kRShoulder = 0x10200,
			kAButton = 0x11000,
			kBButton = 0x12000,
			kXButton = 0x14000,
			kYButton = 0x18000,
		};

		using ButtonCallback = std::function<void(BS_BUTTON_CODE a_key, bool a_down)>;

		static Input* GetSingleton();
		void RegisterForKey(BS_BUTTON_CODE a_key, ButtonCallback a_callback);

		std::map<uint32_t, ButtonCallback> callbacks;
	};
}