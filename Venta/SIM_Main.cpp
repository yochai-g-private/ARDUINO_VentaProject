#include "Venta.h"

#if YG_SIMULATOR
#include <Simulator.h>

#define SIMULATION_SCENARIOS_LIST(DO_WITH)	\
		DO_WITH(StartChol)	\
		DO_WITH(StartKodesh)	\


SIMULATION_SCENARIOS_LIST(DECLARE_SIMULATOR_SCENARIO)

static const Simulator::Scenario* st_scenarios_array[] = { SIMULATION_SCENARIOS_LIST(SIMULATOR_SCENARIO_POINTER) };
Simulator::Scenarios st_scenarios = { st_scenarios_array, countof(st_scenarios_array) };
const Simulator::Scenarios& SIM_GetScenarios()
{
	return st_scenarios;
}

#endif //YG_SIMULATOR
