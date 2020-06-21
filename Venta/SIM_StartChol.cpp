#include "Venta.h"

#if YG_SIMULATOR
#include <Simulator.h>

using namespace YG;

static void onSetup()
{
	SetCurrentTime(DaTi(2019, Nov, 1, 15, 15, 0));
}
//------------------------------------------------
static Simulator::StepResult onLoop(int case_id)
{
	ShowCurrentTime();

	switch (case_id)
	{
		case 0:
		{
			YATF_CHECK_EQUAL(gbl_active, false, nothing);
			return Simulator::ResultTimerMillis(ACTIVATION_DELAY_SECONDS * 1200);
		}

		case 1:
		{
			YATF_CHECK_EQUAL(gbl_active,					true,  nothing);
			YATF_CHECK_EQUAL(gbl_state.s.cheder_motion,		false, nothing);
			YATF_CHECK_EQUAL(gbl_state.s.light,				false, nothing);
			YATF_CHECK_EQUAL(gbl_state.s.machsan_motion,	false, nothing);
			YATF_CHECK_EQUAL(gbl_state.s.kodesh,			false, nothing);
			YATF_CHECK_EQUAL(gbl_state.s.night,				false, nothing);
			YATF_CHECK_EQUAL(gbl_state.s.sleep,				false, nothing);

			YATF_CHECK_EQUAL(SensorsAreActive(),			true,  nothing);
			
			return Simulator::ResultEnd();
		}
	}

	return Simulator::ResultEnd();
}
//------------------------------------------------
IMPLEMENT_SIMULATOR_SCENARIO(StartChol);

#endif //YG_SIMULATOR
