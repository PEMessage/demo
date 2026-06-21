#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- xxd ---------- */
#define PRINTF_INFO(fmt, ...) \
    printf("  " fmt, ##__VA_ARGS__)

// Feature:
// 1. only use one printf perline
// 2. fully comply standard
void xxd_minimal(size_t len, void* buf)
{
    if (!buf || len == 0) {
        PRINTF_INFO("Empty buffer\n");
        return;
    }
    unsigned char* ptr = (unsigned char*)buf;

    PRINTF_INFO(" Offset    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

    for (size_t i = 0; i < len; i += 16)
    {
        char line[80];  // 足够容纳一行所有内容
        int pos = 0;

        // Part1: write [Offset]
        pos += sprintf(line + pos, "%08zx  ", i);

        // Part2: write [0 1 ... f]
        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < len)
                pos += sprintf(line + pos, "%02x ", ptr[i + j]);
            else
                pos += sprintf(line + pos, "   ");
        }

        // Part3: write ASCII of [0 1 ... f]
        pos += sprintf(line + pos, " |");
        for (size_t j = 0; j < 16 && i + j < len; j++)
        {
            unsigned char c = ptr[i + j];
            pos += sprintf(line + pos, "%c", isprint(c) ? c : '.');
        }
        pos += sprintf(line + pos, "|");

        PRINTF_INFO("%s\n", line);
    }
}

/* ---------- der ---------- */
typedef struct {
    // const
    const uint8_t *data;
    const size_t  len;

    // var
    uint8_t *pos;


} der_parser_t;

typedef struct {
    uint8_t tag;
    size_t len;

    uint8_t *value;
} der_tlv_t;

#define DER_OK 0
#define DER_ERR_NULL -1
#define DER_ERR_OUT_OF_LENGTH -2
#define DER_ERR_LENGTH_INVALID -3
#define DER_ERR_MULTI_BYTES_TAG -4
#define DER_ERR_END_UNMATCH_BEGIN -5

#define DER_ERR_UNEXPECT_TAG -6



/* ---------- bottom ---------- */
static int der_read_bytes(der_parser_t *p, const size_t n, uint8_t **out_pos)
{
    if (p->pos - p->data + n > p->len) { return DER_ERR_OUT_OF_LENGTH; }

    if (out_pos) { *out_pos = p->pos; }
    p->pos += n;
    return DER_OK;
}


static int der_read_byte(der_parser_t *p, uint8_t *const out_byte)
{
    uint8_t *pos = NULL;
    int res = der_read_bytes(p, 1, &pos);
    if (res) {return res;}

    if (out_byte) { *out_byte = *pos; }

    return DER_OK;
}



/* ---------- middle ---------- */
#define DER_LEN_XTND  0x80		/* Indefinite or long form */
#define DER_LEN_Msk   0x7F		/* Bits 7 - 1 */
static int der_read_length(der_parser_t *p, size_t *const out_len)
{

    uint8_t *save_pos = p->pos;
    int res = 0;
    uint8_t b = 0;
    if ((res = der_read_byte(p, &b))) { return res; }

    if ((b & DER_LEN_XTND) == 0) {
        *out_len = b;
        return DER_OK;
    }

    size_t nbytes = b & DER_LEN_Msk;
    if (nbytes == 0 || nbytes > sizeof(size_t)) {
        p->pos = save_pos;
        return DER_ERR_LENGTH_INVALID;
    }

    size_t len = 0;
    for (size_t i = 0; i < nbytes; i++) {
        if ((res = der_read_byte(p, &b))) { return res; }
        len = (len << 8) | b;
    }
    if (p->pos - p->data + len > p->len) {
        p->pos = save_pos;
        return DER_ERR_OUT_OF_LENGTH;
    }
    if (out_len) { *out_len = len; }
    return DER_OK;
}

/* ---------- core ---------- */
#define DER_TAG_Msk		0x1F	/* Bits 5 - 1 */

