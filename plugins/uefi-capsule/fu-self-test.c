/*
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <fwupdplugin.h>

#include "fu-context-private.h"
#include "fu-ucs2.h"
#include "fu-uefi-backend.h"
#include "fu-uefi-bgrt.h"
#include "fu-uefi-cod-device.h"
#include "fu-uefi-common.h"
#include "fu-uefi-pcrs.h"

static void
fu_uefi_pcrs_1_2_func(void)
{
	gboolean ret;
	g_autoptr(FuUefiPcrs) pcrs = fu_uefi_pcrs_new();
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) pcr0s = NULL;
	g_autoptr(GPtrArray) pcrXs = NULL;
	g_autofree gchar *testdatadir = NULL;

	testdatadir = g_test_build_filename(G_TEST_DIST, "tests", NULL);
	g_setenv("FWUPD_SYSFSTPMDIR", testdatadir, TRUE);

	ret = fu_uefi_pcrs_setup(pcrs, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	pcr0s = fu_uefi_pcrs_get_checksums(pcrs, 0);
	g_assert_nonnull(pcr0s);
	g_assert_cmpint(pcr0s->len, ==, 1);
	pcrXs = fu_uefi_pcrs_get_checksums(pcrs, 999);
	g_assert_nonnull(pcrXs);
	g_assert_cmpint(pcrXs->len, ==, 0);

	g_unsetenv("FWUPD_SYSFSTPMDIR");
}

static void
fu_uefi_pcrs_2_0_func(void)
{
	g_autoptr(FuUefiPcrs) pcrs = fu_uefi_pcrs_new();
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) pcr0s = NULL;
	g_autoptr(GPtrArray) pcrXs = NULL;
	const gchar *tpm_server_running = g_getenv("TPM_SERVER_RUNNING");
	g_setenv("FWUPD_FORCE_TPM2", "1", TRUE);

#ifndef HAVE_TSS2
	g_test_skip("Compiled without TPM2.0 support");
	return;
#endif

#ifdef HAVE_GETUID
	if (tpm_server_running == NULL && (getuid() != 0 || geteuid() != 0)) {
		g_test_skip("TPM2.0 tests require simulated TPM2.0 running or need root access "
			    "with physical TPM");
		return;
	}
#endif

	if (!fu_uefi_pcrs_setup(pcrs, &error)) {
		if (tpm_server_running == NULL &&
		    g_error_matches(error, FWUPD_ERROR, FWUPD_ERROR_NOT_FOUND)) {
			g_test_skip("no physical or simulated TPM 2.0 device available");
			return;
		}
	}
	g_assert_no_error(error);
	pcr0s = fu_uefi_pcrs_get_checksums(pcrs, 0);
	g_assert_nonnull(pcr0s);
	g_assert_cmpint(pcr0s->len, >=, 1);
	pcrXs = fu_uefi_pcrs_get_checksums(pcrs, 999);
	g_assert_nonnull(pcrXs);
	g_assert_cmpint(pcrXs->len, ==, 0);
	g_unsetenv("FWUPD_FORCE_TPM2");
}

static void
fu_uefi_ucs2_func(void)
{
	g_autofree guint16 *str1 = NULL;
	g_autofree gchar *str2 = NULL;
	str1 = fu_uft8_to_ucs2("hw!", -1);
	g_assert_cmpint(fu_ucs2_strlen(str1, -1), ==, 3);
	str2 = fu_ucs2_to_uft8(str1, -1);
	g_assert_cmpstr("hw!", ==, str2);
}

static void
fu_uefi_bgrt_func(void)
{
	gboolean ret;
	g_autoptr(GError) error = NULL;
	g_autoptr(FuUefiBgrt) bgrt = fu_uefi_bgrt_new();
	ret = fu_uefi_bgrt_setup(bgrt, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_assert_true(fu_uefi_bgrt_get_supported(bgrt));
	g_assert_cmpint(fu_uefi_bgrt_get_xoffset(bgrt), ==, 123);
	g_assert_cmpint(fu_uefi_bgrt_get_yoffset(bgrt), ==, 456);
	g_assert_cmpint(fu_uefi_bgrt_get_width(bgrt), ==, 54);
	g_assert_cmpint(fu_uefi_bgrt_get_height(bgrt), ==, 24);
}

static void
fu_uefi_framebuffer_func(void)
{
	gboolean ret;
	guint32 height = 0;
	guint32 width = 0;
	g_autoptr(GError) error = NULL;
	ret = fu_uefi_get_framebuffer_size(&width, &height, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_assert_cmpint(width, ==, 456);
	g_assert_cmpint(height, ==, 789);
}

static void
fu_uefi_bitmap_func(void)
{
	gboolean ret;
	gsize sz = 0;
	guint32 height = 0;
	guint32 width = 0;
	g_autofree gchar *fn = NULL;
	g_autofree gchar *buf = NULL;
	g_autoptr(GError) error = NULL;

	fn = g_test_build_filename(G_TEST_DIST, "tests", "test.bmp", NULL);
	ret = g_file_get_contents(fn, &buf, &sz, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_assert_nonnull(buf);
	ret = fu_uefi_get_bitmap_size((guint8 *)buf, sz, &width, &height, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_assert_cmpint(width, ==, 54);
	g_assert_cmpint(height, ==, 24);
}

static GByteArray *
fu_uefi_cod_device_build_efi_string(const gchar *text)
{
	GByteArray *array = g_byte_array_new();
	glong items_written = 0;
	g_autofree gunichar2 *test_utf16 = NULL;
	g_autoptr(GError) error = NULL;

	fu_byte_array_append_uint32(array, 0x0, G_LITTLE_ENDIAN); /* attrs */
	test_utf16 = g_utf8_to_utf16(text, -1, NULL, &items_written, &error);
	g_assert_no_error(error);
	g_assert_nonnull(test_utf16);
	g_byte_array_append(array, (const guint8 *)test_utf16, items_written * 2);
	return array;
}

