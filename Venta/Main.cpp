/*
 Name:		Venta.ino
 Created:	8/4/2019 12:16:47 PM
 Author:	Yochai.Glauber
*/

#include "Venta.h"
#include <MicroController.h>
#include <Timer.h>
#include <DaTi.h>
#include <Led.h>
#include <KodeshWatch.h>
#include <MotionDetector.h>
#include <Photoresistor.h>
#include <Transistor.h>
#include <PushButton.h>
#include <Buzzer.h>
#include <SevenSegmentDisplay.h>
#include <HumiditySensor.h>
#include <ProgMem.h>
#include <Logger.h>
#include <Tester.h>

#include <math.h>

using namespace YG;

#ifndef PROJECT_Venta
#	error Invalid AppDefines.h !!!
#endif //PROJECT_Venta

#if YG_SIMULATOR
#include <Simulator.h>
const Simulator::Scenarios& SIM_GetScenarios();
#endif //YG_SIMULATOR

//--------------------------------------------------------------------------
// DEFINEs
//--------------------------------------------------------------------------

#if __DEBUG__
#define LONG_TERM_VENTA_OFF_SECONDS		20
#define SHORT_TEMP_VENTA_OFF_SECONDS	10
#define MACHSAN_TERM_LIGHT_ON_SECONDS   10
#else
#define LONG_TERM_VENTA_OFF_SECONDS		(120 * 60)
#define SHORT_TEMP_VENTA_OFF_SECONDS    30
#define MACHSAN_TERM_LIGHT_ON_SECONDS   5
#endif //__DEBUG__

#define LED_ON_MILLIS   5
#define LED_TOGGLE_PARAMS(total_seconds)  LED_ON_MILLIS, (total_seconds*1000)-LED_ON_MILLIS

//--------------------------------------------------------------------------
// TYPEs
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// COMPONENTs
//--------------------------------------------------------------------------

enum Pins
{
	PIN_CHEDER_B_LED = 24,
	PIN_CHEDER_G_LED = 26,
	PIN_CHEDER_R_LED = 28,
	//-----------------------------------
	PIN_CHEDER_MOTION_DETECTOR = 30,
	PIN_CHEDER_LIGHT_DETECTOR = 32,
	//-----------------------------------
	PIN_LIGHT_RELAY = 34,
	PIN_VENTA_RELAY = 36,
	PIN_MACHSAN_MOTION_DETECTOR = 38,
	//-----------------------------------
	PIN_SENSORS_TEST_BUTTON = 42,
	PIN_SENSORS_DIP_SWITCH = 44,
	PIN_SENSORS_SWITCH_TRANSISTOR = 46,
	//-----------------------------------
	PIN_DISPLAY_DIO = 25,
	PIN_DISPLAY_CLK = 27,
	//-----------------------------------
	PIN_ACTIVE_BUZZER = 2,
	PIN_DHT_SENSOR    = 3,
};

#define TRACE_GLOBALS_INITIALIZATION	0
#if !TRACE_GLOBALS_INITIALIZATION
	#undef TRCBEGIN
	#undef TRCLOC
	#define TRCBEGIN
	#define TRCLOC(id)
#endif //!TRACE_GLOBALS_INITIALIZATION 

TRCBEGIN;
TRCLOC(1);

KodeshWatch				kodesh_watch(PreciseRTC::GetInstance());
LightDetector           light_detector("LIGHT_DETECTOR", PIN_CHEDER_LIGHT_DETECTOR);
MotionDetector          cheder_motion_detector("CHEDER_MOTION_SENSOR", PIN_CHEDER_MOTION_DETECTOR);
Led		                RED("R", PIN_CHEDER_R_LED);
Led                     GREEN("G", PIN_CHEDER_G_LED);
Led                     BLUE("B", PIN_CHEDER_B_LED);

TRCLOC(2);
NPN_Transistor			sensors("SENSORS", PIN_SENSORS_SWITCH_TRANSISTOR, false);


MotionDetector          machsan_motion_detector("MACHSAN_MOTION_SENSOR", PIN_MACHSAN_MOTION_DETECTOR);
Relay					venta("VENTA_RELAY", PIN_VENTA_RELAY, false, true);
Relay					light("LIGHT_RELAY", PIN_LIGHT_RELAY, false, true);

//	PreciseRTC uses		20 = SDA
//						21 = SCL

Led&					VENTA_ON_LED = GREEN;
Led&					MOTION_DETECTED_LED = RED;
Led&					LIGHT_DETECTED_LED = BLUE;

TRCLOC(3);
DualTimedButton			button("TEST_BUTTON", PIN_SENSORS_TEST_BUTTON);
DipSwitch				daily_saving_time("DST_SWITCH", PIN_SENSORS_DIP_SWITCH);

