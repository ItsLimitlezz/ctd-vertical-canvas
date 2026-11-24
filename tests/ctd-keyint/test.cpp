// CTD: the vertical stream's keyframe rule against real obs_data (see run.sh). Not part of the plugin build.
#include <obs.h>
#include <cstdio>

#include "keyint.inc"

static int failures = 0;

static void expect(const char *what, obs_data_t *settings, bool relay, int want)
{
	const int got = ctd_stream_keyint_sec(settings, relay);
	printf("  %-62s relay=%d -> %2d %s\n", what, relay ? 1 : 0, got, got == want ? "ok" : "FAIL");
	if (got != want) {
		failures++;
	}
}

int main()
{
	expect("no settings (main profile in Simple output mode)", nullptr, false, 2);
	expect("no settings (main profile in Simple output mode)", nullptr, true, 2);

	obs_data_t *s = obs_data_create();
	expect("empty settings", s, false, 2);

	/* x264, SVT-AV1 and others default keyint_sec to 0: a default isn't the user's choice */
	obs_data_set_default_int(s, "keyint_sec", 0);
	expect("encoder default 0 only", s, false, 2);
	expect("encoder default 0 only", s, true, 2);

	obs_data_set_int(s, "keyint_sec", 0);
	expect("user chose 0 (auto)", s, false, 0);
	expect("user chose 0 (auto), output feeds CTD Relay", s, true, 2);

	obs_data_set_int(s, "keyint_sec", 4);
	expect("user chose 4 s", s, false, 4);
	expect("user chose 4 s, output feeds CTD Relay", s, true, 4);

	/* what Settings and KeepCtdKeyint store: after this the value is explicit and survives Use main on/off */
	obs_data_t *saved = obs_data_create();
	obs_data_set_default_int(saved, "keyint_sec", 0);
	obs_data_set_int(saved, "keyint_sec", ctd_stream_keyint_sec(saved, true));
	obs_data_t *reloaded = obs_data_create_from_json(obs_data_get_json(saved));
	expect("stored by Settings, saved to JSON and loaded again", reloaded, false, 2);
	if (!obs_data_has_user_value(reloaded, "keyint_sec")) {
		printf("  FAIL: the stored keyframe interval isn't in the saved JSON\n");
		failures++;
	}

	obs_data_release(reloaded);
	obs_data_release(saved);
	obs_data_release(s);
	printf(failures ? "\n%d failure(s)\n" : "\nall ok\n", failures);
	return failures ? 1 : 0;
}
