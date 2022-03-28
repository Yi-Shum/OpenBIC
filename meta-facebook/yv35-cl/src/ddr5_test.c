#include <shell/shell.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <drivers/i3c/i3c.h>
#include <device.h>
#include <logging/log.h>

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(ddr5_test);

static int cmd_ddr5_init(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *master;
	int ret;

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c3)));
	if (!master) {
		LOG_ERR("master device not found\n");
		return 0;
	}

	ret = i3c_master_send_rstdaa(master);
	if (ret) {
		LOG_ERR("RSTDAA failed %d\n", ret);
		return 0;
	}

	ret = i3c_master_send_aasa(master);
	if (ret) {
		LOG_ERR("SETAASA failed\n");
		return 0;
	}

	return 0;
}

static int cmd_ddr5_write_read(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *master;
	static struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer[2];
	int ret;
	uint8_t in[16], out[16];

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c3)));
	if (!master) {
		LOG_ERR("master device not found\n");
		return 0;
	}

	uint8_t device_addr, write_len, read_len;
	device_addr = strtol(argv[1], NULL, 16);
	write_len = strtol(argv[2], NULL, 16);
	int i = 0;
	for (; i < write_len; i++) {
		in[i] = strtol(argv[i + 3], NULL, 16);
	}
	read_len = strtol(argv[i + 3], NULL, 16);

	slave.info.static_addr = device_addr;
	slave.info.assigned_dynamic_addr = slave.info.static_addr;
	slave.info.i2c_mode = 0;

	ret = i3c_master_attach_device(master, &slave);
	if (ret) {
		LOG_ERR("failed to attach slave\n");
		return 0;
	}

	xfer[0].rnw = 0;
	xfer[0].len = write_len;
	xfer[0].data.in = in;

	xfer[1].rnw = 1;
	xfer[1].len = read_len;
	xfer[1].data.out = out;

	ret = i3c_master_priv_xfer(&slave, xfer, 2);
	LOG_HEXDUMP_INF(out, read_len, "Read data");

	ret = i3c_master_detach_device(master, &slave);
	if (ret) {
		LOG_ERR("failed to attach slave\n");
		return 0;
	}

	return 0;
}

static int cmd_ddr5_read(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *master;
	static struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer;
	int ret;
	uint8_t out[16];

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c3)));
	if (!master) {
		LOG_ERR("master device not found\n");
		return 0;
	}

	uint8_t device_addr, read_len;
	device_addr = strtol(argv[1], NULL, 16);
	read_len = strtol(argv[2], NULL, 16);

	slave.info.static_addr = device_addr;
	slave.info.assigned_dynamic_addr = slave.info.static_addr;
	slave.info.i2c_mode = 0;

	ret = i3c_master_attach_device(master, &slave);
	if (ret) {
		LOG_ERR("failed to attach slave\n");
		return 0;
	}

	xfer.rnw = 1;
	xfer.len = read_len;
	xfer.data.out = out;

	ret = i3c_master_priv_xfer(&slave, &xfer, 1);
	LOG_HEXDUMP_INF(out, read_len, "Read data");

	ret = i3c_master_detach_device(master, &slave);
	if (ret) {
		LOG_ERR("failed to attach slave\n");
		return 0;
	}

	return 0;
}

static int cmd_ddr5_write(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *master;
	static struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer;
	int ret;
	uint8_t in[16];

	master = device_get_binding(DT_LABEL(DT_NODELABEL(i3c3)));
	if (!master) {
		LOG_ERR("master device not found\n");
		return 0;
	}

	uint8_t device_addr, write_len;
	device_addr = strtol(argv[1], NULL, 16);
	write_len = strtol(argv[2], NULL, 16);

	for (int i = 0; i < write_len; i++) {
		in[i] = strtol(argv[i + 3], NULL, 16);
	}

	slave.info.static_addr = device_addr;
	slave.info.assigned_dynamic_addr = slave.info.static_addr;
	slave.info.i2c_mode = 0;

	ret = i3c_master_attach_device(master, &slave);
	if (ret) {
		LOG_ERR("failed to attach slave\n");
		return 0;
	}

	xfer.rnw = 0;
	xfer.len = write_len;
	xfer.data.in = in;

	ret = i3c_master_priv_xfer(&slave, &xfer, 1);

	ret = i3c_master_detach_device(master, &slave);
	if (ret) {
		LOG_ERR("failed to attach slave\n");
		return 0;
	}

	return 0;
}

static int cmd_ddr5_set_clk(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t high_ns, low_ns;
	high_ns = strtol(argv[1], NULL, 16);
	low_ns = strtol(argv[2], NULL, 16);

	uint8_t hcnt, lcnt;
	hcnt = (float)high_ns * 0.2;
	lcnt = (float)low_ns * 0.2;

	uint32_t scl_i3c_PP_reg = 0x7e7a50b8;
	uint32_t reg_val = *(uint32_t *)scl_i3c_PP_reg;

	reg_val &= ~(0xf << 16 | 0xf);
	reg_val |= ((hcnt & 0xf) << 16 | (lcnt & 0xf));
	*(uint32_t *)scl_i3c_PP_reg = reg_val;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ddr5_test, SHELL_CMD(init, NULL, "Initialize I3C bus", cmd_ddr5_init),
	SHELL_CMD(write_read, NULL,
		  "write and read: <addr> <write bytes> <write data...> <read length>",
		  cmd_ddr5_write_read),
	SHELL_CMD(read, NULL, "read: <addr> <read bytes>", cmd_ddr5_read),
	SHELL_CMD(write, NULL, "write: <addr> <write bytes> <data...>", cmd_ddr5_write),
	SHELL_CMD(set_clk, NULL, "set i3c PP clock: <high ns> <low ns>", cmd_ddr5_set_clk),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(ddr5, &sub_ddr5_test, "I3C to access DDR5", NULL);
