// CTD: encoder ownership check against real libobs (see run.sh). Not part of the plugin build.
//  - "old": what the canvas did before the fix (get -> set -> release(old)), on the plain path and on an output slot
//    the canvas holds no reference for. Each case runs in a child process, so a crash is reported, not fatal.
//  - "fixed": output_take_video_encoder / output_take_audio_encoder, cut out of vertical-canvas.cpp by run.sh, over
//    the sequences the canvas runs: start, restart, encoder change, encoders shared across outputs, record and
//    Backtrack reuse, Settings clearing encoders, a refused encoder, outputs removed.
#include <obs.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <vector>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "helpers.inc"

static const char *nm(void *)
{
	return "dummy";
}
static void *enc_create(obs_data_t *, obs_encoder_t *)
{
	return (void *)1;
}
static void enc_destroy(void *) {}
static bool enc_encode(void *, struct encoder_frame *, struct encoder_packet *, bool *)
{
	return false;
}
static void *out_create(obs_data_t *, obs_output_t *)
{
	return (void *)1;
}
static void out_destroy(void *) {}
static bool out_start(void *)
{
	return false;
}
static void out_stop(void *, uint64_t) {}
static void out_packet(void *, struct encoder_packet *) {}

static void register_types()
{
	struct obs_encoder_info v = {};
	v.id = "dummy_venc_a";
	v.type = OBS_ENCODER_VIDEO;
	v.codec = "h264";
	v.get_name = nm;
	v.create = enc_create;
	v.destroy = enc_destroy;
	v.encode = enc_encode;
	obs_register_encoder(&v);
	v.id = "dummy_venc_b";
	obs_register_encoder(&v);
	struct obs_encoder_info a = v;
	a.id = "dummy_aenc";
	a.type = OBS_ENCODER_AUDIO;
	a.codec = "aac";
	obs_register_encoder(&a);

	struct obs_output_info o = {};
	o.id = "dummy_output";
	o.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK;
	o.get_name = nm;
	o.create = out_create;
	o.destroy = out_destroy;
	o.start = out_start;
	o.stop = out_stop;
	o.encoded_packet = out_packet;
	obs_register_output(&o);
}

static int failures = 0;
#define CHECK(cond, msg) \
	do { \
		if (!(cond)) { \
			failures++; \
			printf("    FAIL: %s\n", msg); \
		} \
	} while (0)

struct Watch {
	std::string name;
	obs_weak_encoder_t *weak;
};
static std::vector<Watch> watched;

static obs_encoder_t *watch(obs_encoder_t *e, const char *name)
{
	watched.push_back({name, obs_encoder_get_weak_encoder(e)});
	return e;
}
static bool alive(const char *name)
{
	for (auto &w : watched) {
		if (w.name == name) {
			obs_encoder_t *e = obs_weak_encoder_get_encoder(w.weak);
			if (e) {
				obs_encoder_release(e);
				return true;
			}
			return false;
		}
	}
	return false;
}
static int alive_count()
{
	int n = 0;
	for (auto &w : watched) {
		n += alive(w.name.c_str()) ? 1 : 0;
	}
	return n;
}
static obs_encoder_t *venc(const char *id, const char *name)
{
	return watch(obs_video_encoder_create(id, name, nullptr, nullptr), name);
}
static obs_encoder_t *aenc(const char *name)
{
	return watch(obs_audio_encoder_create("dummy_aenc", name, nullptr, 0, nullptr), name);
}
static obs_output_t *out(const char *name)
{
	return obs_output_create("dummy_output", name, nullptr, nullptr);
}

/* ---- the canvas's old pattern ---- */
static void old_swap_video(obs_output_t *o, obs_encoder_t *new_enc /* owned */)
{
	auto old_enc = obs_output_get_video_encoder(o);
	if (old_enc != new_enc) {
		obs_output_set_video_encoder(o, new_enc);
		obs_encoder_release(old_enc);
	} else {
		obs_encoder_release(new_enc);
	}
}