TRCLOC(4);
SevenSegmentDisplay		display("DISPLAY", PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);

TRCLOC(5);
ActiveBuzzer			buzzer("BUZZER", PIN_ACTIVE_BUZZER);
HumiditySensor			humidity_sensor("HUMIDITY_SENSOR", PIN_DHT_SENSOR);

TRCLOC(6);
State                   gbl_state = { 0 };
bool					gbl_active = false;
//bool					testing = false;

static bool				sensors_on = false;
static bool				venta_on = false;
static bool				light_on = false;

static uint8_t			state_n = 0xFF;

//--------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------

void reset_leds()
{
	if (gbl_state.s.sleep)
		return;

	RED.Off();
	GREEN.Off();
	BLUE.Off();
}
//--------------------------------------------------------------------------
void set_RGB()
{
	if (!gbl_active)
		return;

#define SHOW_STATE	1
#if SHOW_STATE
#define _SHOW_STATE_FLAG(id, fld)		_TRC(F("  " #id ": ") << fld << NL)
#define SHOW_STATE_FLAG(id)				_SHOW_STATE_FLAG(id, gbl_state.s.id)

	_TRC(F("State:") << NL);

	SHOW_STATE_FLAG(cheder_motion);
	SHOW_STATE_FLAG(light);
	SHOW_STATE_FLAG(machsan_motion);
	SHOW_STATE_FLAG(kodesh);
	SHOW_STATE_FLAG(night);
	SHOW_STATE_FLAG(sleep);
	_SHOW_STATE_FLAG(summertime, daily_saving_time.IsOn());
#endif

	if (gbl_state.s.kodesh || gbl_state.s.night)
	{
		reset_leds();
		return;
	}

	// VENTA_ON_LED
	if (gbl_state.n == 0)
	{
		reset_leds();
		VENTA_ON_LED.Toggle2(LED_TOGGLE_PARAMS(2));
		return;
	}
	else
	{
		VENTA_ON_LED.Off();
	}

	// LIGHT_DETECTED_LED
	if (gbl_state.s.light)
	{
		if (!LIGHT_DETECTED_LED.IsToggling())	LIGHT_DETECTED_LED.Toggle2(LED_TOGGLE_PARAMS(5));
	}
	else
	{
		LIGHT_DETECTED_LED.Off();
	}

	// MOTION_DETECTED_LED
	if (gbl_state.s.cheder_motion || gbl_state.s.machsan_motion)
	{
		if (!MOTION_DETECTED_LED.IsToggling())	MOTION_DETECTED_LED.Toggle2(LED_TOGGLE_PARAMS(5));
	}
	else
	{
		MOTION_DETECTED_LED.Off();
	}
}
//--------------------------------------------------------------------------
void activate(Timer& timer, void* context)
{
	_TRC(F("Activated!") << NL);

	gbl_active = true;

	set_RGB();
}
//--------------------------------------------------------------------------
void delay_activation()
{
	gbl_active = false;

	reset_leds();

	static Timer timer;

	_TRC(F("Activation delayed") << NL);

	timer.RestartMilli(ACTIVATION_DELAY_SECONDS * 1000,
					   activate,
					   NULL,
					   1);
}
//--------------------------------------------------------------------------
void set_sensors(bool on)
{
	sensors.Set(on);

	sensors_on = on;

	reset_leds();

	if (on)
	{
		delay_activation();
	}
	else
	{
		venta_on = light_on = false;

		venta.Set(venta_on);
		light.Set(light_on);
	}
}
//--------------------------------------------------------------------------
static void OnKodeshChanged(KodeshWatch& kw)
{
	bool kodesh = kodesh_watch.IsKodesh();

	if (gbl_state.s.kodesh == kodesh)
		return;

	_TRC(F("Kodesh=") << kodesh << NL);

	gbl_state.s.kodesh = kodesh;

	buzzer.Toggle(400, true, 3);
}
//--------------------------------------------------------------------------
class ChederMotionObserver : public StableDigitalInputComponentObserver
{
public:

	ChederMotionObserver() : StableDigitalInputComponentObserver(cheder_motion_detector, "ChederMotionSensor", SHORT_TEMP_VENTA_OFF_SECONDS)
	{
	}

private:

	void set(bool on)
	{
		_TRC("ChederMotionObserver: " << on << NL);
		gbl_state.s.cheder_motion = on;
	}
} cheder_motion_observer;
//--------------------------------------------------------------------------
class MachsanMotionObserver : public StableDigitalInputComponentObserver
{
public:

	MachsanMotionObserver() : StableDigitalInputComponentObserver(machsan_motion_detector, "MachsanMotionSensor", MACHSAN_TERM_LIGHT_ON_SECONDS) {	}

private:

	void set(bool on)
	{
		_TRC("MachsanMotionObserver: " << on << NL);
		gbl_state.s.machsan_motion = on;
	}
} machsan_motion_observer;
//--------------------------------------------------------------------------
class LightObserver : public StableDigitalInputComponentObserver
{
public:

	LightObserver() : StableDigitalInputComponentObserver(light_detector, "LightSensor", SHORT_TEMP_VENTA_OFF_SECONDS),
		sleep_state(NO_SLEEP)
	{
	}

private:

	enum SleepState
	{
		NO_SLEEP,
		FIRST_ON,
		FIRST_OFF,
		SECOND_ON,
		SLEEP
	};

	void set(bool on)
	{
		_TRC("LightObserver: " << on << NL);
		gbl_state.s.light = on;
	}

	void on_change(bool on)
	{

		sleep_timer.Stop();

#define _SET_SLEEP_STATE(me, state)	me->sleep_state = state; _TRC(F("Sleep state=" #state) << NL); gbl_state.s.sleep = (state == SLEEP)
#define SET_SLEEP_STATE(state)	_SET_SLEEP_STATE(this, state)
#define SLEEP_TIMEOUT_MILLIS	4000

		if (on)
		{
			switch (sleep_state)
			{
			case NO_SLEEP:
			{

				SET_SLEEP_STATE(FIRST_ON);
				sleep_timer.StartMilli(SLEEP_TIMEOUT_MILLIS, on_sleep_timer, this, 1);
				break;
			}

			case FIRST_OFF:
			{
				SET_SLEEP_STATE(SECOND_ON);
				sleep_timer.StartMilli(SLEEP_TIMEOUT_MILLIS, on_sleep_timer, this, 1);
				break;
			}

			case SLEEP:
			{
				SET_SLEEP_STATE(NO_SLEEP);
				break;
			}
			}
		}
		else
		{
			switch (sleep_state)
			{
			case FIRST_ON:
			{
				SET_SLEEP_STATE(FIRST_OFF);
				sleep_timer.StartMilli(SLEEP_TIMEOUT_MILLIS, on_sleep_timer, this, 1);
				break;
			}

			case SECOND_ON:
			{
				reset_leds();

				SET_SLEEP_STATE(SLEEP);
				sleep_timer.StartMilli(2 * 3600 * 1000, on_sleep_timer, this, 1);

				RED.Toggle(1000, true, 5);
				GREEN.Toggle(1000, true, 5);
				BLUE.Toggle(1000, true, 5);

				break;
			}
			}
		}
	}

	static void on_sleep_timer(Timer& timer, void* context)
	{
		LightObserver* This = (LightObserver*)context;
		_SET_SLEEP_STATE(This, NO_SLEEP);

		gbl_state.s.sleep = false;
	}

	Timer			sleep_timer;
	SleepState		sleep_state;

} light_observer;
//--------------------------------------------------------------------------
void ClockCallback(const DaTi& dt, IRTC::TickType tick_type, void* ctx)
{
	int summertime = daily_saving_time.IsOn();
	hour_t hour = (dt.Hour() + summertime) % DaTi::HOURS_PER_DAY;

	gbl_state.s.night = (hour >= 23) ||
		(hour < 8);

	if (!(gbl_state.s.kodesh || gbl_state.s.night || gbl_state.s.sleep) &&
		dt.Minute() == 0 &&
		dt.Second() == 0)
	{
		buzzer.ToggleOnce(200);
	}

	if (gbl_state.s.light			||
		gbl_state.s.cheder_motion	||
		gbl_state.s.machsan_motion)
	{
		enum Show { TIME_1 = 0, CLEAR_1, TEMPERATURE, CLEAR_2, TIME_2, CLEAR_3, HUMIDITY, CLEAR_4, __last_show__ };

		static Show show = TIME_1;

		char text[5];
		
		switch (show)
		{
			case TIME_1:
			case TIME_2:
			{
				display.PrintTime(hour, dt.Minute(), false);
				break;
			}

			case TEMPERATURE:
			{

				float ft = PreciseRTC::GetInstance().GetTemperature();
				dtostrf(ft, 4, 1, text);

				display.PrintText(text);
				break;
			}

			case HUMIDITY:
			{
				HumiditySensor::IoResult io_result = humidity_sensor.GetIoResult();

				if (HumiditySensor::DHT_OK == io_result)
				{
					int i = (int)humidity_sensor.GetHumidity();

					sprintf(text, "h %d", i);
				}
				else
				{
					strcpy(text, "HERR");
				}

				display.PrintText(text);
				break;
			}

			default:
			{
				display.Clear();
				break;
			}
		}

		int int_show = show;
		int_show = (int_show + 1) % __last_show__;
		show = (Show)int_show;
	}
	else
	{
		display.Clear();
	}
}
///--------------------------------------------------------------------------
InputComponentTest	light_detector_test			(light_detector,			light_observer,				&sensors),
					cheder_motion_detector_test	(cheder_motion_detector,	cheder_motion_observer,		&sensors),
					machsan_motion_detector_test(machsan_motion_detector,	machsan_motion_observer,	&sensors);
