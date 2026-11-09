#include <obs.h>
#include <util/platform.h>
#include <util/bmem.h>
#include <string>
#include <cstdio>
#include <cstdlib>
/* the config.json helpers, cut out of vertical-canvas.cpp by run.sh */
#include "helpers.inc"
#include <cerrno>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
/* libobs's os_safe_replace uses rename(); fail it on demand, like a ReplaceFileW that doesn't land */
static bool fail_rename = false;
extern "C" int rename(const char *a, const char *b)
{
	if (fail_rename) {
		errno = EACCES;
		return -1;
	}
	return (int)syscall(SYS_renameat2, AT_FDCWD, a, AT_FDCWD, b, 0);
}
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } else printf("ok: %s\n", #c); } while (0)
static void write_cfg(const std::string &file, long long seq, int outputs)
{
	obs_data_t *c = obs_data_create();
	obs_data_array_t *canvas = obs_data_array_create();
	obs_data_t *d = obs_data_create();
	if (seq)
		obs_data_set_int(d, kCtdSaveSeq, seq);
	obs_data_array_t *so = obs_data_array_create();
	for (int i = 0; i < outputs; i++) {
		obs_data_t *o = obs_data_create();
		obs_data_set_string(o, "name", "CTD Relay");
		obs_data_array_push_back(so, o);
		obs_data_release(o);
	}
	obs_data_set_array(d, "stream_outputs", so);
	obs_data_array_release(so);
	obs_data_array_push_back(canvas, d);
	obs_data_release(d);
	obs_data_set_array(c, "canvas", canvas);
	obs_data_array_release(canvas);
	obs_data_save_json(c, file.c_str());
	obs_data_release(c);
}
static int outputs_of(obs_data_t *c)
{
	obs_data_array_t *canvas = obs_data_get_array(c, "canvas");
	obs_data_t *d = obs_data_array_item(canvas, 0);
	obs_data_array_t *so = obs_data_get_array(d, "stream_outputs");
	int n = (int)obs_data_array_count(so);
	obs_data_array_release(so);
	obs_data_release(d);
	obs_data_array_release(canvas);
	return n;
}
/* what save_canvas does with the file, minus the docks */
static bool save(const std::string &p, int outputs)
{
	obs_data_t *c = ctd_load_config(p.c_str());
	if (!c)
		c = obs_data_create();
	long long seq = ++ctd_last_save_seq;
	std::string tmp = p + ".new";
	write_cfg(tmp, seq, outputs);
	obs_data_t *n = obs_data_create_from_json_file(tmp.c_str());
	os_unlink(tmp.c_str());
	obs_data_array_t *canvas = obs_data_get_array(n, "canvas");
	obs_data_set_array(c, "canvas", canvas);
	obs_data_array_release(canvas);
	obs_data_release(n);
	bool ok = ctd_save_config(c, p.c_str());
	obs_data_release(c);
	return ok;
}
int main(int, char **argv)
{
	std::string dir = argv[1];
	std::string p = dir + "/config.json";
	// 1. the PC's state today: config.json legacy with 0 outputs, .bak holds the CTD Relay output (also legacy, no seq)
	write_cfg(p, 0, 0);
	write_cfg(p + ".bak", 0, 1);
	obs_data_t *c = ctd_load_config(p.c_str());
	CHECK(outputs_of(c) == 0);
	obs_data_release(c); // tie -> config.json (can't tell)
	// 2. proc sets the output and saves: config.json must hold it, and survive a "restart" (fresh load)
	CHECK(save(p, 1));
	ctd_last_save_seq = 0;
	c = ctd_load_config(p.c_str());
	CHECK(outputs_of(c) == 1);
	obs_data_release(c);
	// 3. exit save, then restart
	CHECK(save(p, 1));
	ctd_last_save_seq = 0;
	c = ctd_load_config(p.c_str());
	CHECK(outputs_of(c) == 1);
	CHECK(ctd_config_seq(c) == 2);
	obs_data_release(c);
	// 4. something without the CTD stamp rewrites config.json with 0 outputs (stock plugin / stale writer): .bak wins
	{
		obs_data_t *cfg = obs_data_create_from_json_file(p.c_str());
		write_cfg(p + ".x", 0, 0);
		obs_data_t *x = obs_data_create_from_json_file((p + ".x").c_str());
		os_unlink((p + ".x").c_str());
		obs_data_array_t *a = obs_data_get_array(x, "canvas");
		obs_data_set_array(cfg, "canvas", a);
		obs_data_array_release(a);
		obs_data_save_json_safe(cfg, p.c_str(), "tmp", "bak");
		obs_data_release(x);
		obs_data_release(cfg);
	}
	c = ctd_load_config(p.c_str());
	CHECK(outputs_of(c) == 1);
	obs_data_release(c);
	// 5. the safe swap fails (here: .bak can't be replaced): config.json is still written, .tmp cleaned up
	fail_rename = true;
	ctd_last_save_seq = 0;
	CHECK(save(p, 1));
	fail_rename = false;
	c = obs_data_create_from_json_file(p.c_str());
	CHECK(ctd_config_seq(c) == 3 && outputs_of(c) == 1);
	obs_data_release(c);
	CHECK(!os_file_exists((p + ".tmp").c_str()));
	// 5b. the same failure before this fix: obs_data_save_json_safe alone leaves config.json stale
	{
		obs_data_t *n = obs_data_create();
		fail_rename = true;
		CHECK(!obs_data_save_json_safe(n, p.c_str(), "tmp", "bak"));
		fail_rename = false;
		obs_data_release(n);
		os_unlink((p + ".tmp").c_str());
	}
	// 6. a leftover newer .tmp (a swap that failed before this fix) is loaded
	write_cfg(p + ".tmp", 9, 1);
	write_cfg(p, 4, 0);
	c = ctd_load_config(p.c_str());
	CHECK(ctd_config_seq(c) == 9 && outputs_of(c) == 1);
	obs_data_release(c);
	printf(fails ? "FAILED %d\n" : "ALL PASSED\n", fails);
	return fails != 0;
}