int der_read_tl(der_parser_t *p, der_tlv_t *const out_tlv) {
    uint8_t tag = 0;
    int res = 0;

    uint8_t *save_pos = p->pos;
    if ((res = der_read_byte(p, &tag))) {
        p->pos = save_pos;
        return res;
    }

    if ((tag & DER_TAG_Msk) == DER_TAG_Msk) {
        p->pos = save_pos;
        return DER_ERR_MULTI_BYTES_TAG;
    }

    size_t length = 0;
    if((res = der_read_length(p, &length))) { return res;}

    if (out_tlv) {
        out_tlv->tag = tag & DER_TAG_Msk;
        out_tlv->len = length;
        out_tlv->value = p->pos;
    }
    return DER_OK;
}

/* ---------- derive ---------- */
int der_read_tlv(der_parser_t *p, der_tlv_t *const out_tlv) {
    int res = 0;

    der_tlv_t tlv = {0};
    if ((res = der_read_tl(p, &tlv))) { return res; }
    p->pos += tlv.len;
    if (out_tlv) { *out_tlv = tlv; }
    return res;
}


int der_peak_tlv(der_parser_t *p, der_tlv_t *const out_tlv) {
    uint8_t * const save_pos = p->pos;
    int res = der_read_tlv(p, out_tlv);
    p->pos = save_pos;
    return res;
}


#define TLV_CMP_TAG_Pos 1U
#define TLV_CMP_TAG_Msk (1UL << TLV_CMP_TAG_Pos)

#define TLV_CMP_LEN_Pos 2U
#define TLV_CMP_LEN_Msk (1UL << TLV_CMP_LEN_Pos)

#define TLV_CMP_VALUE_Pos 8U
#define TLV_CMP_VALUE_Msk (0xFFUL << TLV_CMP_LEN_Pos)

uint32_t tlv_cmp(der_tlv_t *left, der_tlv_t *right) {
    int result = 0;
    if (left->tag != right->tag) {
        result |= 1 << TLV_CMP_TAG_Pos;
    }
    if (left->len != right->len) {
        result |= 1 << TLV_CMP_LEN_Pos;
    }

    if (result == 0 && left->value && right->value) {
        result |= (int8_t)memcmp(left->value, right->value, left->len) << TLV_CMP_VALUE_Pos;
    }
    return result;
}

#define DER_TAG_LIST \
    X(DER_EOC,             0x00) \
    X(DER_BOOLEAN,         0x01) \
    X(DER_INTEGER,         0x02) \
    X(DER_BITSTRING,       0x03) \
    X(DER_OCTETSTRING,     0x04) \
    X(DER_NULLTAG,         0x05) \
    X(DER_OID,             0x06) \
    X(DER_OBJDESCRIPTOR,   0x07) \
    X(DER_EXTERNAL,        0x08) \
    X(DER_REAL,            0x09) \
    X(DER_ENUMERATED,      0x0A) \
    X(DER_EMBEDDED_PDV,    0x0B) \
    X(DER_UTF8STRING,      0x0C) \
    X(DER_SEQUENCE,        0x10) \
    X(DER_SET,             0x11) \
    X(DER_NUMERICSTRING,   0x12) \
    X(DER_PRINTABLESTRING, 0x13) \
    X(DER_T61STRING,       0x14) \
    X(DER_VIDEOTEXSTRING,  0x15) \
    X(DER_IA5STRING,       0x16) \
    X(DER_UTCTIME,         0x17) \
    X(DER_GENERALIZEDTIME, 0x18) \
    X(DER_GRAPHICSTRING,   0x19) \
    X(DER_VISIBLESTRING,   0x1A) \
    X(DER_GENERALSTRING,   0x1B) \
    X(DER_UNIVERSALSTRING, 0x1C) \
    X(DER_BMPSTRING,       0x1E)

enum {
#define X(name, val) name = val,
    DER_TAG_LIST
#undef X
};

const char* get_tag_name(uint8_t tag) {
    switch (tag) {
#define X(name, val) case name: return #name;
        DER_TAG_LIST
#undef X
        default: return "Unknown";
    }
}

void der_print_tlv(const der_tlv_t *tlv) {
    if (!tlv) {
        printf("NULL PTR\n");
        return;
    }
    printf("T: %s L: %zu\n", get_tag_name(tlv->tag), tlv->len);
    xxd_minimal(tlv->len, tlv->value);

    return;
}


