#include "survive.h"
#include "survive_kalman_tracker.h"
#include "survive_recording.h"

#define TIMECENTER_TICKS (48000000 / 240) // for now.

void survive_default_light_pulse_process(SurviveObject *so, int sensor_id, int acode, survive_timecode timecode,
										 FLT length, uint32_t lh) {}

void survive_ootx_behavior(SurviveObject *so, int8_t bsd_idx, int8_t lh_version, int ootx);

void survive_default_light_process(SurviveObject *so, int sensor_id, int acode, int timeinsweep, uint32_t timecode,
								   uint32_t length, uint32_t lh) {
	lh = survive_get_bsd_idx(so->ctx, lh);

	survive_notify_gen1(so, "Lightcap called");

	SurviveContext *ctx = so->ctx;
	int base_station = lh;

	if (sensor_id == -1 || sensor_id == -2) {
		uint8_t dbit = (acode & 2) >> 1;
		survive_ootx_behavior(so, lh, ctx->lh_version, dbit);
	}

	survive_recording_light_process(so, sensor_id, acode, timeinsweep, timecode, length, lh);

	FLT length_sec = length / (FLT)so->timebase_hz;

	if (sensor_id <= -1) {
		PoserDataLightGen1 l = {
			.common =
				{
					.hdr =
						{
							.pt = POSERDATA_SYNC,
							.timecode = SurviveSensorActivations_long_timecode_light(&so->activations, timecode),
						},
					.sensor_id = sensor_id,
					.angle = 0,
					.lh = lh,
				},
			.acode = acode,
			.length = length,
		};

		SURVIVE_POSER_INVOKE(so, &l);

		ctx->light_pulseproc(so, sensor_id, acode, timecode, length_sec, lh);

		return;
	}

	if (base_station > NUM_GEN1_LIGHTHOUSES)
		return;

	// No loner need sync information past this point.
	if (sensor_id < 0)
		return;

	if (timeinsweep > 2 * TIMECENTER_TICKS) {
		SV_WARN("Disambiguator gave invalid timeinsweep %s %u", so->codename, timeinsweep);
		return;
	}

	int centered_timeinsweep = (timeinsweep - TIMECENTER_TICKS);
	FLT angle = centered_timeinsweep * (1. / TIMECENTER_TICKS * 3.14159265359 / 2.0);
	assert(angle >= -LINMATHPI && angle <= LINMATHPI);

	ctx->angleproc(so, sensor_id, acode, timecode, length_sec, angle, lh);
}

void survive_default_lightcap_process(SurviveObject *so, const LightcapElement *le) {
	survive_notify_gen1(so, "Lightcap called");
}

void survive_default_angle_process(SurviveObject *so, int sensor_id, int acode, uint32_t timecode, FLT length,
								   FLT angle, uint32_t lh) {
	survive_notify_gen1(so, "Default angle called");
	SurviveContext *ctx = so->ctx;

	PoserDataLightGen1 l = {
		.common =
			{
				.hdr =
					{
						.pt = POSERDATA_LIGHT,
						.timecode = SurviveSensorActivations_long_timecode_light(&so->activations, timecode),
					},
				.sensor_id = sensor_id,
				.angle = angle,
				.lh = lh,
			},
		.acode = acode,
		.length = length,
	};

	// Simulate the use of only one lighthouse in playback mode.
	if (lh < ctx->activeLighthouses)
		SurviveSensorActivations_add(&so->activations, &l);

	survive_recording_angle_process(so, sensor_id, acode, timecode, length, angle, lh);

	survive_kalman_tracker_integrate_light(so->tracker, &l.common);
	SURVIVE_POSER_INVOKE(so, &l);
}
