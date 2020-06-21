#ifndef __Venta__
#define __Venta__

#include <YG.h>
#include <Logger.h>
#include <DaTi.h>
#include <IRTC.h>
#include <PreciseRTC.h>

using namespace YG;

typedef union State
{
	struct
	{
		bool cheder_motion	: 1;
		bool light			: 1;
		bool machsan_motion : 1;
		bool kodesh			: 1;
		bool night			: 1;
		bool sleep			: 1;
	} s;

	uint8_t n;
} State;

enum VentaConstants
{
#if YG_NO_SIMULATOR
	ACTIVATION_DELAY_SECONDS = 5,
#else
	ACTIVATION_DELAY_SECONDS = 1,
#endif //YG_NO_SIMULATOR
};

#if YG_SIMULATOR
bool SensorsAreActive();
#endif //YG_SIMULATOR

extern State					gbl_state;
extern bool						gbl_active;


#endif //__Venta__