static GByteArray *
fu_uefi_cod_device_build_efi_result(const gchar *guidstr)
{
	GByteArray *array = g_byte_array_new();
	fwupd_guid_t guid = {0x0};
	gboolean ret;
	guint8 timestamp[16] = {0x0};
	g_autoptr(GError) error = NULL;

	fu_byte_array_append_uint32(array, 0x0, G_LITTLE_ENDIAN);  /* attrs */
	fu_byte_array_append_uint32(array, 0x3A, G_LITTLE_ENDIAN); /* VariableTotalSize */
	fu_byte_array_append_uint32(array, 0xFF, G_LITTLE_ENDIAN); /* Reserved */
	ret = fwupd_guid_from_string(guidstr, &guid, FWUPD_GUID_FLAG_MIXED_ENDIAN, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_byte_array_append(array, guid, sizeof(guid));		  /* CapsuleGuid */
	g_byte_array_append(array, timestamp, sizeof(timestamp)); /* CapsuleProcessed */
	fu_byte_array_append_uint32(array,
				    FU_UEFI_DEVICE_STATUS_ERROR_PWR_EVT_BATT,
				    G_LITTLE_ENDIAN); /* Status */
	return array;
}

static void
fu_uefi_cod_device_write_efi_name(const gchar *name, GByteArray *array)
{
	gboolean ret;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *fn = g_strdup_printf("%s-%s", name, FU_EFIVAR_GUID_EFI_CAPSULE_REPORT);
	g_autofree gchar *path = NULL;
	path = g_test_build_filename(G_TEST_DIST, "tests", "efi", "efivars", fn, NULL);
	ret = g_file_set_contents(path, (gchar *)array->data, array->len, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
}

static void
fu_uefi_cod_device_func(void)
{
	gboolean ret;
	g_autoptr(FuDevice) dev = NULL;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *str = NULL;

	/* these are checked into git and so are not required */
	if (g_getenv("FWUPD_UEFI_CAPSULE_RECREATE_COD_DATA") != NULL) {
		g_autoptr(GByteArray) cap0 = NULL;
		g_autoptr(GByteArray) cap1 = NULL;
		g_autoptr(GByteArray) last = NULL;
		g_autoptr(GByteArray) max = NULL;

		last = fu_uefi_cod_device_build_efi_string("Capsule0001");
		max = fu_uefi_cod_device_build_efi_string("Capsule9999");
		cap0 = fu_uefi_cod_device_build_efi_result("99999999-bf9d-540b-b92b-172ce31013c1");
		cap1 = fu_uefi_cod_device_build_efi_result("cc4cbfa9-bf9d-540b-b92b-172ce31013c1");
		fu_uefi_cod_device_write_efi_name("CapsuleLast", last);
		fu_uefi_cod_device_write_efi_name("CapsuleMax", max);
		fu_uefi_cod_device_write_efi_name("Capsule0000", cap0);
		fu_uefi_cod_device_write_efi_name("Capsule0001", cap1);
	}

	/* create device */
	dev = g_object_new(FU_TYPE_UEFI_COD_DEVICE,
			   "fw-class",
			   "cc4cbfa9-bf9d-540b-b92b-172ce31013c1",
			   NULL);
	ret = fu_device_get_results(dev, &error);
	g_assert_no_error(error);
	g_assert_true(ret);

	/* debug */
	str = fu_device_to_string(dev);
	g_debug("%s", str);
	g_assert_cmpint(fu_device_get_update_state(dev), ==, FWUPD_UPDATE_STATE_FAILED_TRANSIENT);
	g_assert_cmpstr(fu_device_get_update_error(dev),
			==,
			"failed to update to 0: battery level is too low");
	g_assert_cmpint(fu_uefi_device_get_status(FU_UEFI_DEVICE(dev)),
			==,
			FU_UEFI_DEVICE_STATUS_ERROR_PWR_EVT_BATT);
}

static void
fu_uefi_device_func(void)
{
	/* check enums all converted */
	for (guint i = 0; i < FU_UEFI_DEVICE_STATUS_LAST; i++)
		g_assert_nonnull(fu_uefi_device_status_to_string(i));
}

static void
fu_uefi_plugin_func(void)
{
	FuUefiDevice *dev;
	gboolean ret;
	g_autoptr(FuContext) ctx = fu_context_new();
	g_autoptr(FuBackend) backend = fu_uefi_backend_new(ctx);
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) devices = NULL;

#ifndef __linux__
	g_test_skip("ESRT data is mocked only on Linux");
	return;
#endif

	/* do not save silo */
	ret = fu_context_load_quirks(ctx, FU_QUIRKS_LOAD_FLAG_NO_CACHE, &error);
	g_assert_no_error(error);
	g_assert_true(ret);

	/* add each device */
	ret = fu_backend_coldplug(backend, &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	devices = fu_backend_get_devices(backend);
	g_assert_cmpint(devices->len, ==, 3);

	/* system firmware */
	dev = g_ptr_array_index(devices, 0);
	ret = fu_device_probe(FU_DEVICE(dev), &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_assert_cmpint(fu_uefi_device_get_kind(dev), ==, FU_UEFI_DEVICE_KIND_SYSTEM_FIRMWARE);
	g_assert_cmpstr(fu_uefi_device_get_guid(dev), ==, "ddc0ee61-e7f0-4e7d-acc5-c070a398838e");
	g_assert_cmpint(fu_uefi_device_get_hardware_instance(dev), ==, 0x0);
	g_assert_cmpint(fu_uefi_device_get_version(dev), ==, 65586);
	g_assert_cmpint(fu_uefi_device_get_version_lowest(dev), ==, 65582);
	g_assert_cmpint(fu_uefi_device_get_version_error(dev), ==, 18472960);
	g_assert_cmpint(fu_uefi_device_get_capsule_flags(dev), ==, 0xfe);
	g_assert_cmpint(fu_uefi_device_get_status(dev),
			==,
			FU_UEFI_DEVICE_STATUS_ERROR_UNSUCCESSFUL);

	/* system firmware */
	dev = g_ptr_array_index(devices, 1);
	ret = fu_device_probe(FU_DEVICE(dev), &error);
	g_assert_no_error(error);
	g_assert_true(ret);
	g_assert_cmpint(fu_uefi_device_get_kind(dev), ==, FU_UEFI_DEVICE_KIND_DEVICE_FIRMWARE);
	g_assert_cmpstr(fu_uefi_device_get_guid(dev), ==, "671d19d0-d43c-4852-98d9-1ce16f9967e4");
	g_assert_cmpint(fu_uefi_device_get_version(dev), ==, 3090287969);
	g_assert_cmpint(fu_uefi_device_get_version_lowest(dev), ==, 1);
	g_assert_cmpint(fu_uefi_device_get_version_error(dev), ==, 0);
	g_assert_cmpint(fu_uefi_device_get_capsule_flags(dev), ==, 32784);
	g_assert_cmpint(fu_uefi_device_get_status(dev), ==, FU_UEFI_DEVICE_STATUS_SUCCESS);

	/* invalid */
	dev = g_ptr_array_index(devices, 2);
	ret = fu_device_probe(FU_DEVICE(dev), &error);
	g_assert_error(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED);
	g_assert_false(ret);
}

static void
fu_uefi_update_info_func(void)
{
	FuUefiDevice *dev;
	gboolean ret;
	g_autoptr(FuContext) ctx = fu_context_new();
	g_autoptr(FuBackend) backend = fu_uefi_backend_new(ctx);
	g_autoptr(FuUefiUpdateInfo) info = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) devices = NULL;

#ifndef __linux__
	g_test_skip("ESRT data is mocked only on Linux");
	return;
#endif

	/* add each device */
	ret = fu_backend_coldplug(backend, &error);
	g_assert_no_error(error);
	g_assert_true(ret);

	devices = fu_backend_get_devices(backend);
	g_assert_cmpint(devices->len, ==, 3);
	dev = g_ptr_array_index(devices, 0);
	g_assert_cmpint(fu_uefi_device_get_kind(dev), ==, FU_UEFI_DEVICE_KIND_SYSTEM_FIRMWARE);
	g_assert_cmpstr(fu_uefi_device_get_guid(dev), ==, "ddc0ee61-e7f0-4e7d-acc5-c070a398838e");
	info = fu_uefi_device_load_update_info(dev, &error);
	g_assert_no_error(error);
	g_assert_nonnull(info);
	g_assert_cmpint(fu_uefi_update_info_get_version(info), ==, 0x7);
	g_assert_cmpstr(fu_uefi_update_info_get_guid(info),
			==,
			"697bd920-12cf-4da9-8385-996909bc6559");
	g_assert_cmpint(fu_uefi_update_info_get_capsule_flags(info), ==, 0x50000);
	g_assert_cmpint(fu_uefi_update_info_get_hw_inst(info), ==, 0x0);
	g_assert_cmpint(fu_uefi_update_info_get_status(info),
			==,
			FU_UEFI_UPDATE_INFO_STATUS_ATTEMPT_UPDATE);
	g_assert_cmpstr(fu_uefi_update_info_get_capsule_fn(info),
			==,
			"/EFI/fedora/fw/fwupd-697bd920-12cf-4da9-8385-996909bc6559.cap");
}

int
main(int argc, char **argv)
{
	g_autofree gchar *testdatadir = NULL;

	g_test_init(&argc, &argv, NULL);

	testdatadir = g_test_build_filename(G_TEST_DIST, "tests", NULL);
	g_setenv("FWUPD_SYSFSFWDIR", testdatadir, TRUE);
	g_setenv("FWUPD_SYSFSDRIVERDIR", testdatadir, TRUE);
	g_setenv("FWUPD_UEFI_TEST", "1", TRUE);

	/* only critical and error are fatal */
	g_log_set_fatal_mask(NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
	g_setenv("G_MESSAGES_DEBUG", "all", TRUE);

	/* tests go here */
	g_test_add_func("/uefi/pcrs1.2", fu_uefi_pcrs_1_2_func);
	g_test_add_func("/uefi/pcrs2.0", fu_uefi_pcrs_2_0_func);
	g_test_add_func("/uefi/ucs2", fu_uefi_ucs2_func);
	g_test_add_func("/uefi/bgrt", fu_uefi_bgrt_func);
	g_test_add_func("/uefi/framebuffer", fu_uefi_framebuffer_func);
	g_test_add_func("/uefi/bitmap", fu_uefi_bitmap_func);
	g_test_add_func("/uefi/device", fu_uefi_device_func);
	g_test_add_func("/uefi/cod-device", fu_uefi_cod_device_func);
	g_test_add_func("/uefi/update-info", fu_uefi_update_info_func);
	g_test_add_func("/uefi/plugin", fu_uefi_plugin_func);
	return g_test_run();
}
