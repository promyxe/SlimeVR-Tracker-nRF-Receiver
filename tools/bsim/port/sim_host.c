/* Host side (native simulator runner): HID report log. Env SLIME_SIM_OUT (default slime_rx_hid.csv). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void slime_rx_log_report(uint64_t t_us, const uint8_t *report, uint16_t len)
{
	static FILE *out;
	if (!out) {
		const char *path = getenv("SLIME_SIM_OUT");
		out = fopen(path ? path : "slime_rx_hid.csv", "w");
		if (!out) {
			return;
		}
		fprintf(out, "t_us,report_hex\n");
	}
	fprintf(out, "%llu,", (unsigned long long)t_us);
	for (uint16_t i = 0; i < len; i++) {
		fprintf(out, "%02x", report[i]);
	}
	fputc('\n', out);
	fflush(out);
}