//--------------------------------------------------------------------------
DigitalOutputComponentTest	RED_test	(RED,		("RED"),		&sensors),
							GREEN_test	(GREEN,		("GREEN"),		&sensors),
							BLUE_test	(BLUE,		("BLUE"),		&sensors),
							SENSORS_test(sensors,	("SENSORS"),	&sensors),
							VENTA_test	(venta,		("VENTA"),		&sensors),
							LIGHT_test	(light,		("LIGHT"),		&sensors);
//--------------------------------------------------------------------------
static ITestTarget* tests[] =
{
	&BeginTests::sm_instance,
	&light_detector_test,
	&cheder_motion_detector_test,
	&machsan_motion_detector_test,
	&RED_test,
	&GREEN_test,
	&BLUE_test,
	&SENSORS_test,
	&VENTA_test,
	&LIGHT_test,
	&EndTests::sm_instance,
	NULL
};

Tester button_observer(button, tests);
//--------------------------------------------------------------------------
class Application : public IApplication
{
public:

	const char*		GetApplicationName()	const { return "Venta"; }
	Version			GetApplicationVersion()	const { return 5; }

	void OnSetup()
	{
		//#define SETUP_TRACE()	TRACE_INFO_LOCATION()
		#define SETUP_TRACE()	

		SETUP_TRACE();
		MicroController::BeginSerial();

		SETUP_TRACE();
		if (!kodesh_watch.Begin())
		{
			TRACE_ERROR("Failed to start kodesh_watch");
			return;
		}

		SETUP_TRACE();
		__TRC(
			IRTC& clk = kodesh_watch.GetRTC();
			clk.ShowAll());

		SETUP_TRACE();
		set_sensors(false);

		SETUP_TRACE();
		venta.Off();
		light.Off();

		SETUP_TRACE();
		gbl_state.n = 0;

#define SET_FLAG(id)	gbl_state.s.id = 1
		SET_FLAG(kodesh);

//		kodesh_watch.Register(kodesh_observer);
		kodesh_watch.SetCallback(OnKodeshChanged);
		kodesh_watch.SetClockCallback(ClockCallback, NULL);
		light_detector.Register(light_observer);
		cheder_motion_detector.Register(cheder_motion_observer);
		machsan_motion_detector.Register(machsan_motion_observer);

		SETUP_TRACE();
		button.Register(button_observer);

//		_TRC(clk.ShowAll());

		SETUP_TRACE();
		if (!kodesh_watch.Begin())
		{
			SETUP_TRACE();
			return;
		}

		set_sensors(true);

		display.Begin();

		humidity_sensor.Start(15);

		buzzer.Toggle(500, true, 5);

		_TRC(F("=========== STARTED ============") << NL);
	}

	void OnLoop(bool first_loop)
	{
		if (!gbl_active)
			return;

		bool now_sensors_on = (gbl_state.s.kodesh == false);
		bool now_venta_on = (gbl_state.n == 0);
		bool now_light_on = gbl_state.s.machsan_motion;

		#define CHECK_CHANGE(id, name, statements)	\
			do { if (id != now_##id)	{ id = now_##id; \
										  _TRC(name << F(": ") << (id ? F("On") : F("Off")) << NL);\
										  statements; } } while(0)

		CHECK_CHANGE(sensors_on, F("Sensors"), set_sensors(sensors_on));

		if (sensors_on)
		{
			CHECK_CHANGE(venta_on, F("Venta"), venta.Set(venta_on));
			CHECK_CHANGE(light_on, F("Light"), light.Set(light_on));
		}

		if (state_n != gbl_state.n)
		{
			state_n = gbl_state.n;
			set_RGB();
		}
	}

	void OnError()
	{
	}

#if YG_USE_LOGGER && !YG_SIMULATOR
	void OnSerialAvailable()
	{
		kodesh_watch.GetRTC().UpdateFromSerial();
	}
#endif //YG_USE_LOGGER && !YG_SIMULATOR

#if YG_SIMULATOR
	const Simulator::Scenarios& GetScenarios()	const
	{
		return SIM_GetScenarios();
	}
#endif //YG_SIMULATOR
} application;
//--------------------------------------------------------------------------
IApplication& YG::GetApplication() { return application; }
//--------------------------------------------------------------------------
#if YG_SIMULATOR
bool SensorsAreActive()
{
	return sensors.IsOn();
}
#endif //YG_SIMULATOR
//--------------------------------------------------------------------------

