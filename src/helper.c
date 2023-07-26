/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <zephyr/kernel.h>

#include "nanocbor/nanocbor.h"

#define CBOR_READ_BUFFER_BYTES 4096
#define MAX_DEPTH 20

static char buffer[CBOR_READ_BUFFER_BYTES];

static int _parse_type(nanocbor_value_t *value, unsigned indent);

static bool _pretty = false;

static void _print_indent(unsigned indent)
{
    if (_pretty) {
        for (unsigned i = 0; i < indent; i++) {
            printk("  ");
        }
    }
}

static void _print_separator(void)
{
    if (_pretty) {
        printk("\n");
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_cbor(nanocbor_value_t *it, unsigned indent)
{
    while (!nanocbor_at_end(it)) {
        _print_indent(indent);
        int res = _parse_type(it, indent);

        if (res < 0) {
            printk("Err\n");
            break;
        }

        if (!nanocbor_at_end(it)) {
            printk(", ");
        }
        _print_separator();
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_map(nanocbor_value_t *it, unsigned indent)
{
    while (!nanocbor_at_end(it)) {
        _print_indent(indent);
        int res = _parse_type(it, indent);
        printk(": ");
        if (res < 0) {
            printk("Err\n");
            break;
        }
        res = _parse_type(it, indent);
        if (res < 0) {
            printk("Err\n");
            break;
        }
        if (!nanocbor_at_end(it)) {
            printk(", ");
        }
        _print_separator();
    }
}

/* NOLINTNEXTLINE(misc-no-recursion, readability-function-cognitive-complexity) */
static int _parse_type(nanocbor_value_t *value, unsigned indent)
{
    uint8_t type = nanocbor_get_type(value);
    if (indent > MAX_DEPTH) {
        return -2;
    }
    switch (type) {
    case NANOCBOR_TYPE_UINT: {
        uint64_t uint = 0;
        if (nanocbor_get_uint64(value, &uint) >= 0) {
            printk("%" PRIu64, uint);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_NINT: {
        int64_t nint = 0;
        if (nanocbor_get_int64(value, &nint) >= 0) {
            printk("%" PRIi64, nint);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_BSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        if (nanocbor_get_bstr(value, &buf, &len) >= 0 && buf) {
            size_t iter = 0;
            printk("h\'");
            while (iter < len) {
                printk("%.2x", buf[iter]);
                iter++;
            }
            printk("\'");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_TSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        if (nanocbor_get_tstr(value, &buf, &len) >= 0) {
            printk("\"%.*s\"", (int)len, buf);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_ARR: {
        nanocbor_value_t arr;
        if (nanocbor_enter_array(value, &arr) >= 0) {
            printk("[");
            _print_separator();
            _parse_cbor(&arr, indent + 1);
            nanocbor_leave_container(value, &arr);
            _print_indent(indent);
            printk("]");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_MAP: {
        nanocbor_value_t map;
        if (nanocbor_enter_map(value, &map) >= NANOCBOR_OK) {
            printk("{");
            _print_separator();
            _parse_map(&map, indent + 1);
            nanocbor_leave_container(value, &map);
            _print_indent(indent);
            printk("}");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_FLOAT: {
        bool test = false;
        uint8_t simple = 0;
        float fvalue = 0;
        double dvalue = 0;
        if (nanocbor_get_bool(value, &test) >= NANOCBOR_OK) {
            test ? printk("true") : printk("false");
        }
        else if (nanocbor_get_null(value) >= NANOCBOR_OK) {
            printk("null");
        }
        else if (nanocbor_get_undefined(value) >= NANOCBOR_OK) {
            printk("\"undefined\"");
        }
        else if (nanocbor_get_simple(value, &simple) >= NANOCBOR_OK) {
            printk("\"simple(%u)\"", simple);
        }
        else if (nanocbor_get_float(value, &fvalue) >= 0) {
            printk("%f", fvalue);
        }
        else if (nanocbor_get_double(value, &dvalue) >= 0) {
            printk("%f", dvalue);
        }
        else {
            return -1;
        }
        break;
    }
    case NANOCBOR_TYPE_TAG: {
        uint32_t tag = 0;
        if (nanocbor_get_tag(value, &tag) >= NANOCBOR_OK) {
            printk("%" PRIu32 "(", tag);
            _parse_type(value, 0);
            printk(")");
        }
        else {
            return -1;
        }
        break;
    }
    default:
        printk("Unsupported type\n");
        return -1;
    }
    return 1;
}

int nanocbor_print(uint8_t *buffer, size_t len, bool pretty)
{
    _pretty = pretty;

    printk("Start decoding %u bytes:\n", len);

    nanocbor_value_t it;
    nanocbor_decoder_init(&it, buffer, len);
    while (!nanocbor_at_end(&it)) {
        if (nanocbor_skip(&it) < 0) {
            break;
        }
    }

    nanocbor_decoder_init(&it, buffer, len);
    _parse_cbor(&it, 0);
    printk("\n");

    return 0;
}