int der_begin(der_parser_t *p, uint8_t tag, uint8_t **out_endpos) {
    if (!out_endpos) { return DER_ERR_NULL; }
    uint8_t *save_pos = p->pos;

    int res = 0;
    der_tlv_t tlv = {0};
    der_tlv_t seq = {
        .tag = tag,
        .len = 0,
        .value = NULL,
    };
    if ((res = der_read_tl(p, &tlv))) {
        p->pos = save_pos;
        return res;
    }
    if (tlv_cmp(&tlv, &seq) & TLV_CMP_TAG_Msk) {
        p->pos = save_pos;
        return DER_ERR_UNEXPECT_TAG;
    }


    *out_endpos = p->pos + tlv.len;
    return DER_OK;
}



int der_end(der_parser_t *p, const uint8_t *endpos) {
    if (p->pos != endpos) { return DER_ERR_END_UNMATCH_BEGIN; }
    return DER_OK;
}



// RSAPrivateKey ::= SEQUENCE {
//     version           Version,
//     modulus           INTEGER,  -- n
//     publicExponent    INTEGER,  -- e
//     privateExponent   INTEGER,  -- d
//     prime1            INTEGER,  -- p
//     prime2            INTEGER,  -- q
//     exponent1         INTEGER,  -- d mod (p-1)
//     exponent2         INTEGER,  -- d mod (q-1)
//     coefficient       INTEGER,  -- (inverse of q) mod p
//     otherPrimeInfos   OtherPrimeInfos OPTIONAL
// }

void der_print_pkcs1_privatekey(der_parser_t *p) {
    uint8_t *endpos = NULL;
    int res = der_begin(p, DER_SEQUENCE, &endpos);
    if (res) {
        printf("Error: expected SEQUENCE at top level\n");
        return;
    }

    static const char *field_names[] = {
        "version",
        "modulus",
        "publicExponent",
        "privateExponent",
        "prime1",
        "prime2",
        "exponent1",
        "exponent2",
        "coefficient",
    };
    const int mandatory_fields = (int)(sizeof(field_names) / sizeof(field_names[0]));

    for (int i = 0; i < mandatory_fields; i++) {
        if (p->pos >= endpos) {
            printf("Error: truncated key, missing '%s'\n", field_names[i]);
            return;
        }
        der_tlv_t tlv = {0};
        res = der_read_tlv(p, &tlv);
        if (res) {
            printf("Error: failed to read '%s' (code %d)\n", field_names[i], res);
            return;
        }
        printf("%s:\n", field_names[i]);
        der_print_tlv(&tlv);
    }

    while (p->pos < endpos) {
        der_tlv_t tlv = {0};
        res = der_read_tlv(p, &tlv);
        if (res) {
            printf("Error: failed to read otherPrimeInfo\n");
            return;
        }
        printf("otherPrimeInfo:\n");
        der_print_tlv(&tlv);
    }

    if (der_end(p, endpos)) {
        printf("Warning: SEQUENCE length mismatch (trailing bytes?)\n");
    }
}


typedef struct buffer {
    size_t len;
    uint8_t* buf;
} buffer_t;

buffer_t* create_file_buffer(const char* path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *rawbuf = (uint8_t *)malloc((size_t)fsize + sizeof(buffer_t));
    if (!rawbuf) {
        perror("malloc");
        fclose(f);
        return NULL;
    }

    if (fread(rawbuf + sizeof(buffer_t), 1, (size_t)fsize, f) != (size_t)fsize) {
        perror("fread");
        free(rawbuf);
        fclose(f);
        return NULL;
    }
    fclose(f);

    buffer_t *buf = (buffer_t *)rawbuf;
    buf->buf = rawbuf + sizeof(buffer_t);
    buf->len = fsize;

    return buf;
}

void delete_file_buffer(buffer_t *buf) {
    free(buf);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <der_file>\n", argv[0]);
        return 1;
    }

    buffer_t *buf = create_file_buffer(argv[1]);

    der_parser_t p = {
        .data = buf->buf,
        .len  = (size_t)buf->len,
        .pos  = buf->buf,
    };

    der_print_pkcs1_privatekey(&p);

    delete_file_buffer(buf);
    return 0;
}