static void old_plain_path()
{
	obs_output_t *stream = out("stream");
	old_swap_video(stream, venc("dummy_venc_a", "A"));                                 // first start
	old_swap_video(stream, obs_encoder_get_ref(obs_output_get_video_encoder(stream))); // restart, same encoder
	old_swap_video(stream, venc("dummy_venc_b", "B"));                                 // encoder changed
	CHECK(!alive("A"), "old encoder destroyed after the change");
	CHECK(alive("B"), "new encoder alive");
	auto e = obs_output_get_video_encoder(stream);
	obs_output_release(stream);
	obs_encoder_release(e);
	CHECK(alive_count() == 0, "nothing left");
}

/* GetStreamVideoEncoder's Backtrack branch handed back the Backtrack output's encoder without a reference */
static void old_backtrack_borrow()
{
	obs_output_t *replay = out("replay");
	obs_output_t *stream = out("stream");
	old_swap_video(replay, venc("dummy_venc_a", "A")); // Backtrack started: canvas ref + output ref
	// StartStream: GetStreamVideoEncoder() returns replay's encoder, not a reference of its own
	obs_encoder_t *borrowed = obs_output_get_video_encoder(replay);
	old_swap_video(stream, borrowed); // stream slot now holds A, the canvas holds no reference for it
	// Settings > record encoder changed (Backtrack idle): the canvas clears Backtrack's encoder and drops its reference
	{
		auto old_enc = obs_output_get_video_encoder(replay);
		obs_output_set_video_encoder(replay, nullptr);
		obs_encoder_release(old_enc);
	}
	CHECK(alive("A"), "A still on the stream output");
	fflush(stdout);
	// stream encoder changed, Start Streaming:
	old_swap_video(stream, venc("dummy_venc_b", "B")); // set frees A, then release(A)
	printf("    (no crash this time: freed memory was reused)\n");
}

/* ---- the fixed pattern ---- */
static void new_sequences()
{
	obs_output_t *s1 = out("stream1");
	obs_output_t *s2 = out("stream2");
	obs_output_t *record = out("record");
	obs_output_t *replay = out("replay");

	// StartStream, two outputs sharing the stream encoders (OBSEncoderAutoRelease in the canvas)
	{
		obs_encoder_t *v = venc("dummy_venc_a", "V1");
		obs_encoder_t *a = aenc("A1");
		obs_output_set_video_encoder(s1, v);
		obs_output_set_audio_encoder(s1, a, 0);
		obs_output_set_video_encoder(s2, v);
		obs_output_set_audio_encoder(s2, a, 0);
		obs_encoder_release(v);
		obs_encoder_release(a);
	}
	CHECK(alive("V1") && alive("A1"), "stream encoders alive on the outputs");

	// restart with the same encoder (GetStreamVideoEncoder: get_ref of the one on the output)
	for (int i = 0; i < 3; i++) {
		output_take_video_encoder(s1, obs_encoder_get_ref(obs_output_get_video_encoder(s1)));
		output_take_audio_encoder(s1, obs_encoder_get_ref(obs_output_get_audio_encoder(s1, 0)), 0);
	}
	CHECK(alive("V1") && alive("A1"), "restart keeps the encoders");

	// record reuses the stream encoder; Backtrack reuses the record encoder (borrowed pointer passed to set)
	output_take_video_encoder(record, obs_encoder_get_ref(obs_output_get_video_encoder(s1)));
	obs_output_set_video_encoder(replay, obs_output_get_video_encoder(record));
	output_take_audio_encoder(record, aenc("RA0"), 0);
	output_take_audio_encoder(replay, obs_encoder_get_ref(obs_output_get_audio_encoder(record, 0)), 0);
	for (size_t idx = 1; idx < MAX_AUDIO_MIXES; idx++) {
		obs_output_set_audio_encoder(record, nullptr, idx);
	}

	// stream encoder changed: both stream outputs get the new one
	{
		obs_encoder_t *v = venc("dummy_venc_b", "V2");
		obs_output_set_video_encoder(s1, v);
		obs_output_set_video_encoder(s2, v);
		obs_encoder_release(v);
	}
	CHECK(alive("V1"), "old stream encoder still alive: record and Backtrack hold it");
	CHECK(alive("V2"), "new stream encoder alive");

	// record and Backtrack pick up the new stream encoder on their next start
	output_take_video_encoder(record, obs_encoder_get_ref(obs_output_get_video_encoder(s1)));
	CHECK(alive("V1"), "old encoder alive while Backtrack holds it");
	output_take_video_encoder(replay, obs_encoder_get_ref(obs_output_get_video_encoder(s1)));
	CHECK(!alive("V1"), "old encoder destroyed once no output holds it");

	// Settings: stream encoder / audio track changed -> cleared on idle outputs
	obs_output_set_video_encoder(s2, nullptr);
	obs_output_set_audio_encoder(s2, nullptr, 0);
	obs_output_set_audio_encoder(s2, nullptr, 0); // already empty
	CHECK(alive("V2") && alive("A1"), "cleared on one output, alive on the others");

	// a new encoder the output refuses (wrong type stands in for "output is active") is dropped, not leaked
	output_take_video_encoder(s1, aenc("WRONG"));
	CHECK(!alive("WRONG"), "refused encoder dropped");
	CHECK(obs_output_get_video_encoder(s1) != nullptr && alive("V2"), "encoder on the output untouched");

	// output without an object yet (GetStreamAudioEncoder guards it; the helpers do too)
	output_take_audio_encoder(nullptr, aenc("NOOUT"), 0);
	CHECK(!alive("NOOUT"), "encoder for a missing output dropped");

	// outputs removed / canvas destroyed: releasing the output releases its encoders
	obs_output_release(s2);
	obs_output_release(record);
	CHECK(alive("V2") && alive("A1") && alive("RA0"), "encoders alive while another output holds them");
	obs_output_release(replay);
	CHECK(!alive("RA0"), "record audio encoder destroyed with its last output");
	obs_output_release(s1);
	CHECK(alive_count() == 0, "no encoder left after the last output is released");
	if (alive_count()) {
		for (auto &w : watched) {
			if (alive(w.name.c_str())) {
				printf("    leaked: %s\n", w.name.c_str());
			}
		}
	}
}

static int run_case(const char *title, void (*fn)())
{
	printf("%s\n", title);
	fflush(stdout);
	pid_t pid = fork();
	if (pid == 0) {
		fn();
		fflush(stdout);
		_exit(failures ? 1 : 0);
	}
	int status = 0;
	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status)) {
		printf("    CRASH: signal %d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
		return -WTERMSIG(status);
	}
	printf("    %s\n", WEXITSTATUS(status) == 0 ? "ok" : "FAILED");
	return WEXITSTATUS(status);
}

int main()
{
	if (!getenv("REFTEST_VERBOSE")) {
		base_set_log_handler([](int, const char *, va_list, void *) {}, nullptr);
	}
	if (!obs_startup("en-US", nullptr, nullptr)) {
		printf("obs_startup failed (libobs needs an X display on Linux: use xvfb-run)\n");
		return 2;
	}
	printf("libobs %s\n", obs_get_version_string());
	if (obs_get_version() < MAKE_SEMANTIC_VERSION(31, 0, 0)) {
		printf("needs libobs 31.0 or newer: before that, outputs kept a plain pointer to their encoders\n");
		return 2;
	}
	register_types();

	if (getenv("REFTEST_DIRECT")) { /* the crashing case without a child process, for a debugger */
		old_backtrack_borrow();
		return 0;
	}

	int r1 = run_case("old pattern, plain path (create, restart, encoder change):", old_plain_path);
	int r2 = run_case("old pattern, stream slot holds the Backtrack encoder without a canvas reference, then encoder change:",
			  old_backtrack_borrow);
	int r3 = run_case("fixed pattern, all sequences:", new_sequences);

	printf("\nsummary: old plain path %s; old unowned slot %s; fixed pattern %s\n", r1 == 0 ? "balanced" : "BROKEN",
	       r2 < 0 ? "crashes" : "did not crash", r3 == 0 ? "clean (no crash, no leak)" : "BROKEN");
	return (r1 == 0 && r3 == 0) ? 0 : 1;
}
